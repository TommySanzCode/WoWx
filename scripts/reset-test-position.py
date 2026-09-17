"""WSL root: reset only the offline, disposable WOWXTEST/Xboxer test character."""
import json
import argparse
import subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
credentials=json.loads((root/'server/local-credentials.json').read_text())
if credentials['username']!='WOWXTEST':raise RuntimeError('Unexpected disposable account identity')
p=argparse.ArgumentParser();p.add_argument('--fixture',choices=('spawn','quest','combat','turnin','kobolds','boundary','vendor'),default='spawn');p.add_argument('--reset-hearthstone',action='store_true');args=p.parse_args()
# Quest fixture is on the ground beside Deputy Willem, observed in server records.
x,y,z,o={'vendor':(-8901.59,-116.5,82.0314,1.570796),'spawn':(-8949.95,-132.493,83.5312,.15),'quest':(-8933.54,-139.523,83.447,1.570796),'combat':(-8981.5,-149.461,82.0318,3.14159),'turnin':(-8904.8,-161.5,82.0223,5.95),'kobolds':(-8803,-177,81.6547,0),'boundary':(-9048,-160,84.439552,3.14159265)}[args.fixture]
statement=f"""UPDATE wowx_characters.characters AS c JOIN wowx_logon.account AS a ON c.account=a.id
SET c.map=0,c.position_x={x},c.position_y={y},c.position_z={z},c.orientation={o}
WHERE a.username='WOWXTEST' AND c.name='Xboxer' AND c.online=0;
SELECT COUNT(*) FROM wowx_characters.characters AS c JOIN wowx_logon.account AS a ON c.account=a.id
WHERE a.username='WOWXTEST' AND c.name='Xboxer' AND c.online=0 AND c.map=0
AND ABS(c.position_x-({x}))<0.01 AND ABS(c.position_y-({y}))<0.01
AND ABS(c.position_z-({z}))<0.01 AND ABS(c.orientation-({o}))<0.001;"""
result=subprocess.run(['mariadb','--batch','--skip-column-names'],input=statement,text=True,capture_output=True,check=True)
if result.stdout.strip()!='1':raise RuntimeError('Offline disposable test character was not found or reset')
if args.reset_hearthstone:
    statement="""DELETE cd FROM wowx_characters.character_spell_cooldown cd
JOIN wowx_characters.characters c ON cd.guid=c.guid JOIN wowx_logon.account a ON c.account=a.id
WHERE a.username='WOWXTEST' AND c.name='Xboxer' AND c.online=0 AND cd.spell=8690 AND cd.item_id=6948;"""
    result=subprocess.run(['mariadb','--batch','wowx_characters'],input=statement,text=True,capture_output=True)
    if result.returncode:raise RuntimeError('Could not clear hearthstone fixture cooldown: '+result.stderr.strip())
    print('Cleared only the offline test character hearthstone cooldown for repeatable testing.')
print(f'Reset offline Xboxer to Northshire {args.fixture} fixture.')
