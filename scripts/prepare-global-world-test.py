"""Freeze a credential-free global-WMO routing fixture from verified private packs."""
import argparse,hashlib,json,math,shutil,struct,subprocess,sys
from pathlib import Path
from workspace_paths import xiso_tool
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from world_index import encode
from material_probe import pack as material_probe
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
p.add_argument('--maps',nargs='+',type=int,required=True)
p.add_argument('--material-probe',action='store_true',help='Include synthetic map 999 material-state probe')
p.add_argument('--material-motion',action='store_true',help='Animate colour, opacity and UVs on the material probe')
p.add_argument('--vertex-lighting',action='store_true',help='Exercise RGBA8 vertex lighting and baked-light state')
p.add_argument('--liquids',action='store_true',help='Accept original WMO liquid draw/texture-frame submissions')
p.add_argument('--environment-points',type=Path,help='Original-metadata camera cases; no account or input')
p.add_argument('--terrain',action='store_true',help='Use prepared terrain regions with liquid/environment probes')
p.add_argument('--point',action='append',default=[],help='Override diagnostic point MAP:X:Y:Z; verify collision on host first')
p.add_argument('--xiso',type=Path,help='extract-xiso executable; otherwise use local configuration or PATH')
a=p.parse_args();a.xiso=xiso_tool(a.xiso);folder=a.output.resolve();source=a.source.resolve()
if a.material_motion and not a.material_probe:raise SystemExit('Material motion needs --material-probe')
if a.vertex_lighting and not a.material_probe:raise SystemExit('Vertex lighting needs --material-probe')
if a.terrain and (not a.liquids or not a.environment_points or a.material_probe or a.point):raise SystemExit('Terrain inspection requires independent liquid/environment cases')
if folder.exists() or not folder.is_relative_to(ROOT/'build'):raise SystemExit('Use a fresh workspace build folder')
maps=sorted(set(a.maps))
if not 0<len(maps)<=32:raise SystemExit('Use 1-32 unique maps')
overrides={}
for text in a.point:
    fields=text.split(':')
    if len(fields)!=4:raise SystemExit('Point must be MAP:X:Y:Z')
    map_id=int(fields[0]);point=tuple(float(x) for x in fields[1:])
    if map_id not in maps or map_id in overrides or any(not math.isfinite(x) or abs(x)>=18000 for x in point):raise SystemExit('Invalid/duplicate point override')
    overrides[map_id]=point
