"""Discover and batch-convert Vanilla map tiles; resume only verified identical work.

This prepares assets. It does not claim that every renderer feature or runtime
map transition is implemented. Failures remain visible in the batch report.
"""
import argparse
import concurrent.futures
import hashlib
import json
import shutil
import struct
import subprocess
import sys
from pathlib import Path
from workspace_paths import game_data

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from world_index import encode as encode_index,filename as region_filename

def sha(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(4 * 1024 * 1024), b''):
            value.update(chunk)
    return value.hexdigest()

def atomic_json(path, value):
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')
    temporary.replace(path)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--data', type=Path, default=game_data())
    parser.add_argument('--output', type=Path, default=ROOT / 'build/worldpacks')
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument('--tile', action='append', help='Repeatable MAP:X:Y coordinates from the discovered catalog')
    selection.add_argument('--maps', type=int, nargs='+', help='All regions in these map IDs, including global WMO maps')
    selection.add_argument('--world-models',action='store_true',help='Every available global WMO map, including developer maps')
    selection.add_argument('--all', action='store_true', help='All discovered maps, including developer maps')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--reserve-gib', type=int, default=32)
    args = parser.parse_args()
    if not 1 <= args.jobs <= 4 or args.reserve_gib < 8:
        parser.error('Use 1-4 workers and retain at least 8 GiB host disk reserve')
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=True)
    logs = output / 'logs'; logs.mkdir(exist_ok=True)
    cooker = ROOT / 'build/host/wowx_assetc.exe'
    verifier = ROOT / 'build/host/wowx_packcheck.exe'
    with (logs / 'catalog.log').open('w', encoding='utf-8') as log:
        subprocess.run([str(cooker), '--catalog', str(args.data), str(output / 'catalog.json')], stdout=log, stderr=subprocess.STDOUT, check=True)
    catalog = json.loads((output / 'catalog.json').read_text(encoding='utf-8'))
    available = {}
    for entry in catalog['maps']:
        if entry.get('available') and entry.get('valid',True):
            if entry.get('kind')=='terrain':
                for x,y in entry['tiles']:available[(entry['id'],x,y)]=(entry['directory'],0)
            elif entry.get('kind')=='world_model':available[(entry['id'],0,0)]=(entry['directory'],1)
    if args.tile:
        try: wanted = {tuple(map(int, tile.split(':'))) for tile in args.tile}
        except ValueError: parser.error('Tile coordinates must be MAP:X:Y')
        if any(len(key) != 3 or key not in available or available[key][1] for key in wanted):
            parser.error('A selected tile is absent from the game map catalog')
    elif args.maps:
        if set(args.maps) - {key[0] for key in available}: parser.error('A selected map has no valid available regions')
        wanted = {key for key in available if key[0] in args.maps}
    elif args.world_models:wanted={key for key in available if available[key][1]}
    else: wanted = set(available)
    if not wanted: parser.error('No world regions selected')
    # The cooker's identity pins its source/dependencies. Full archive hashes
    # ensure an overlay change cannot silently reuse an old converted tile.
    print(f'Fingerprinting source archives for {len(wanted)} tile jobs...', flush=True)
    archives = {p.name: sha(p) for p in sorted(args.data.glob('*.MPQ'))}
    recipe = {'version': 2, 'cooker_sha256': sha(cooker), 'verifier_sha256': sha(verifier), 'archives': archives}
    fingerprint = hashlib.sha256(json.dumps(recipe, sort_keys=True).encode()).hexdigest()
    previous_recipe = json.loads((output / 'source.json').read_text(encoding='utf-8')) if (output / 'source.json').exists() else {}
    previous_fingerprint = hashlib.sha256(json.dumps(previous_recipe, sort_keys=True).encode()).hexdigest()
    same_conversion = all(previous_recipe.get(k) == recipe[k] for k in ('version', 'cooker_sha256', 'archives'))
    atomic_json(output / 'source.json', recipe)

    def convert(key):
        map_id, x, y = key
        if not 0 <= map_id <= 999: raise ValueError('Map ID exceeds short filename format')
        directory,kind=available[key]
        name = region_filename(map_id,x,y,kind)
        target = output / name; receipt = output / (name + '.json')
        result = {'map': map_id, 'x': x, 'y': y, 'file': name, 'kind':kind, 'fingerprint': fingerprint}
        try:
            if receipt.exists() and target.exists():
                old = json.loads(receipt.read_text(encoding='utf-8'))
                coverage_ok=(output/(name+'.coverage.json')).exists() and old.get('coverage_sha256')==sha(output/(name+'.coverage.json'))
                if coverage_ok and old.get('fingerprint') == fingerprint and old.get('sha256') == sha(target):
                    return {**old, 'status': 'reused'}
                if coverage_ok and same_conversion and old.get('fingerprint') == previous_fingerprint and old.get('sha256') == sha(target):
                    with (logs / (name + '.verify.log')).open('w', encoding='utf-8') as log:
                        subprocess.run([str(verifier), str(target)], stdout=log, stderr=subprocess.STDOUT, check=True)
                    result = {**old, 'fingerprint': fingerprint, 'status': 'revalidated'}
                    atomic_json(receipt, result)
                    return result
            # A tile is bounded below 1 GiB. Leave room for every active worker.
            if shutil.disk_usage(output).free < (args.reserve_gib + args.jobs) * 1024**3:
                raise RuntimeError('Host disk reserve would be exhausted')
            temporary = output / (name + '.part')
            with (logs / (name + '.log')).open('w', encoding='utf-8') as log:
                command=[str(cooker),'--world-model' if kind else '--tile',str(args.data),str(temporary),str(map_id),directory]
                if not kind:command.extend([str(x),str(y)])
                subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
                subprocess.run([str(verifier), str(temporary)], stdout=log, stderr=subprocess.STDOUT, check=True)
            size = temporary.stat().st_size
            if not 32 < size < 1024**3: raise RuntimeError('Converted tile exceeds pack bounds')
            result.update(status='converted', bytes=size, sha256=sha(temporary))
            coverage=Path(str(temporary)+'.coverage.json');result['coverage_sha256']=sha(coverage)
            coverage.replace(output/(name+'.coverage.json'))
            temporary.replace(target)
            atomic_json(receipt, result)
            return result
        except Exception as error:
            return {**result, 'status': 'failed', 'error': str(error)}

    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {pool.submit(convert, key): key for key in sorted(wanted)}
        for future in concurrent.futures.as_completed(futures):
            result = future.result(); results.append(result)
            print(f'{len(results)}/{len(wanted)} {result["file"]}: {result["status"]}', flush=True)
            atomic_json(output / 'report.json', {'selected': len(wanted), 'tiles': sorted(results, key=lambda r: (r['map'], r['x'], r['y']))})
    ready = sorted((r for r in results if r['status'] != 'failed'), key=lambda r: (r['map'], r['x'], r['y']))
    # Small native index: 16-byte header, 32-byte cells. It lists only verified
    # tiles in this batch; incomplete/failed data never enters the runtime index.
    # An empty failed batch invalidates a previous index in this output folder.
    index=encode_index(ready) if ready else struct.pack('<4sIII',b'WXI1',2,0,32)
    pending = output / 'world.wxi.tmp'; pending.write_bytes(index); pending.replace(output / 'world.wxi')
    if len(ready) != len(wanted): raise SystemExit('Some tiles failed; inspect report.json and per-tile logs.')
    print(f'Verified {len(ready)} tiles; {sum(r["bytes"] for r in ready):,} bytes.', flush=True)

if __name__ == '__main__': main()
