"""Create an isolated, credential-free native portrait fixture disc."""
import argparse,hashlib,json,shutil,subprocess
from pathlib import Path
from workspace_paths import xiso_tool
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--action-icons',action='store_true')
p.add_argument('--cooldowns',action='store_true')
p.add_argument('--lighting',action='store_true')
p.add_argument('--weather',action='store_true')
p.add_argument('--fog',action='store_true')
p.add_argument('--streaming',action='store_true')
p.add_argument('--stream-actors',action='store_true')
p.add_argument('--appearance-stream',action='store_true')
p.add_argument('--profiles',action='store_true')
p.add_argument('--action-feedback',action='store_true')
p.add_argument('--global-cooldowns',action='store_true')
p.add_argument('--xiso',type=Path,help='extract-xiso executable; otherwise use local configuration or PATH')
a=p.parse_args();a.xiso=xiso_tool(a.xiso);folder=a.output.resolve()
if not folder.is_relative_to(ROOT/'build') or folder.exists():raise SystemExit('Use a new directory inside build; previous captures are preserved.')
source=ROOT/'build/xbox';receipt=json.loads((ROOT/'build/evidence/build-receipt.json').read_text())
names=['default.xbe','actors.wxp','FONT.BIN','INTERFACE.WUI','PORTRAIT.WPT','COOLDOWN.WCD']
if a.profiles:names+=['OUTFITS.WXO']
if a.fog or a.lighting or a.weather:names+=['world.wxp']
if a.lighting or a.weather:names+=['LIGHT.WLF']
if a.streaming or a.stream_actors or a.appearance_stream:names+=['world.wxi']+sorted(x.name for x in source.glob('M*.WXP'))
if a.action_icons or a.cooldowns or a.global_cooldowns or a.action_feedback:names+=['ICONS.WIC','SPELLS.WXS']
names+=sorted(x.name for pattern in ('A*.WXP','A*.WXA','L*.WXL') for x in source.glob(pattern))
def digest(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
for name in names:
    record=receipt['outputs']['build/xbox/'+name]
    if digest(source/name)!=record['sha256']:raise SystemExit('Build receipt mismatch: '+name)
disc=folder/'disc';disc.mkdir(parents=True)
for name in names:shutil.copy2(source/name,disc/name)
(disc/'PTTEST.BIN').write_bytes(b'WXPF0012' if a.profiles else b'WXPF0011' if a.appearance_stream else b'WXPF0010' if a.weather else b'WXPF0009' if a.lighting else b'WXPF0008' if a.stream_actors else b'WXPF0007' if a.streaming else b'WXPF0006' if a.fog else b'WXPF0005' if a.action_feedback else b'WXPF0004' if a.global_cooldowns else b'WXPF0003' if a.cooldowns else b'WXPF0002' if a.action_icons else b'WXPF0001')
subprocess.run([str(a.xiso),'-c',str(disc),str(folder/'portrait.iso')],check=True,stdout=subprocess.DEVNULL)
outputs={str(path.relative_to(ROOT).as_posix()):{'bytes':path.stat().st_size,'sha256':digest(path)} for path in [*disc.iterdir(),folder/'portrait.iso']}
receipt.update(outputs=outputs,scope='Offline native portrait renderer fixture. No credentials, account authentication, world connection, gameplay or physical input acceptance.')
if a.lighting:receipt['scope']='Offline resident Northshire fog/material/sky colours, independent portrait, synthetic clock/conditions and scaled actors. No full sky/weather, server, traversal, pad or hardware acceptance.'
if a.weather:receipt['scope']='Offline resident Northshire with synthetic Vanilla weather packets through production parser/presentation, including smooth/instant changes and explicit environment overrides. No precipitation/audio, shared account, traversal, input or hardware acceptance.'
if a.streaming:receipt['scope']='Offline real-asset streaming/collision routes with explicit relocation between routes. No credentials, account connection, injected pad or physical controller acceptance.'
if a.stream_actors:receipt['scope']='Offline real-asset world/NPC/avatar streaming, synthetic units over deterministic collision routes with explicit phase-4 relocation. No credentials, server account, combat or pad-input acceptance.'
if a.appearance_stream:receipt['scope']='Offline world/NPC/avatar route with staged customization/equipment changes and atomic-publication assertions. No credentials, account, live gameplay, input or hardware acceptance.'
if a.profiles:receipt['scope']='Offline production asynchronous profile selection for all sixteen race/sex profiles, with cancellation/missing-profile/recovery scenarios. Real outfits/assets; no account, gameplay, controller or hardware acceptance.'
(folder/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(folder/'portrait.iso')
