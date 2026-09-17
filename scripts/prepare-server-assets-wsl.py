"""Internal WSL extraction stage. All random-I/O intermediates stay on Linux storage."""
import hashlib
import json
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path

root=Path(sys.argv[1]).resolve();installation=Path(sys.argv[2]).resolve();threads=int(sys.argv[3])
if not 1<=threads<=32:raise ValueError('Invalid thread count')
tools=root/'server/bin/Extractors'
for name in ('MapExtractor','VMapExtractor','VMapAssembler','MoveMapGenerator'):
    if not (tools/name).is_file():raise RuntimeError('Build vMaNGOS extractors first: '+name)
archives=sorted((installation/'Data').glob('*.MPQ'))+sorted((installation/'Data').glob('*.mpq'))
archives=sorted(set(archives))
if not archives:raise RuntimeError('No MPQ archives in supplied installation')
identity={'archives':[(p.name,p.stat().st_size,p.stat().st_mtime_ns) for p in archives],
          'tools':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in tools.iterdir() if p.is_file()}}
key=hashlib.sha256(json.dumps(identity,sort_keys=True).encode()).hexdigest()[:16]
work=Path.home()/'.cache/wowx/server-assets'/key
work.mkdir(parents=True,exist_ok=True)
(work/'identity.json').write_text(json.dumps(identity,indent=2)+'\n')
data=work/'data';data.mkdir(exist_ok=True)

def run_stage(name,command,cwd):
    marker=work/(name+'.complete')
    if marker.exists():print('Reusing completed stage:',name,flush=True);return
    print('Extracting:',name,flush=True)
    with (work/(name+'.log')).open('w') as log:
        subprocess.run(list(map(str,command)),cwd=cwd,stdout=log,stderr=subprocess.STDOUT,check=True)
    marker.touch()

run_stage('maps',[tools/'MapExtractor','--silent','-i',installation,'-o',data,'-f','0','-h','0'],work)
(data/'5875').mkdir(exist_ok=True)
shutil.copytree(data/'dbc',data/'5875/dbc',dirs_exist_ok=True)
run_stage('vmap-raw',[tools/'VMapExtractor','--silent','-d',installation/'Data'],work)
(data/'vmaps').mkdir(exist_ok=True)
run_stage('vmap-assemble',[tools/'VMapAssembler','--silent',work/'Buildings',data/'vmaps'],work)
(data/'mmaps').mkdir(exist_ok=True)
run_stage('mmap-northshire',[tools/'MoveMapGenerator','0','--tile','32,48','--silent','--threads',threads,
                           '--offMeshInput',tools/'offmesh.txt','--configInputPath',tools/'config.json'],data)
archive=root/'build/server-assets.tar';archive.parent.mkdir(exist_ok=True)
with tarfile.open(archive,'w') as target:
    for folder in ('5875','maps','vmaps','mmaps','Cameras'):target.add(data/folder,arcname=folder)
print('Server data archive:',archive,flush=True)
