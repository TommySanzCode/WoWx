"""Check that the current source and disc outputs match a recorded build."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('receipt', type=Path)
parser.add_argument('--output', type=Path)
args = parser.parse_args()
receipt = json.loads(args.receipt.read_text(encoding='utf-8'))
failures = []
counts = {}
for group in ('sources', 'outputs'):
    counts[group] = 0
    for name, expected in receipt[group].items():
        path = (ROOT / name).resolve()
        if not path.is_relative_to(ROOT) or not path.is_file():
            failures.append({'file': name, 'reason': 'missing or outside workspace'})
            continue
        with path.open('rb') as stream:
            digest = hashlib.file_digest(stream, 'sha256').hexdigest()
        required = expected if group == 'sources' else expected['sha256']
        if digest != required or (group == 'outputs' and path.stat().st_size != expected['bytes']):
            failures.append({'file': name, 'reason': 'hash or length mismatch'})
        else:
            counts[group] += 1
result = {'passed': not failures, 'matched': counts, 'failures': failures,
          'scope': 'Current recorded source/output identities; does not verify gameplay or hardware.'}
text = json.dumps(result, indent=2) + '\n'
if args.output:
    args.output.write_text(text, encoding='utf-8')
print(text, end='')
raise SystemExit(0 if result['passed'] else 1)
