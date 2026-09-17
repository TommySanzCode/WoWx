"""Verify the native Back-button map replay without treating it as a physical pad test."""
import argparse,csv,json,math
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);a=p.parse_args()
rows=list(csv.DictReader(a.capture.open()));last=rows[-1] if rows else {}
settled=[r for r in rows if r['world_active']=='1' and int(r['player_level']) and int(r['inventory_count']) and int(r['replay_frame'])<600]
first=settled[-1] if settled else {}
opened=[r for r in rows if r['map_open']=='1']
zone=[r for r in opened if r['map_id']=='30']
checks={
 'map_displayed':len(opened)>1200 and all(r['map_ready']=='1' for r in opened),
 'zone_and_continent_and_neighbour':{30,14,15}<={int(r['map_id']) for r in opened},
 'exploration_revealed':bool(zone) and min(int(r['map_overlays']) for r in zone)>0,
 'player_marker_projected':bool(zone) and all(r['map_marker']=='1' and abs(int(r['map_player_u'])-46480)<150 and abs(int(r['map_player_v'])-62180)<150 for r in zone),
 'zoom_and_pan':{1000,2000}<={int(r['map_zoom']) for r in opened} and any(int(r['map_u'])>70000 and int(r['map_v'])<30000 for r in opened),
 'bounded_texture':bool(opened) and all(r['map_bytes']==str(512*512*4+96) for r in opened),
 'six_deliberate_loads':last.get('map_loads')=='6',
 'input_captured':bool(opened) and all(r['buttons']=='0' and r['pressed']=='0' and r['layer']=='0' and r['action']=='-1' and r['actionbar_layer']=='0' for r in opened),
 'no_casts':bool(rows) and all(r['casts_accepted']=='0' and r['casts_rejected']=='0' for r in rows),
 'no_movement_or_camera_leak':bool(first) and all(abs(float(r[k])-float(first[k]))<.01 for r in rows if int(r['replay_frame'])>=600 for k in ('x','y','z','yaw','pitch')),
 'reconnect':bool(first) and last.get('world_active')=='1' and int(last['world_revision'])>int(first['world_revision']),
 'progress_preserved':bool(first) and all(last[k]==first[k] for k in ('player_guid_low','player_level','player_xp','inventory_money','inventory_count')),
 'closed_and_freed':last.get('replay_active')=='0' and all(last.get(k)=='0' for k in ('map_open','map_ready','map_bytes','menu')),
 'headroom':bool(rows) and min(int(r['free_kib']) for r in rows)>=8192,
 'no_failures':bool(rows) and all(all(r[k]=='0' for k in ('failures','region_failures','book_failures','avatar_select_failures','map_failures','text_failures')) for r in rows),
 'no_packet_faults':not a.capture.with_suffix('.faults.jsonl').exists(),
}
result={'passed':all(checks.values()),'checks':checks,'map_frames':len(opened),
 'scope':'Injected native pad replay, zone/continent imagery, server exploration, player marker, bounded allocation and capture through close. Physical controls and hardware remain separate.'}
a.capture.with_suffix('.map-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(not result['passed'])
