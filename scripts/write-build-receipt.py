"""Record source/output identities without copying credentials or game content."""
import hashlib
import json
from datetime import datetime,timezone
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def digest(path):
    value=hashlib.sha256()
    with path.open('rb') as source:
        for block in iter(lambda:source.read(1024*1024),b''):value.update(block)
    return value.hexdigest()
source={}
for folder in ('src','include','tools','scripts','tests','third_party'):
    for path in sorted((ROOT/folder).rglob('*')):
        if path.is_file() and path.suffix in ('.c','.cpp','.h','.hpp','.cg','.py','.sh','.ps1'):
            source[path.relative_to(ROOT).as_posix()]=digest(path)
for name in ('CMakeLists.txt','Makefile','config/dependencies.json','config/creatures.txt','config/player.txt','config/ui-requirements.txt'):source[name]=digest(ROOT/name)
outputs={}
for name in ('build/xbox/default.xbe','build/wowx.iso','build/xbox/world.wxp','build/xbox/actors.wxp','build/xbox/input.rpl','build/xbox/scenario.bin','build/xbox/CAMP.RTE','build/xbox/TESTCHAR.BIN','build/xbox/FONT.BIN','build/xbox/SOAK.BIN','build/xbox/MAPS.WMI','build/xbox/SPELLS.WXS','build/xbox/INTERFACE.WUI','build/xbox/LOGIN.BIN','build/xbox/TITLE.WXB','build/xbox/PORTRAIT.WPT','build/xbox/ICONS.WIC','build/xbox/COOLDOWN.WCD','build/xbox/LIGHT.WLF'):
    path=ROOT/name;outputs[name]={'bytes':path.stat().st_size,'sha256':digest(path)}
for path in sorted([*(ROOT/'build/xbox').glob('Z*.WMP'),*(ROOT/'build/xbox').glob('M*.WXP'),*(ROOT/'build/xbox').glob('G*.WXP'),*(ROOT/'build/xbox').glob('A*.WXP'),*(ROOT/'build/xbox').glob('A*.WXA'),*(ROOT/'build/xbox').glob('*.WXB'),*(ROOT/'build/xbox').glob('*.WXO'),*(ROOT/'build/xbox').glob('L*.WXL')]):
    outputs[path.relative_to(ROOT).as_posix()]={'bytes':path.stat().st_size,'sha256':digest(path)}
path=ROOT/'build/xbox/world.wxi'
if path.exists():outputs[path.relative_to(ROOT).as_posix()]={'bytes':path.stat().st_size,'sha256':digest(path)}
receipt={'created_utc':datetime.now(timezone.utc).isoformat(),'sources':source,'outputs':outputs,
         'dependencies':json.loads((ROOT/'config/dependencies.json').read_text()),
         'scope':'Development build; game assets and test credentials are private; full port incomplete'}
output=ROOT/'build/evidence/build-receipt.json';output.parent.mkdir(parents=True,exist_ok=True)
serialized=json.dumps(receipt,indent=2)+'\n';output.write_text(serialized)
archive=output.parent/'receipts';archive.mkdir(exist_ok=True)
(archive/(datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')+'.json')).write_text(serialized)
print(output)
