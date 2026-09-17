"""Change only the private local vMaNGOS account's SRP credentials (run in WSL).

Uses the pinned AccountMgr::ChangePassword/SRP6 algorithm. Backups stay under
the ignored server directory. No world login, character edit or server restart.
"""
import argparse
from datetime import datetime, timezone
import getpass
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

def sql(statement):
    p = subprocess.run(['mariadb','--batch','--raw','--skip-column-names'],
                       input=statement.encode(),capture_output=True)
    if p.returncode:
        raise RuntimeError('Local database operation failed; database diagnostics were suppressed to protect credentials')
    return p.stdout.decode().strip()

def character_digest(account_id):
    # These persisted records must remain byte-for-byte unchanged by a password reset.
    state = sql(f'SELECT * FROM wowx_characters.characters WHERE account={account_id} ORDER BY guid;')
    return hashlib.sha256(state.encode()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stdin',action='store_true',help='Read the new password from standard input')
    args = parser.parse_args()
    password = (sys.stdin.readline().rstrip('\r\n') if args.stdin else getpass.getpass('New local game password: ')).upper()
    if not password.isascii() or not 1 <= len(password) <= 16 or any(ord(c)<32 for c in password):
        parser.error('The native login requires 1-16 printable ASCII characters')
    path = ROOT/'server/local-credentials.json'
    original = path.read_bytes()
    credentials = json.loads(original)
    username = credentials['username'].upper()
    if not re.fullmatch(r'[A-Z0-9_]{1,16}',username):
        parser.error('Unexpected local account name')
    rows = sql(f"SELECT id,username,s,v FROM wowx_logon.account WHERE username='{username}';").splitlines()
    if len(rows) != 1:
        raise RuntimeError('Expected exactly one existing local account')
    account_id, database_username, old_s, old_v = rows[0].split('\t')
    if not account_id.isdecimal() or database_username != username or not all(re.fullmatch(r'[A-Fa-f0-9]+',v) for v in (old_s,old_v)):
        raise RuntimeError('Unexpected account identity or SRP fields')
    before = character_digest(account_id)
    backup = ROOT/'server/private-backups'/datetime.now(timezone.utc).strftime('password-%Y%m%dT%H%M%S%fZ')
    backup.mkdir(parents=True,mode=0o700)
    (backup/'local-credentials.json').write_bytes(original)
    (backup/'account-auth.json').write_text(json.dumps(dict(id=int(account_id),username=username,s=old_s,v=old_v)))
    with (backup/'characters.sql').open('wb') as stream:
        p = subprocess.run(['mariadb-dump','--lock-all-tables','--hex-blob','wowx_characters'],stdout=stream,stderr=subprocess.PIPE)
    if p.returncode:
        raise RuntimeError('Character backup failed; password was not changed')
    salt = bytearray(secrets.token_bytes(32))
    salt[-1] |= 0x80  # Match the server's 256-bit BigNumber salt width.
    identity = hashlib.sha1((username+':'+password).encode('ascii')).digest()
    exponent = int.from_bytes(hashlib.sha1(salt+identity).digest(),'little')
    modulus = int('894B645E89E1535BBDAD5B8B290650530801B18EBFBF5E8FAB3C82872A3E9BB7',16)
    new_s = f'{int.from_bytes(salt,"little"):X}'
    new_v = f'{pow(7,exponent,modulus):X}'
    changed = sql(f"UPDATE wowx_logon.account SET s='{new_s}',v='{new_v}' WHERE id={account_id} AND username='{username}' AND s='{old_s}' AND v='{old_v}'; SELECT ROW_COUNT();")
    if changed != '1':
        raise RuntimeError('Account changed concurrently; private credential file was not modified')
    credentials['password'] = password
    temporary = backup/'updated-credentials.json'
    temporary.write_text(json.dumps(credentials,indent=2)+'\n')
    os.chmod(temporary,0o600)
    os.replace(temporary,path)
    # A private, short-lived binary for the existing authentication-only probe.
    probe = backup/'auth-probe.bin'
    probe.write_bytes(struct.pack('<64s32s32sI',b'127.0.0.1',username.encode(),password.encode(),3725))
    for item in backup.iterdir():
        os.chmod(item,0o600)
    after = character_digest(account_id)
    if before != after:
        raise RuntimeError('Character records changed during the reset; inspect the private backup before gameplay')
    report = dict(account_id=int(account_id),username=username,password_changed=True,
                  character_records_unchanged=True,character_sha256=before,
                  private_backup=str(backup),scope='SRP credentials changed; no world login or character edits. Authentication handshake still required.')
    (ROOT/'build/evidence/hardware-password-change.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Updated local account {username}; character records unchanged. Private backup and auth probe: {backup}')

if __name__ == '__main__':
    main()
