"""Check a read-only native action-layer replay against saved server bindings."""
import argparse
import csv
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('capture', type=Path)
parser.add_argument('--bindings', type=Path, required=True)
args = parser.parse_args()
rows = list(csv.DictReader(args.capture.open()))
expected = json.loads(args.bindings.read_text())
if len(expected) != 120 or any(type(v) is not int or not 0 <= v <= 0xffffffff for v in expected):
    parser.error('Expected 120 unsigned server bindings')
shown = [r for r in rows if int(r['actionbar_layer'])]
settled = [r for r in rows if r['world_active'] == '1' and int(r['player_level']) and int(r['inventory_count']) and int(r['replay_frame']) < 600]
first = settled[-1] if settled else {}
last = rows[-1] if rows else {}
checks = {
    'all_layers_shown': {int(r['actionbar_layer']) for r in shown} == {1, 2, 3},
    'layer_follows_triggers': bool(shown) and all(r['actionbar_layer'] == r['layer'] for r in shown),
    'matches_authoritative_bindings': bool(shown) and all(0 <= int(r[f'actionbar_slot_{i}']) < 120 and int(r[f'actionbar_binding_{i}']) == expected[int(r[f'actionbar_slot_{i}'])] for r in shown for i in range(8)),
    'menu_suppression': any(r['menu'] == '1' and r['layer'] == '1' for r in rows) and all(r['actionbar_layer'] == '0' for r in rows if r['menu'] == '1'),
    'inventory_suppression': any(r['layer'] == '2' and 2360 < int(r['replay_frame']) < 2520 for r in rows) and all(r['actionbar_layer'] == '0' for r in rows if 2360 < int(r['replay_frame']) < 2520),
    'no_triggered_actions': bool(rows) and all(int(r['action']) == -1 and r['casts_accepted'] == '0' for r in rows),
    'normal_after_replay': last.get('replay_active') == '0' and last.get('actionbar_layer') == '0' and last.get('world_active') == '1',
    'progress_preserved': bool(first) and all(last[k] == first[k] for k in ('player_guid_low', 'player_xp', 'player_level', 'inventory_money', 'inventory_count')),
    'position_preserved': bool(first) and all(abs(float(last[k])-float(first[k])) < .01 for k in ('x', 'y', 'z', 'yaw')),
    'headroom': bool(rows) and min(int(r['free_kib']) for r in rows) >= 8192,
    'no_failures': bool(rows) and all(all(r[k] == '0' for k in ('failures', 'region_failures', 'book_failures', 'avatar_select_failures')) for r in rows),
    'no_packet_faults': not args.capture.with_suffix('.faults.jsonl').exists(),
}
result = {'passed': all(checks.values()), 'checks': checks,
          'scope': 'Native injected triggers, displayed binding telemetry and separate server snapshot; actual screenshots verify names/layout. Physical controls remain separate.'}
args.capture.with_suffix('.actionbar-check.json').write_text(json.dumps(result, indent=2)+'\n')
print(json.dumps(result, indent=2))
raise SystemExit(not result['passed'])
