"""Stage a verified world batch into the native disc directory."""
import argparse
import hashlib
import json
import shutil
import socket
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools'))
from world_index import decode as decode_index
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=Path, default=root / 'build/worldpacks')
source = parser.parse_args().source.resolve()
target = root / 'build/xbox'
with socket.socket() as probe:
    probe.settimeout(.2)
    if probe.connect_ex(('127.0.0.1', 4444)) == 0:
        raise SystemExit('Stop the owned xemu instance before staging its disc files.')
index = (source / 'world.wxi').read_bytes()
tiles = []
for row in decode_index(index):
    filename,size=row['file'],row['bytes']
    path = source / filename
    receipt = json.loads(path.with_suffix('.WXP.json').read_text(encoding='utf-8'))
    with path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    if path.stat().st_size != size or receipt['sha256'] != digest:
        raise SystemExit(f'Tile hash/size mismatch: {filename}')
    tiles.append(path)
for path in tiles:
    shutil.copy2(path, target / path.name)
# Publish the index only after every referenced file is complete.
pending = target / 'world.wxi.part'
pending.write_bytes(index)
pending.replace(target / 'world.wxi')
print(f'Staged {len(tiles)} verified world region packs.')
