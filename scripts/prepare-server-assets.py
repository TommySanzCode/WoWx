"""Prepare vMaNGOS data in WSL's native filesystem, then install a tar archive.

Run on Windows. --verify checks the existing Northshire server data without rebuilding.
The default generation covers all maps/vmaps and the initial Northshire mmap tile.
"""
import argparse
import json
import subprocess
import tarfile
from pathlib import Path
from workspace_paths import game_data

ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/'server/data'

def verify():
    required=['5875/dbc/Map.dbc','5875/dbc/AreaTable.dbc','5875/dbc/ChrRaces.dbc',
              'maps/0004832.map','vmaps/000.vmtree','mmaps/000.mmap','mmaps/0004832.mmtile']
    missing=[p for p in required if not (DATA/p).is_file() or not (DATA/p).stat().st_size]
    if missing:raise RuntimeError('Missing server assets: '+', '.join(missing))
    counts={folder:sum(1 for p in (DATA/folder).rglob('*') if p.is_file())
            for folder in ('5875/dbc','maps','vmaps','mmaps','Cameras')}
    print(json.dumps({'required_files_present':True,'counts':counts,
                      'pathfinding_scope':'Northshire tile 32,48; other mmap tiles are not generated'},indent=2))

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true')
    parser.add_argument('--installation',type=Path,default=game_data().parent)
    parser.add_argument('--distribution',default='Ubuntu');parser.add_argument('--threads',type=int,default=6)
    args=parser.parse_args()
    if args.verify:verify();return
    if not 1<=args.threads<=32:parser.error('Threads must be 1..32')
    def wsl(*command,capture=False):
        return subprocess.run(['wsl','-d',args.distribution,'--exec',*map(str,command)],check=True,
                              capture_output=capture,text=True)
    def linux(path):return wsl('wslpath','-a',str(path.resolve()),capture=True).stdout.strip()
    root=linux(ROOT);installation=linux(args.installation)
    wsl('python3',root+'/scripts/prepare-server-assets-wsl.py',root,installation,str(args.threads))
    archive=ROOT/'build/server-assets.tar'
    DATA.mkdir(parents=True,exist_ok=True)
    with tarfile.open(archive) as source:
        # Reject links and unexpected roots; never write outside this generated data folder.
        members=source.getmembers()
        for member in members:
            path=Path(member.name)
            if path.is_absolute() or '..' in path.parts or member.issym() or member.islnk():
                raise ValueError('Unsafe generated archive member')
            if not path.parts or path.parts[0] not in {'5875','maps','vmaps','mmaps','Cameras'}:
                raise ValueError('Unexpected generated data directory')
        source.extractall(DATA,members=members,filter='data')
    verify()
if __name__=='__main__':main()
