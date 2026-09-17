"""Read the private vMaNGOS catalog around a saved character; never change SQL."""
import argparse,json,math,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--player-guid',type=int,default=1);p.add_argument('--radius',type=float,default=150)
p.add_argument('--existing',type=Path,default=Path('config/creatures.txt'));p.add_argument('--output',type=Path,required=True)
p.add_argument('--include-inactive-events',action='store_true');args=p.parse_args()
if not 1<=args.player_guid<=0xffffffff or not math.isfinite(args.radius) or not 1<=args.radius<=1000:p.error('Invalid character GUID or radius (1..1000 yards)')
def query(sql):
    r=subprocess.run(['wsl','-d','Ubuntu','-u','root','--','mariadb','--batch','--raw','--skip-column-names','-e',sql],capture_output=True,text=True,check=True)
    return [line.split('\t') for line in r.stdout.splitlines() if line]
players=query(f'SELECT map,position_x,position_y,position_z FROM wowx_characters.characters WHERE guid={args.player_guid}')
if len(players)!=1:raise SystemExit('Saved test character was not found')
event_filter='' if args.include_inactive_events else '''AND (
NOT EXISTS (SELECT 1 FROM wowx_world.game_event_creature e WHERE e.guid=c.guid)
OR EXISTS (SELECT 1 FROM wowx_world.game_event_creature e JOIN wowx_characters.game_event_status s ON e.event=s.event WHERE e.guid=c.guid AND e.event>0)
OR EXISTS (SELECT 1 FROM wowx_world.game_event_creature e WHERE e.guid=c.guid AND e.event<0 AND NOT EXISTS (SELECT 1 FROM wowx_characters.game_event_status s WHERE s.event=-e.event)))'''
rows=query(f'''SELECT DISTINCT t.entry,t.name,t.display_id1,t.display_id2,t.display_id3,t.display_id4
FROM wowx_world.creature c JOIN wowx_world.creature_template t ON t.entry IN(c.id,c.id2,c.id3,c.id4,c.id5)
JOIN wowx_characters.characters p ON p.guid={args.player_guid} AND c.map=p.map
WHERE POW(c.position_x-p.position_x,2)+POW(c.position_y-p.position_y,2)+POW(c.position_z-p.position_z,2)<{args.radius**2}
AND c.patch_min<=10 AND c.patch_max>=10
{event_filter}
AND t.patch=(SELECT MAX(v.patch) FROM wowx_world.creature_template v WHERE v.entry=t.entry AND v.patch<=10)
ORDER BY t.entry''')
existing=args.existing.read_text();ids={int(line.strip()) for line in existing.splitlines() if line.strip() and not line.startswith('#')}
additions={}
for row in rows:
    for value in row[2:]:
        display=int(value)
        if display and display not in ids:additions.setdefault(display,row[1])
all_ids=ids|set(additions)
# vMaNGOS can choose the alternate gender from its display addon table.
for depth in range(4):
    variants=query('SELECT a.display_id,a.display_id_other_gender FROM wowx_world.creature_display_info_addon a WHERE a.display_id IN ('+
        ','.join(map(str,sorted(all_ids)))+') AND a.build=(SELECT MAX(b.build) FROM wowx_world.creature_display_info_addon b WHERE b.display_id=a.display_id AND b.build<=5875)')
    new={int(other):'Alternate gender of display '+source for source,other in variants if int(other) and int(other) not in all_ids}
    if not new:break
    additions.update(new);all_ids.update(new)
else:raise SystemExit('Alternate display chain exceeded bounded catalog depth')
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_text(existing.rstrip()+'\n\n# Private server catalog near saved character; includes alternate genders\n'+
    ''.join(f'# {name}\n{display}\n' for display,name in sorted(additions.items())))
report={'character_guid':args.player_guid,'saved_map_position':players[0],'radius':args.radius,'templates':len(rows),'include_inactive_events':args.include_inactive_events,
        'displays_before':len(ids),'displays_after':len(all_ids),'additions':additions,
        'scope':'Read-only spawn catalog, current event eligibility and alternate display IDs. Does not claim coverage of all event overrides, summoned, scripted or transformed units.'}
args.output.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'{len(additions)} additional displays; {len(all_ids)} total; {len(rows)} nearby templates')
