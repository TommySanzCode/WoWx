"""Prepare shared race/sex avatar geometry and Vanilla appearance layers."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--data', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--profile', type=Path, default=ROOT / 'config/player.txt')
parser.add_argument('--defaults', action='store_true', help='Prepare all sixteen default race/sex looks with this profile equipment catalog')
parser.add_argument('--items', type=Path, help='Optional additional display/slot/type catalog')
args = parser.parse_args()
converter = ROOT / 'build/host/wowx_assetc.exe'
verifier = ROOT / 'build/host/wowx_avatar_tests.exe'
values = [int(word) for line in args.profile.read_text().splitlines() for word in line.split('#')[0].split()]
if len(values) != 47:
    parser.error('Profile must contain seven appearance values and twenty equipment pairs')
looks = [[race, sex, 0, 0, 0, 0, 0] for race in range(1, 9) for sex in range(2)] if args.defaults else [values[:7]]
for look in looks:
    if not 1 <= look[0] <= 8 or not 0 <= look[1] <= 1 or any(not 0 <= x <= 255 for x in look[2:]):
        parser.error('Invalid Vanilla appearance')
if args.output.exists():
    parser.error('Choose a new output directory; prepared assets are never overwritten')
args.output.mkdir(parents=True)

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

receipt = {'converter': digest(converter), 'verifier': digest(verifier),
           'profile': digest(args.profile), 'items': digest(args.items) if args.items else None,
           'scope': 'Shared Vanilla appearance layers and listed equipment; native acceptance and full visual parity still required', 'profiles': []}
for look in looks:
    name = 'A' + ''.join(f'{value:02X}' for value in look)
    profile = args.output / (name + '.txt')
    profile.write_text(' '.join(map(str, look)) + '\n' + '\n'.join(f'{values[i]} {values[i+1]}' for i in range(7, 47, 2)) + '\n')
    pack = args.output / (name + '.WXP')
    metadata = pack.with_suffix('.WXA')
    layers = args.output / f'L{look[0]:02X}{look[1]:02X}.WXL'
    command = [str(converter), '--avatar', str(args.data), str(pack), str(profile)]
    if args.items:
        command.append(str(args.items))
    with (args.output / (name + '.log')).open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
        subprocess.run([str(verifier), str(pack), str(metadata)], stdout=log, stderr=subprocess.STDOUT, check=True)
    receipt['profiles'].append({'look': look, 'files': {p.name: {'bytes': p.stat().st_size, 'sha256': digest(p)} for p in (pack, metadata,layers)}})
    print(f'{name}: verified {pack.stat().st_size + metadata.stat().st_size} bytes', flush=True)
(args.output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(f'Prepared and runtime-verified {len(looks)} profiles. Stage their WXP/WXA/WXL files together with the project emulator stopped.')
