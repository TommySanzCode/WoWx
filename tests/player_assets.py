"""Exercise the actual supplied Vanilla archives and runtime pack validator."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--data',type=Path,required=True)
parser.add_argument('--modular',action='store_true')
args=parser.parse_args()
output=ROOT/('build/avatar-validation' if args.modular else 'build/player-validation')
output.mkdir(parents=True,exist_ok=True)
cases=[(f'race{race}-{sex}',f'{race} {sex} 0 0 0 0 0\n'+'0 0\n'*20) for race in range(1,9) for sex in range(2)]
cases.append(('equipped',(ROOT/'config/player.txt').read_text()))
cases.append(('human-alternate','1 0 1 1 1 1 1\n'+'0 0\n'*20))
results=[]
for name,profile in cases:
    source=output/(name+'.txt');source.write_text(profile)
    pack=output/(name+'.wxp')
    command=[str(ROOT/'build/host/wowx_assetc.exe'),'--avatar' if args.modular else '--player',str(args.data),str(pack),str(source)]
    result=subprocess.run(command,capture_output=True,text=True)
    (output/(name+'.log')).write_text(result.stdout+result.stderr)
    if result.returncode:
        print(name+': FAILED: '+result.stderr.strip(),flush=True)
        results.append(dict(name=name,passed=False,error=result.stderr.strip()))
        continue
    validator=[str(ROOT/'build/host/wowx_avatar_tests.exe'),str(pack),str(pack.with_suffix('.WXA'))] if args.modular else [str(ROOT/'build/host/wowx_packcheck.exe'),str(pack)]
    verification=subprocess.run(validator,capture_output=True,text=True)
    omissions=sorted(set(line for line in result.stdout.splitlines() if line.startswith('Optional character overlay absent:')))
    results.append(dict(name=name,passed=verification.returncode==0,bytes=pack.stat().st_size,optional_overlay_omissions=omissions,
                        sha256=hashlib.sha256(pack.read_bytes()).hexdigest(),verification=verification.stdout+verification.stderr))
    print(name+': '+verification.stdout.strip()+verification.stderr.strip(),flush=True)
valid='1 0 0 0 0 0 0\n'+'0 0\n'*20
invalid={'empty':'','race':'9'+valid[1:],'sex':valid.replace('1 0','1 2',1),
         'skin':valid.replace('1 0 0','1 0 256',1),'short':valid[:-4],
         'trailing':valid+'junk','equipment_type':valid[:-4]+'1542 29\n',
         'unpaired_equipment':valid[:-4]+'1542 0\n'}
for name,profile in invalid.items():
    source=output/('invalid-'+name+'.txt');source.write_text(profile)
    pack=output/'invalid-output.wxp';pack.write_bytes(b'preserve previous output')
    result=subprocess.run([str(ROOT/'build/host/wowx_assetc.exe'),'--avatar' if args.modular else '--player',str(args.data),str(pack),str(source)],capture_output=True,text=True)
    passed=result.returncode!=0 and pack.read_bytes()==b'preserve previous output'
    results.append(dict(name='invalid-'+name,passed=passed,error=result.stderr.strip()))
    print('invalid-'+name+': '+('rejected without changing output' if passed else 'FAILED'),flush=True)
(output/'results.json').write_text(json.dumps(results,indent=2)+'\n')
raise SystemExit(0 if all(row['passed'] for row in results) else 1)
