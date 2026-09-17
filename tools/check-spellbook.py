"""Verify the native controller assignment, reconnect and restoration replay."""
import csv
import json
import argparse
from pathlib import Path

parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('capture',type=Path);parser.add_argument('--actionbar',action='store_true');args=parser.parse_args()
path=args.capture
rows = list(csv.DictReader(path.open()))
active = [r for r in rows if r['world_active'] == '1' and int(r['scenario_stage']) > 0]
first, last = (active[0], active[-1]) if active else ({}, {})
# Online arrives before the player/inventory packets. Stage 2 starts only after
# the driver has initialized its saved-state baseline, before any binding edit.
initialized = next((r for r in active if r['scenario_stage'] == '2'), {})
stages = {int(r['scenario_stage']) for r in active}
assigned = [r for r in active if r['scenario_stage'] == '12' and r['book_binding'] == '78']
checks = {
    'correct_scenario': bool(rows) and all(r['scenario_kind'] == '12' for r in rows),
    'controller_flow_completed': {1,2,3,4,5,6,10,11,12,13,14,15,16,17,18,8} <= stages and 9 not in stages and last.get('scenario_stage') == '8',
    'book_browsed': any(r['book_open'] == '1' and r['book_screen'] == '0' and int(r['book_count']) > 0 and r['book_count'] == r['book_loaded'] for r in active),
    'assignment_confirmed': any(r['scenario_stage'] == '5' and r['book_screen'] == '2' and r['book_binding'] == '0' for r in active),
    'assignment_survived_reconnect': bool(assigned) and int(assigned[-1]['world_revision']) > int(first['world_revision']),
    'original_empty_binding_restored': last.get('book_binding') == '0' and bool(first) and int(last['world_revision']) >= int(first['world_revision']) + 2,
    'same_control_restored': bool(assigned) and last.get('book_server_slot') == assigned[-1]['book_server_slot'],
    'no_character_progress_change': bool(initialized) and all(last[k] == initialized[k] for k in ('player_guid_low','player_guid_high','player_xp','player_level','inventory_money','inventory_count')),
    'no_translation': bool(active) and all(abs(float(r[k]) - float(first[k])) < .01 for r in active for k in ('x','y','z')),
    'headroom': bool(rows) and min(int(r['free_kib']) for r in rows) >= 8192,
    'no_asset_failures': bool(rows) and all(r['failures'] == '0' and r['region_failures'] == '0' and r['book_failures'] == '0' for r in rows),
    'no_packet_faults': not path.with_suffix('.faults.jsonl').exists(),
}
if args.actionbar:
    for stage, expected in (('10',78),('17',0)):
        viewed=[r for r in rows if r['scenario_stage']==stage and int(r['actionbar_layer'])]
        checks['action_panel_'+('assigned' if expected else 'cleared')]=len(viewed)>60 and all(int(r['actionbar_binding_'+str(int(r['book_slot'])%8)])==expected and r['actionbar_slot_'+str(int(r['book_slot'])%8)]==r['book_server_slot'] for r in viewed)
    checks['book_hides_action_panel']=all(r['actionbar_layer']=='0' for r in rows if r['book_open']=='1')
result = {'passed': all(checks.values()), 'checks': checks,
          'scope': 'Native injected controller input, authoritative action list after two reconnects; does not verify physical controls or all spells/classes.'}
path.with_suffix('.spellbook-check.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
raise SystemExit(0 if result['passed'] else 1)