def digest(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
normal=json.loads((ROOT/'build/evidence/build-receipt.json').read_text())
rows=[];points=[];dependencies={}
selected=[(m,f'G{m:03}.WXP') for m in maps]
if a.terrain:
    report=json.loads((source/'report.json').read_text())
    selected=[(r['map'],r['file']) for r in report['tiles'] if r['map'] in maps and r.get('kind')==0 and r['status']!='failed']
    if not selected or {m for m,n in selected}!=set(maps):raise SystemExit('Missing selected terrain maps')
for map_id,name in selected:
    path=source/name
    r=json.loads((source/(name+'.json')).read_text())
    coverage=source/(name+'.coverage.json')
    if r['map']!=map_id or r.get('kind')!=(0 if a.terrain else 1) or r['status'] not in ('converted','revalidated') or r['file']!=name or path.stat().st_size!=r['bytes'] or digest(path)!=r['sha256'] or digest(coverage)!=r['coverage_sha256']:raise SystemExit('World pack receipt mismatch: '+name)
    with path.open('rb') as f:header=f.read(32)
    magic,version,count,stride,x,y,z,size=struct.unpack('<4sIII3fI',header)
    if magic!=b'WXP1' or version not in (5,6,7,8,9,10) or not count or stride!=64 or size!=r['bytes']:raise SystemExit('Invalid global pack header')
    rows.append(r);points.append((map_id,*overrides.get(map_id,(x,y,z))))
    for dependency in (path,source/(name+'.json'),coverage,source/'source.json'):
        dependencies[dependency.relative_to(ROOT).as_posix()]={'bytes':dependency.stat().st_size,'sha256':digest(dependency)}
environment=None
if a.environment_points:
    if a.material_probe or a.point:raise SystemExit('Use an independent environment batch')
    environment=json.loads(a.environment_points.read_text())
    cases=environment['cases']
    if not 0<len(cases)<=32 or {x['map'] for x in cases}!=set(maps):raise SystemExit('Environment case/map mismatch')
    for path,expected_hash in environment['dependencies'].items():
        if digest(Path(path))!=expected_hash:raise SystemExit('Environment source mismatch')
    points=[(x['map'],*x['point']) for x in cases]
    dependencies[a.environment_points.resolve().relative_to(ROOT).as_posix()]={'bytes':a.environment_points.stat().st_size,'sha256':digest(a.environment_points)}
names=['default.xbe','FONT.BIN','INTERFACE.WUI','COOLDOWN.WCD','PORTRAIT.WPT']
if environment:names.append('LIGHT.WLF')
for name in names:
    if digest(ROOT/'build/xbox'/name)!=normal['outputs']['build/xbox/'+name]['sha256']:raise SystemExit('Native receipt mismatch: '+name)
disc=folder/'disc';disc.mkdir(parents=True)
for name in names:shutil.copy2(ROOT/'build/xbox'/name,disc/name)
for r in rows:shutil.copy2(source/r['file'],disc/r['file'])
if a.material_probe:
    if 999 in maps or len(maps)>=32:raise SystemExit('Material probe needs its own fixture-only map 999 slot')
    blob=material_probe(a.material_motion,a.vertex_lighting);(disc/'G999.WXP').write_bytes(blob);(disc/'MATERIAL.BIN').write_bytes(b'WMP1')
    subprocess.run([str(ROOT/'build/host/wowx_packcheck.exe'),str(disc/'G999.WXP')],check=True)
    rows.append({'map':999,'x':0,'y':0,'kind':1,'file':'G999.WXP','bytes':len(blob)})
    points.append((999,0,0,0));maps.append(999)
(disc/'world.wxi').write_bytes(encode(rows))
(disc/'GLOBAL.BIN').write_bytes(struct.pack('<4sII',b'WGM1',3 if a.terrain else 2 if environment else 1,len(points))+b''.join(struct.pack('<I3f',*x) for x in points))
if environment:(disc/'ENVIRON.BIN').write_bytes(struct.pack('<4sI',b'WEN1',len(cases))+b''.join(struct.pack('<IIf',x['environment'],x['liquid_type'],x['depth']) for x in cases))
(disc/'PTTEST.BIN').write_bytes(b'WXPF0013')
if a.liquids:(disc/'LIQUID.BIN').write_bytes(b'WQL1')
subprocess.run([str(a.xiso),'-c',str(disc),str(folder/'portrait.iso')],check=True,stdout=subprocess.DEVNULL)
if a.liquids and not environment:raise SystemExit('Liquid acceptance needs original environment camera cases')
normal['outputs']={path.relative_to(ROOT).as_posix():{'bytes':path.stat().st_size,'sha256':digest(path)} for path in [*disc.iterdir(),folder/'portrait.iso']}
normal['asset_dependencies']=dependencies
normal['scope']='Offline map-wide WMO selection, bounded payload streaming, resident collision and renderer. Synthetic map switches at inspection points; no traversal, account, original UI, controller or hardware acceptance.'
if environment:normal['scope']='Offline original-WMO camera environment classification and fog/sky selection. Synthetic camera positions from original bounds and liquid grids; no water surfaces, movement, swimming, account, gameplay or hardware acceptance.'
if a.liquids:normal['scope']='Offline original WMO liquid surface geometry, texture-frame submissions and camera environment transitions. Synthetic camera positions; no swimming, live combat, physical controller/hardware or full water appearance acceptance.'
if a.terrain:normal['scope']='Offline original terrain MCLQ and placed WMO liquid surfaces, camera classification and terrain region streaming. Synthetic probes, no walked route, swimming, account, physical controller/hardware or full appearance acceptance.'
if a.material_probe:normal['scope']+=' Map 999 is a synthetic material probe, not Vanilla content.'
(folder/'receipt.json').write_text(json.dumps(normal,indent=2)+'\n')
(folder/'maps.json').write_text(json.dumps({'points':points,'maps':maps,'terrain':a.terrain,'regions':rows,'environment':environment,'liquids':a.liquids,'material_probe':a.material_probe,'material_motion':a.material_motion,'vertex_lighting':a.vertex_lighting,'scope':normal['scope']},indent=2)+'\n')
print(folder/'portrait.iso')
