"""Stage a private HDD test from a verified checkpoint, without rebuilding it."""
import argparse
import hashlib
import ipaddress
import json
import secrets
import shutil
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from world_index import decode, encode

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--checkpoint', type=Path, required=True)
    parser.add_argument('--pc-ip', type=ipaddress.IPv4Address, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--scoped-native-check', type=Path,
                        help='Use a passing scoped native candidate instead of a gameplay checkpoint')
    parser.add_argument('--world-packs', type=Path, action='append', default=[],
                        help='Merge converted region packs, retaining every other installed region')
    args = parser.parse_args()
    checkpoint = args.checkpoint.resolve(strict=True)
    receipt = json.loads((checkpoint/'receipt.json').read_text())
    verification_path = args.scoped_native_check or checkpoint/'verification.json'
    verification = json.loads(verification_path.read_text())
    if not verification.get('passed'):
        parser.error('Checkpoint has no passing verification receipt')
    if args.scoped_native_check:
        capture = json.loads((checkpoint/'capture-receipt.json').read_text())
        captured = [v for k,v in capture['outputs'].items() if k.endswith('/default.xbe')]
        if captured != [receipt['outputs']['build/xbox/default.xbe']]:
            parser.error('Scoped capture executable does not match the candidate')
    if args.output.exists():
        parser.error('Choose a new output directory; hardware packages are not overwritten')
    sources = []
    for relative, identity in receipt['outputs'].items():
        path = Path(relative)
        if path.parent != Path('build/xbox'):
            continue
        source = checkpoint/'default.xbe' if path.name == 'default.xbe' else ROOT/path
        if source.stat().st_size != identity['bytes'] or digest(source) != identity['sha256']:
            parser.error(f'Checkpoint asset differs: {path.name}; do not mix builds')
        if len(path.name) > 42 or source.stat().st_size >= 1024**3:
            parser.error(f'Xbox filename/file size limit: {path.name}')
        sources.append((source, path.name))
    if not any(name == 'default.xbe' for _, name in sources):
        parser.error('Checkpoint has no executable')
    # Validate all overlays before creating the new package. The existing disc is untouched.
    overlays = {}
    regions = {r['file']:r for r in decode((ROOT/'build/xbox/world.wxi').read_bytes())}
    for folder in args.world_packs:
        report = json.loads((folder/'report.json').read_text())
        records = {r['file']:r for r in report['tiles'] if r.get('status') in ('converted','cached')}
        for row in decode((folder/'world.wxi').read_bytes()):
            name = row['file']
            source = folder/name
            expected = records.get(name)
            if not expected or source.stat().st_size != row['bytes'] or row['bytes'] != expected['bytes'] or digest(source) != expected['sha256']:
                parser.error(f'Converted pack identity differs: {source}')
            if name in overlays:
                parser.error(f'Duplicate overlay region: {name}')
            overlays[name] = source
            regions[name] = row
    world_index = encode(sorted(regions.values(),key=lambda r:(r['map'],r['x'],r['y'])))
    package = args.output/'WOWX'
    package.mkdir(parents=True)
    for source, name in sources:
        shutil.copyfile(source, package/name)
    for name, source in overlays.items():
        shutil.copyfile(source, package/name)
    if overlays:
        (package/'world.wxi').write_bytes(world_index)
    # Always physical controller, manual login, and no scenario commands.
    for name in ('scenario.bin','CAMP.RTE','TESTCHAR.BIN','FONT.BIN','SOAK.BIN'):
        (package/name).write_bytes(b'NONE')
    if any((package/name).exists() for name in ('PTTEST.BIN','GLOBAL.BIN','ENVIRON.BIN','LIQUID.BIN')):
        raise RuntimeError('An offline fixture marker was included in the hardware package')
    (package/'LOGIN.BIN').write_bytes(b'MENU')
    (package/'input.rpl').write_bytes(struct.pack('<4sII',b'WXR1',0,0))
    credentials = json.loads((ROOT/'server/local-credentials.json').read_text())
    username = credentials['username'].encode('ascii')
    if not 1 <= len(username) <= 16:
        parser.error('Development account name does not fit the login field')
    (package/'testauth.bin').write_bytes(struct.pack('<64s32s32sI',str(args.pc_ip).encode(),username,b'',3725))
    (package/'entropy.bin').write_bytes(secrets.token_bytes(4112))
    files = {p.name: {'bytes':p.stat().st_size,'sha256':digest(p)} for p in sorted(package.iterdir())}
    for source,name in sources:
        if name not in overlays and not (overlays and name == 'world.wxi') and name not in ('scenario.bin','CAMP.RTE','TESTCHAR.BIN','FONT.BIN','SOAK.BIN','LOGIN.BIN','input.rpl','testauth.bin','entropy.bin'):
            if files[name]['sha256'] != digest(source):
                raise RuntimeError(f'Copy verification failed: {name}')
    for name, source in overlays.items():
        if files[name]['sha256'] != digest(source):
            raise RuntimeError(f'Overlay copy verification failed: {name}')
    for row in decode((package/'world.wxi').read_bytes()):
        if files[row['file']]['bytes'] != row['bytes']:
            raise RuntimeError(f'Region index/file mismatch: {row["file"]}')
    scope = ('Exact executable from the passing scoped native capture; base assets match its normal build receipt. '
             'Merged converted regions are recorded separately. Combined live gameplay and physical hardware remain unverified.'
             if args.scoped_native_check or overlays else
             'Executable and game assets match the xemu-verified checkpoint. Physical hardware unverified; limited playable development client.')
    manifest = {'checkpoint':str(checkpoint),'pc_ip':str(args.pc_ip),'login_port':3725,'world_port':8086,
                'files':files,'bytes':sum(f['bytes'] for f in files.values()),
                'changes':'Manual input/menu; LAN login address; empty packaged password; fresh entropy.',
                'world_overlays':{name:str(p.resolve()) for name,p in overlays.items()},
                'region_count':len(regions),'validation':scope,'release_passed':False}
    (args.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    shutil.copytree(ROOT/'licenses',args.output/'licenses')
    shutil.copyfile(ROOT/'NOTICE.md',args.output/'NOTICE.md')
    shutil.copyfile(checkpoint/'receipt.json',args.output/'checkpoint-receipt.json')
    shutil.copyfile(verification_path,args.output/'native-check.json')
    if args.scoped_native_check:
        shutil.copyfile(checkpoint/'capture-receipt.json',args.output/'capture-receipt.json')
    for i, folder in enumerate(args.world_packs):
        shutil.copyfile(folder/'report.json',args.output/f'world-conversion-{i+1}.json')
    print(f'{package.resolve()}: {len(files)} files, {manifest["bytes"]:,} bytes; checkpoint XBE unchanged.')
    print('Password is intentionally absent from the console package. Use the local disposable account credentials.')

if __name__ == '__main__':
    main()
