"""Run as WSL root after building vMaNGOS. Owns only wowx_* databases."""
import hashlib
import json
import re
import secrets
import shutil
import subprocess
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SERVER = ROOT / 'server'

def sql(statement, database=None, input_file=None):
    command = ['mariadb', '--batch', '--skip-column-names']
    if database:
        if database not in {'wowx_world','wowx_logon','wowx_characters','wowx_logs'}:
            raise ValueError('Database outside WOWX scope')
        command.append(database)
    if input_file:
        with open(input_file, 'rb') as stream:
            return subprocess.run(command, stdin=stream, check=True, capture_output=True).stdout.decode()
    return subprocess.run(command, input=statement.encode(), check=True, capture_output=True).stdout.decode()

def configure(source, target, values):
    data = source.read_text()
    for name, value in values.items():
        data, count = re.subn(r'^'+re.escape(name)+r'\s*=.*$', name+' = '+value, data, flags=re.M)
        if count != 1:
            raise ValueError('Missing/duplicate config key: '+name)
    target.write_text(data)

def main():
    SERVER.mkdir(exist_ok=True)
    (SERVER/'logs').mkdir(exist_ok=True)
    secret_path=SERVER/'local-credentials.json'
    if secret_path.exists():
        credentials=json.loads(secret_path.read_text())
    else:
        credentials={'db_password':secrets.token_hex(24), 'username':'WOWXTEST', 'password':secrets.token_hex(8).upper()}
        secret_path.write_text(json.dumps(credentials,indent=2)+'\n')
        secret_path.chmod(0o600)
    password=credentials['db_password']
    databases={'world':'mangos.sql','logon':'logon.sql','characters':'characters.sql','logs':'logs.sql'}
    existing=set(sql('SHOW DATABASES').splitlines())
    with zipfile.ZipFile(ROOT/'build/downloads/vmangos-db.zip') as archive:
        for kind, filename in databases.items():
            database='wowx_'+kind
            marker=SERVER/('.imported-'+kind)
            if database not in existing:
                sql(f'CREATE DATABASE {database} CHARACTER SET utf8 COLLATE utf8_general_ci')
                dump=SERVER/filename
                with archive.open('mysql-dump/'+filename) as source, dump.open('wb') as target:
                    shutil.copyfileobj(source,target)
                # The selected snapshot contains table-local SQL; reject database switches.
                with dump.open(errors='replace') as source:
                    for line in source:
                        if re.match(r'\s*(USE\s|CREATE\s+DATABASE|DROP\s+DATABASE)',line,re.I):
                            raise ValueError('Snapshot contains a database-level statement')
                sql('',database,input_file=dump)
                marker.touch()
                print('Imported',database,flush=True)
            elif not marker.exists():
                raise RuntimeError(database+' exists without an import marker; inspect before retrying')
            applied=set(sql('SELECT id FROM migrations',database).splitlines())
            for migration in sorted((ROOT/'upstream/vmangos/sql/migrations').glob('*_'+kind+'.sql')):
                if migration.name.split('_')[0] not in applied:
                    sql('',database,input_file=migration)
                    print('Applied',migration.name,flush=True)
    sql(f"CREATE USER IF NOT EXISTS 'wowx_dev'@'localhost' IDENTIFIED BY '{password}'")
    for kind in databases:
        sql(f"GRANT ALL PRIVILEGES ON wowx_{kind}.* TO 'wowx_dev'@'localhost'")
    # Disposable local account; use SRP verifier fields expected by the pinned server.
    user=credentials['username']; account_password=credentials['password']
    if not sql(f"SELECT id FROM account WHERE username='{user}'",'wowx_logon').strip():
        salt=secrets.token_bytes(32)
        identity=hashlib.sha1((user+':'+account_password).encode()).digest()
        exponent=int.from_bytes(hashlib.sha1(salt+identity).digest(),'little')
        modulus=int('894B645E89E1535BBDAD5B8B290650530801B18EBFBF5E8FAB3C82872A3E9BB7',16)
        verifier=pow(7,exponent,modulus)
        sql(f"INSERT INTO account(username,s,v,gmlevel) VALUES ('{user}','{int.from_bytes(salt,'little'):X}','{verifier:X}',0)",'wowx_logon')
    sql("INSERT INTO realmlist(id,name,address,localAddress,localSubnetMask,port,realmflags,gamebuild_min,gamebuild_max,realmbuilds) VALUES (1,'WOWX Local','10.0.2.2','10.0.2.2','255.255.255.255',8086,0,5875,5875,'5875') ON DUPLICATE KEY UPDATE address=VALUES(address),localAddress=VALUES(localAddress),port=VALUES(port),realmflags=0",'wowx_logon')
    common={'BindIP':'"127.0.0.1"','LogsDir':'"'+str(SERVER/'logs')+'"','WaitAtStartupError':'0'}
    configure(SERVER/'etc/realmd.conf.dist',SERVER/'etc/realmd.conf',common|{
        'LoginDatabaseInfo':f'"127.0.0.1;3306;wowx_dev;{password};wowx_logon"',
        'RealmServerPort':'3725','StrictVersionCheck':'0'})
    world=common|{'DataDir':'"'+str(SERVER/'data')+'"','WorldServerPort':'8086','Console.Enable':'0'}
    for kind, key in [('logon','Login'),('world','World'),('characters','Character'),('logs','Logs')]:
        world[key+'Database.Info']=f'"127.0.0.1;3306;wowx_dev;{password};wowx_{kind}"'
    configure(SERVER/'etc/mangosd.conf.dist',SERVER/'etc/mangosd.conf',world)
    print('Local databases, disposable account and loopback server configuration ready.')

if __name__=='__main__':
    main()
