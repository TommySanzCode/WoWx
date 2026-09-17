"""Check a neutral native session against a previously saved character capture."""
import argparse
import csv
import json
import math
import struct
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('capture',type=Path)
p.add_argument('--baseline',type=Path,required=True)
p.add_argument('--staged',type=Path,default=Path(__file__).resolve().parents[1]/'build/xbox')
a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()))
baseline=list(csv.DictReader(a.baseline.open()))
last=rows[-1] if rows else {}
before=baseline[-1] if baseline else {}
progress=('player_guid_low','player_guid_high','player_level','player_xp',
          'inventory_count','inventory_money','active_quests')
equipment=tuple(f'equipment_{i}_{field}' for i in range(19) for field in ('entry','display','type'))
faults=a.capture.with_suffix('.faults.jsonl')
lifecycle_path=a.capture.with_suffix('.lifecycle.json')
lifecycle=json.loads(lifecycle_path.read_text()) if lifecycle_path.exists() else None
fixtures={}
for name,expected in [('scenario.bin',b'NONE'),('CAMP.RTE',b'NONE'),('FONT.BIN',b'NONE'),
                      ('LOGIN.BIN',b'NONE'),
                      ('SOAK.BIN',b'NONE'),('TESTCHAR.BIN',b'NONE'),('input.rpl',struct.pack('<4sII',b'WXR1',0,0))]:
    path=a.staged/name
    fixtures[name]=path.exists() and path.read_bytes()==expected
checks={
    'sustained_capture':len(rows)>30 and int(last['time_ms'])-int(rows[0]['time_ms'])>=60000,
    'normal_input':bool(rows) and all(r['scenario_kind']=='0' and r['replay_active']=='0' and r['replay_frame']=='0' for r in rows),
    'online':last.get('world_active')=='1' and last.get('equipment_ready_mask')=='524287',
    'saved_progress':bool(before) and bool(last) and all(last[k]==before[k] for k in progress+equipment),
    'saved_position':bool(before) and bool(last) and all(abs(float(last[k])-float(before[k]))<.01 for k in ('x','y','z')),
    'saved_view':bool(before) and bool(last) and abs(math.remainder(float(last['yaw'])-float(before['yaw']),2*math.pi))<.01,
    'one_profile':bool(rows) and max(int(r['avatar_profile_changes']) for r in rows)==1 and last.get('avatar_matched')=='1' and last.get('avatar_ready')=='1' and last.get('avatar_missing')=='0',
    'no_failures':bool(rows) and all(all(r[k]=='0' for k in ('failures','region_failures','avatar_select_failures','book_failures','map_failures','text_failures')) and r['lobby_phase']!='4' for r in rows),
    'map_unallocated':bool(rows) and all(r['map_open']=='0' and r['map_ready']=='0' and r['map_bytes']=='0' for r in rows),
    'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
    'no_packet_faults':not faults.exists() or not faults.read_text().strip(),
    'no_guest_exit':lifecycle is not None and lifecycle.get('end_reason')=='duration_limit' and lifecycle.get('guest_exit') is None,
    'no_staged_fixture':all(fixtures.values()),
}
result={'passed':all(checks.values()),'checks':checks,'fixtures':fixtures,
        'baseline':str(a.baseline),'samples':len(rows),
        'scope':'Neutral native capture and current staged fixture files; unchanged saved character against baseline. Adapter connection does not establish physical controller verification.'}
a.capture.with_suffix('.normal-check.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
raise SystemExit(not result['passed'])
