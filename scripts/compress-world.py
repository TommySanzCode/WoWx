"""Prepare GPU-compressed world packs while retaining the source pack directory."""
import argparse
import hashlib
import json
import re
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def geometry_matches(source, target):
    # Independent comparison: compression may change only texture encoding and
    # payload offsets. All gameplay/collision geometry and placement IDs stay exact.
    a, b = source.read_bytes(), target.read_bytes()
    ah, bh = struct.unpack_from('<8I', a), struct.unpack_from('<8I', b)
    if ah[0] != bh[0] or ah[2:7] != bh[2:7] or ah[1] not in (4,5,6,7,8,9,10) or bh[1] not in (5,6,7,8,9,10) or bh[1]<ah[1] or bh[7] != len(b):
        raise ValueError('Pack header changed unexpectedly')
    for i in range(ah[2]):
        x = struct.unpack_from('<16I', a, 32 + i * 64)
        y = struct.unpack_from('<16I', b, 32 + i * 64)
        if x[:4] != y[:4] or x[7:13] != y[7:13] or x[14:] != y[14:] or (x[13] & ~97) != (y[13] & ~97):
            raise ValueError(f'Geometry metadata changed in entry {i}')
        for off, size in ((4, x[2] * 32), (5, x[3] * 2)):
            if a[x[off]:x[off] + size] != b[y[off]:y[off] + size]:
                raise ValueError(f'Geometry payload changed in entry {i}')
        if x[13]&16384 and a[x[4]-2652:x[4]]!=b[y[4]-2652:y[4]]:
            raise ValueError(f'Material animation changed in entry {i}')
        cb=x[2]*4 if x[13]&131072 else 0
        mb=2652 if x[13]&16384 else 0
        if cb and a[x[4]-mb-cb:x[4]-mb]!=b[y[4]-mb-cb:y[4]-mb]:
            raise ValueError(f'Vertex lighting changed in entry {i}')
    if ah[1]>=9:
        af,bf=struct.unpack_from('<4sIII',a,len(a)-16),struct.unpack_from('<4sIII',b,len(b)-16)
        if af[:3]!=bf[:3] or a[af[3]:-16]!=b[bf[3]:-16]:
            raise ValueError('Environment metadata changed during repacking')
    return ah[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'build/worldpacks')
    parser.add_argument('--output', type=Path, default=ROOT / 'build/compressed-world')
    args = parser.parse_args()
    source, output = args.source.resolve(), args.output.resolve()
    if source == output:
        parser.error('Retain the original packs in a separate directory')
    index = bytearray((source / 'world.wxi').read_bytes())
    magic, version, count, stride = struct.unpack_from('<4sIII', index)
    if magic != b'WXI1' or version != 1 or not 0 < count <= 8192 or stride != 32 or len(index) != 16 + count * 32:
        raise ValueError('Invalid source tile index')
    files = []
    for i, (map_id, x, y, name, size, reserved) in enumerate(struct.iter_unpack('<IHH16sII', index[16:])):
        name = name.split(b'\0')[0].decode('ascii')
        if not re.fullmatch(r'M\d{7}\.WXP', name) or name != f'M{map_id:03}{x:02}{y:02}.WXP' or x >= 64 or y >= 64 or reserved:
            raise ValueError('Invalid tile name')
        if (source / name).stat().st_size != size:
            raise ValueError(f'Source size mismatch: {name}')
        files.append((name, i))
    if (source / 'world.wxp').exists():
        files.append(('world.wxp', None))
    output.mkdir(parents=True, exist_ok=True)
    optimizer = ROOT / 'build/host/wowx_packopt.exe'
    verifier = ROOT / 'build/host/wowx_packcheck.exe'
    recipe = {'optimizer_sha256': sha(optimizer), 'verifier_sha256': sha(verifier)}
    results = []
    for name, i in files:
        original, target = source / name, output / name
        source_hash = sha(original)
        if target.exists():
            old = json.loads((output / (name + '.json')).read_text(encoding='utf-8'))
            if old.get('source_sha256') != source_hash or old.get('sha256') != sha(target) or any(old.get(k) != v for k, v in recipe.items()):
                raise ValueError(f'Existing output has a different recipe: {name}; select a new output directory')
        else:
            subprocess.run([str(optimizer), str(original), str(target)], check=True)
        entries = geometry_matches(original, target)
        with (output / (name + '.verify.log')).open('w') as log:
            subprocess.run([str(verifier), str(target)], stdout=log, stderr=subprocess.STDOUT, check=True)
        receipt = {**recipe, 'source_sha256': source_hash, 'sha256': sha(target), 'file': name,
                   'source_bytes': original.stat().st_size, 'bytes': target.stat().st_size, 'entries': entries,
                   'geometry_exact': True}
        pending = output / (name + '.json.tmp')
        pending.write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
        pending.replace(output / (name + '.json'))
        if i is not None:
            struct.pack_into('<I', index, 16 + i * 32 + 24, receipt['bytes'])
        results.append(receipt)
    pending = output / 'world.wxi.tmp'
    pending.write_bytes(index)
    pending.replace(output / 'world.wxi')
    report = {'packs': results, 'source_bytes': sum(r['source_bytes'] for r in results),
              'compressed_bytes': sum(r['bytes'] for r in results)}
    (output / 'compression.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: v for k, v in report.items() if k != 'packs'}))

if __name__ == '__main__':
    main()
