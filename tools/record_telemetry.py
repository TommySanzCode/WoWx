"""Capture bounded localhost-only development telemetry from the Xbox guest."""
from draw_telemetry import FIELDS as DRAW_FIELDS,decode as decode_draw
from profile_telemetry import FIELDS as PROFILE_LOAD_FIELDS,decode as decode_profile
from environment_telemetry import FIELDS as ENVIRONMENT_FIELDS,decode as decode_environment
from material_telemetry import FIELDS as MATERIAL_FIELDS,decode as decode_material
import argparse
import csv
from action_telemetry import FIELDS as ACTION_FIELDS,decode as decode_actions
from stream_telemetry import FIELDS as STREAM_FIELDS,decode as decode_stream,ACTOR_FIELDS,decode_actors
from lighting_telemetry import FIELDS as LIGHT_FIELDS,decode as decode_light
from weather_telemetry import FIELDS as WEATHER_FIELDS,decode as decode_weather
import json
import math
import socket
import struct
import time
from pathlib import Path
from process_watch import ProcessWatch

FIELDS='time_ms frame_ms free_kib cache_kib draws triangles loads failures x y z yaw pitch buttons pressed layer action slot replay_frame replay_active menu saved deadzone world_active world_revision movement_sent packets_received map entry_x entry_y entry_z entry_orientation npc_submitted npc_missing entity_count'.split()
FIELDS+='quest_screen quest_id completed_quest kills damage_dealt damage_received player_xp active_quests scenario_stage scenario_frame looted_items looted_money casts_accepted casts_rejected spell_hits last_spell inventory_count equipped_shield'.split()
FIELDS+='region_loaded region_transitions tile_x tile_y region_failures'.split()
FIELDS+='player_health player_flags position_revision inventory_money vendor_count bundles_bought items_sold'.split()
FIELDS+='appearance_revision equipment_ready_mask player_identity player_look facial_hair appearance_flags player_display player_native_display'.split()
FIELDS += [f'equipment_{i}_{field}' for i in range(19) for field in ('entry','display','type')]
FIELDS+=['scenario_kind']
FIELDS+='avatar_matched avatar_ready avatar_drawn avatar_missing avatar_revision avatar_bytes avatar_equipment camera_mm avatar_clip avatar_pose'.split()
FIELDS+='lobby_phase lobby_count character_screen character_selected character_row character_key character_result character_result_revision player_guid_low player_guid_high player_level'.split()
FIELDS+=['pad_connected']
FIELDS+='journal_open journal_detail journal_story journal_id journal_ready journal_progress'.split()
FIELDS+='book_open book_screen book_count book_loaded book_failures book_spell book_slot book_server_slot book_binding action_revision'.split()
FIELDS+='avatar_select_attempts avatar_profile_changes avatar_select_failures'.split()

FIELDS+=['actionbar_layer']+[f'actionbar_slot_{i}' for i in range(8)]+[f'actionbar_binding_{i}' for i in range(8)]

FIELDS+='utility_open utility_pending utility_selection utility_action utility_revision inventory_open'.split()

PROFILE_FIELDS=['profile_'+x+'_ms' for x in ('update','move','stream','camera','actors','draw','ui','present','pace','work')]
FIELDS+=PROFILE_FIELDS
TEXT_FIELDS="profile_scene_wait_ms profile_text_submit_ms profile_text_wait_ms profile_swap_ms text_glyphs text_mode text_bytes text_failures".split()
FIELDS+=TEXT_FIELDS

FIELDS+='map_open map_id map_ready map_loads map_failures map_bytes map_zoom map_u map_v map_marker map_player_u map_player_v map_overlays map_revision'.split()

FIELDS+='death_state corpse_known corpse_map corpse_low corpse_high corpse_distance_mm corpse_wait_ms corpse_ready corpse_queries spirit_releases corpse_reclaims healer_requests resurrections map_body map_body_u map_body_v corpse_query_attempts corpse_in_range'.split()

FIELDS+='soak_cycles soak_elapsed_ms soak_limit_ms soak_started'.split()
FIELDS+='ui_ready ui_bytes ui_quads ui_failures'.split()
FIELDS+='login_mode login_phase login_auth login_count login_selected login_keyboard login_field login_attempts login_cancellations login_test_stage login_test_frame'.split()

FIELDS+='backdrop_ready backdrop_bytes backdrop_draws backdrop_triangles backdrop_updates backdrop_failures backdrop_loads backdrop_time'.split()

FIELDS+='effects_emitters effects_lights effects_live effects_peak effects_births effects_drops effects_clock_skips effects_quads'.split()

FIELDS+='preview_active preview_scene preview_ready preview_drawn preview_bytes preview_identity preview_pose preview_changes preview_background_missing preview_equipment_missing preview_failures preview_rotation preview_zoom'.split()
FIELDS+='preview_class preview_outfit preview_loading preview_read_bytes preview_index_reads'.split()

FIELDS+='preview_look preview_facial preview_compositions preview_atlas_hash character_appearance_row character_randomizations'.split()

FIELDS+='hud_drawn hud_quads hud_player_health hud_player_max_health hud_player_power_type hud_player_power hud_player_max_power hud_target_low hud_target_high hud_target_health hud_target_max_health hud_target_power_type hud_target_power hud_target_max_power hud_xp hud_next_xp hud_target_present'.split()

FIELDS+='portrait_ready portrait_count portrait_failures portrait_player_key portrait_player_draws portrait_target_key portrait_target_draws portrait_spans portrait_submit_ms portrait_fixture'.split()

FIELDS+='icons_ready icons_index icons_images icons_bytes icons_loads icons_pending icons_failures icons_drawn ui_batches icons_upload_ms'.split()

FIELDS+='cooldown_ready cooldown_count cooldown_bytes cooldown_packets cooldown_revision cooldown_missing cooldown_overflow cooldown_state_bytes'.split()
FIELDS+=[f'cooldown_remaining_{i}' for i in range(8)]+['cooldown_held_mask','cooldown_fixture_step']

FIELDS+='cooldown_version modifier_updates modifier_ambiguous gcd_starts gcd_cancels gcd_mask cast_speed_milli spellmod_family'.split()

def decode_packet(data):
    """Return a current-width row, or None for malformed/version-mismatched packets."""
    if len(data) not in (96,132,144,176,192,216,236,264,524,528,568,612,616,640,680,692,760,784,824,856,912,984,1000,1016,1060,1092,1124,1176,1196,1220,1288,1328,1368,1440,1472):return None
    try:
        decoded=struct.unpack('<4s8I5f3I2iI3if'+('5I4f' if len(data)>=132 else '')+('3I' if len(data)>=144 else '')+('8I' if len(data)>=176 else '')+('4I' if len(data)>=192 else '')+('6I' if len(data)>=216 else '')+('5I' if len(data)>=236 else '')+('7I' if len(data)>=264 else '')+('65I' if len(data)>=524 else '')+('I' if len(data)>=528 else '')+('10I' if len(data)>=568 else '')+('11I' if len(data)>=612 else '')+('I' if len(data)>=616 else '')+('6I' if len(data)>=640 else '')+('10I' if len(data)>=680 else '')+('3I' if len(data)>=692 else '')+('17I' if len(data)>=760 else '')+('6I' if len(data)>=784 else '')+('10I' if len(data)>=824 else '')+('8I' if len(data)>=856 else '')+('14I' if len(data)>=912 else '')+('18I' if len(data)>=984 else '')+('4I' if len(data)>=1000 else '')+('4I' if len(data)>=1016 else '')+('11I' if len(data)>=1060 else '')+('8I' if len(data)>=1092 else '')+('8I' if len(data)>=1124 else '')+('13I' if len(data)>=1176 else '')+('5I' if len(data)>=1196 else '')+('6I' if len(data)>=1220 else '')+('17I' if len(data)>=1288 else '')+('10I' if len(data)>=1328 else '')+('10I' if len(data)>=1368 else '')+('18I' if len(data)>=1440 else '')+('8I' if len(data)>=1472 else ''),data)
        if decoded[0]!={96:b'WXT1',132:b'WXT2',144:b'WXT3',176:b'WXT4',192:b'WXT5',216:b'WXT6',236:b'WXT7',264:b'WXT8',524:b'WXT9',528:b'WXTA',568:b'WXTB',612:b'WXTC',616:b'WXTD',640:b'WXTE',680:b'WXTF',692:b'WXTG',760:b'WXTH',784:b'WXTI',824:b'WXTJ',856:b'WXTK',912:b'WXTL',984:b'WXTM',1000:b'WXTN',1016:b'WXTO',1060:b'WXTP',1092:b'WXTQ',1124:b'WXTR',1176:b'WXTS',1196:b'WXTT',1220:b'WXTU',1288:b'WXTV',1328:b'WXTW',1368:b'WXTX',1440:b'WXTY',1472:b'WXTZ'}[len(data)]:return None
        if len(data)==96:decoded+=tuple([0]*9)
        if len(data)<144:decoded+=tuple([0]*3)
        if len(data)<176:decoded+=tuple([0]*8)
        if len(data)<192:decoded+=tuple([0]*4)
        if len(data)<216:decoded+=tuple([0]*6)
        if len(data)<236:decoded+=tuple([0]*5)
        if len(data)<264:decoded+=tuple([0]*7)
        if len(data)<524:decoded+=tuple([0]*65)
        if len(data)<528:decoded+=(0,)
        if len(data)<568:decoded+=tuple([0]*10)
        if len(data)<612:decoded+=tuple([0]*11)
        if len(data)<616:decoded+=(-1,)
        if len(data)<640:decoded+=(0,)*6
        if len(data)<680:decoded+=(0,)*10
        if len(data)<692:decoded+=(0,)*3
        if len(data)<760:decoded+=(0,)*17
        if len(data)<784:decoded+=(0,)*6
        if len(data)<824:decoded+=(0,)*10
        if len(data)<856:decoded+=(0,)*8
        if len(data)<912:decoded+=(0,)*14
        if len(data)<984:decoded+=(0,)*18
        if len(data)<1000:decoded+=(0,)*4
        if len(data)<1016:decoded+=(0,)*4
        if len(data)<1060:decoded+=(0,)*11
        if len(data)<1092:decoded+=(0,)*8
        if len(data)<1124:decoded+=(0,)*8
        if len(data)<1176:decoded+=(0,)*13
        if len(data)<1196:decoded+=(0,)*5
        if len(data)<1220:decoded+=(0,)*6
        if len(data)<1288:decoded+=(0,)*17
        if len(data)<1328:decoded+=(0,)*10
        if len(data)<1368:decoded+=(0,)*10
        if len(data)<1440:decoded+=(0,)*18
        if len(data)<1472:decoded+=(0,)*8
        row=decoded[1:];values=list(row)
        if not all(math.isfinite(x) for x in values):return None
    except (ValueError,struct.error):return None
    return row

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--guest-pid-file',type=Path,help='Observe the dedicated xemu process and record its exit code');parser.add_argument('--seconds',type=int,default=90);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--expect-replay',action='store_true');parser.add_argument('--expect-combat',action='store_true');parser.add_argument('--expect-turnin',action='store_true');parser.add_argument('--expect-boundary',action='store_true');parser.add_argument('--expect-kobold-turnin',action='store_true');parser.add_argument('--finish-on-scenario',action='store_true',help='Finish after ten seconds in a terminal replay state');args=parser.parse_args()
    if not 1<=args.seconds<=4500:parser.error('Capture duration must be between 1 and 4500 seconds')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    samples=[];slots=set();saved=False;start_position=None;end_position=None;last_replay=0
    watch=ProcessWatch(args.guest_pid_file) if args.guest_pid_file else None
    end_reason='duration_limit';guest_exit=None;last_packet=time.monotonic();watched_pid=None
    with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as listener,args.output.open('w',newline='') as stream,args.output.with_suffix('.actions.csv').open('w',newline='') as action_stream,args.output.with_suffix('.stream.csv').open('w',newline='') as stream_metrics,args.output.with_suffix('.actors.csv').open('w',newline='') as actor_metrics,args.output.with_suffix('.lighting.csv').open('w',newline='') as light_metrics,args.output.with_suffix('.weather.csv').open('w',newline='') as weather_metrics,args.output.with_suffix('.profiles.csv').open('w',newline='') as profile_metrics,args.output.with_suffix('.materials.csv').open('w',newline='') as material_metrics,args.output.with_suffix('.environment.csv').open('w',newline='') as environment_metrics,args.output.with_suffix('.draw.csv').open('w',newline='') as draw_metrics:
        listener.bind(('127.0.0.1',39001));listener.settimeout(1)
        writer=csv.writer(stream);writer.writerow(FIELDS);deadline=time.monotonic()+args.seconds;terminal_since=None
        action_writer=csv.writer(action_stream);action_writer.writerow(ACTION_FIELDS)
        light_writer=csv.writer(light_metrics);light_writer.writerow(LIGHT_FIELDS)
        profile_writer=csv.writer(profile_metrics);profile_writer.writerow(PROFILE_LOAD_FIELDS)
        material_writer=csv.writer(material_metrics);material_writer.writerow(MATERIAL_FIELDS)
        draw_writer=csv.writer(draw_metrics);draw_writer.writerow(DRAW_FIELDS)
        environment_writer=csv.writer(environment_metrics);environment_writer.writerow(ENVIRONMENT_FIELDS)
        weather_writer=csv.writer(weather_metrics);weather_writer.writerow(WEATHER_FIELDS)
        stream_writer=csv.writer(stream_metrics);stream_writer.writerow(STREAM_FIELDS)
        actor_writer=csv.writer(actor_metrics);actor_writer.writerow(ACTOR_FIELDS)
        print('Recording Xbox telemetry on localhost:39001',flush=True)
        while time.monotonic()<deadline and len(samples)<150000:
            if watch:
                guest_exit=watch.poll()
                if watch.pid is not None and watch.pid!=watched_pid:
                    watched_pid=watch.pid;print(f'Watching dedicated xemu PID {watched_pid}',flush=True)
                if guest_exit is not None:
                    end_reason='guest_exit';print('xemu exited: '+json.dumps(guest_exit),flush=True);break
            if samples and time.monotonic()-last_packet>60:
                end_reason='telemetry_silence';print('No valid guest telemetry for 60 seconds.',flush=True);break
            try:data,_=listener.recvfrom(2048)
            except socket.timeout:continue
            if data[:4]==b'WXD1':
                row=decode_draw(data)
                if row is not None:draw_writer.writerow(row)
                continue
            if data[:4]==b'WXY1':
                row=decode_environment(data)
                if row is not None:environment_writer.writerow(row)
                continue
            if data[:4] in (b'WXM1',b'WXM2',b'WXM3'):
                material_row=decode_material(data)
                if material_row is not None:material_writer.writerow(material_row)
                continue
            if data[:4]==b'WXQ1':
                profile_row=decode_profile(data)
                if profile_row is not None:profile_writer.writerow(profile_row)
                continue
            if data[:4]==b'WXW1':
                weather_row=decode_weather(data)
                if weather_row is not None:weather_writer.writerow(weather_row)
                continue
            if data[:4] in (b'WXL2',b'WXL3'):
                light_row=decode_light(data)
                if light_row is not None:light_writer.writerow(light_row)
                continue
            if data[:4] in (b'WXN1',b'WXN2',b'WXN3'):
                actor_row=decode_actors(data)
                if actor_row is not None:actor_writer.writerow(actor_row)
                continue
            if data[:4] in (b'WXS1',b'WXS2',b'WXS3',b'WXS4',b'WXS5'):
                stream_row=decode_stream(data)
                if stream_row is not None:stream_writer.writerow(stream_row)
                continue
            if data[:4]==b'WXF1':
                action_row=decode_actions(data)
                if action_row is not None:action_writer.writerow(action_row)
                continue
            if data[:4]==b'WXE1' and len(data)>=24:
                revision,reason,opcode,size,captured=struct.unpack_from('<5I',data,4)
                if captured<=768 and len(data)==24+captured:
                    fault=dict(revision=revision,reason=reason,opcode=hex(opcode),size=size,captured=captured,hex=data[24:].hex())
                    with args.output.with_suffix('.faults.jsonl').open('a') as faults:faults.write(json.dumps(fault)+'\n')
                    print('World packet failure: '+json.dumps(fault),flush=True)
                continue
            row=decode_packet(data)
            if row is None:continue
            values=list(row)
            if samples and values[0]<samples[-1]['time_ms']:
                print('Guest clock restarted; ending this capture before the new boot.',flush=True)
                end_reason='guest_clock_restart';break
            last_packet=time.monotonic();writer.writerow(row)
            sample=dict(zip(FIELDS,values));samples.append(sample)
            if sample['slot']>=0:slots.add(int(sample['slot']))
            saved|=sample['saved']==1
            position=[sample[x] for x in ('x','y','z')]
            if start_position is None:start_position=position
            end_position=position;last_replay=max(last_replay,int(sample['replay_frame']))
            if args.finish_on_scenario and sample['scenario_stage'] in (8,9):
                if terminal_since is None:terminal_since=time.monotonic()
                if time.monotonic()-terminal_since>=10:end_reason='terminal_scenario';break
            else:terminal_since=None
        stream.flush()
    if len(samples)>=150000:end_reason='sample_limit'
    lifecycle={'end_reason':end_reason,'guest_pid':watch.pid if watch else None,'guest_exit':guest_exit,'samples':len(samples)}
    args.output.with_suffix('.lifecycle.json').write_text(json.dumps(lifecycle,indent=2)+'\n')
    if watch:watch.close()
    if not samples:raise SystemExit('No telemetry received; capture is not a passing test.')
    # Exclude only the first record's undefined previous-frame interval.
    times=sorted(sample['frame_ms'] for sample in samples[1:] if sample['frame_ms']>0)
    def percentile(fraction):return times[min(len(times)-1,int((len(times)-1)*fraction))] if times else None
    summary={'capture_lifecycle':lifecycle,'samples':len(samples),'minimum_free_kib':min(x['free_kib'] for x in samples),
        'characters':{'lobby_phases':sorted({int(x['lobby_phase']) for x in samples}),
            'ui_screens':sorted({int(x['character_screen']) for x in samples if x['lobby_phase']==2}),
            'maximum_roster':max(x['lobby_count'] for x in samples),
            'guids_entered':sorted({int(x['player_guid_low'])|(int(x['player_guid_high'])<<32) for x in samples if x['world_active']})},
        'input':{'pad_states':sorted({int(x['pad_connected']) for x in samples}),
            'buttons_seen':sorted({int(x['buttons']) for x in samples if x['buttons']}),
            'pressed_seen':sorted({int(x['pressed']) for x in samples if x['pressed']}),
            'scope':'Pad state is after replay injection; only runs without a replay measure adapter connection. -1 means old telemetry did not report it.'},
        'avatar':{'maximum_draws':max(x['avatar_drawn'] for x in samples),'maximum_missing':max(x['avatar_missing'] for x in samples),
            'maximum_bytes':max(x['avatar_bytes'] for x in samples),'revisions':sorted(set(int(x['avatar_revision']) for x in samples)),
            'equipment_masks':sorted(set(int(x['avatar_equipment']) for x in samples if x['avatar_drawn'])),
            'animated_pose_count':len(set(int(x['avatar_pose']) for x in samples if x['avatar_ready'])),
            'minimum_camera_mm':min(x['camera_mm'] for x in samples),'maximum_camera_mm':max(x['camera_mm'] for x in samples)},
        'journal':{'opened':any(x['journal_open'] for x in samples),'details':any(x['journal_detail'] and x['journal_ready'] for x in samples),'description':any(x['journal_story'] and x['journal_ready'] for x in samples),'quests':sorted({int(x['journal_id']) for x in samples if x['journal_ready']}),'progress':sorted({int(x['journal_progress']) for x in samples if x['journal_ready']})},
        'maximum_cache_kib':max(x['cache_kib'] for x in samples),'maximum_asset_failures':max(x['failures'] for x in samples),
        'frame_ms_p50':percentile(.5),'frame_ms_p95':percentile(.95),'frame_ms_p99':percentile(.99),
        'frame_ms_max':max(times) if times else None,'action_slots_seen':sorted(slots),
        'settings_save_observed':saved,'replay_last_frame':last_replay,
        'start_position':start_position,'end_position':end_position,
        'world_revisions':sorted(set(int(x['world_revision']) for x in samples if x['world_active'])),
        'maximum_movement_sent':max(x['movement_sent'] for x in samples),
        'maximum_packets_received':max(x['packets_received'] for x in samples),
        'maximum_npc_submitted':max(x['npc_submitted'] for x in samples),
        'maximum_npc_missing':max(x['npc_missing'] for x in samples),
        'maximum_entity_count':max(x['entity_count'] for x in samples),
        'maximum_region_loaded':max(x['region_loaded'] for x in samples),
        'maximum_region_transitions':max(x['region_transitions'] for x in samples),
        'maximum_vendor_count':max(x['vendor_count'] for x in samples),
        'maximum_bundles_bought':max(x['bundles_bought'] for x in samples),
        'maximum_items_sold':max(x['items_sold'] for x in samples),
        'last_inventory_money':int(samples[-1]['inventory_money']),
        'last_player_health':int(samples[-1]['player_health']),
        'last_player_flags':int(samples[-1]['player_flags']),
        'maximum_region_failures':max(x['region_failures'] for x in samples),
        'tiles_seen':sorted(set((int(x['map']),int(x['tile_x']),int(x['tile_y'])) for x in samples if x['region_loaded'])),
        'maximum_casts_accepted':max(x['casts_accepted'] for x in samples),
        'maximum_casts_rejected':max(x['casts_rejected'] for x in samples),
        'maximum_spell_hits':max(x['spell_hits'] for x in samples),
        'maximum_inventory_items':max(x['inventory_count'] for x in samples),
        'equipped_shield_values':sorted(set(int(x['equipped_shield']) for x in samples if x['world_active'] and x['inventory_count'])),
        'scenario_last_stage':int(samples[-1]['scenario_stage']),
        'maximum_looted_items':max(x['looted_items'] for x in samples),
        'maximum_looted_money':max(x['looted_money'] for x in samples),
        'maximum_kills':max(x['kills'] for x in samples),
        'maximum_damage_dealt':max(x['damage_dealt'] for x in samples),
        'maximum_damage_received':max(x['damage_received'] for x in samples),
        'maximum_player_xp':max(x['player_xp'] for x in samples),
        'maximum_active_quests':max(x['active_quests'] for x in samples),
        'quest_dialogs':sorted(set(int(x['quest_id']) for x in samples if x['quest_id'])),
        'completed_quests':sorted(set(int(x['completed_quest']) for x in samples if x['completed_quest'])),
        'last_world_active':bool(samples[-1]['world_active']),
        'last_appearance':{name:int(samples[-1][name]) for name in FIELDS[65:130]},
        'scenario_kind':int(samples[-1]['scenario_kind']),
        'disconnect_after_replay':any(not x['world_active'] for x in samples if x['replay_frame']>=1988),
        'timing_scope':'xemu guest clock; not physical Xbox performance',
        'input_scope':('offline GPU fixture with synthetic units' if any(x.get('portrait_fixture',0) for x in samples) else 'automated controller scenario' if any(x['scenario_stage'] for x in samples) else 'automated input replay' if last_replay else 'live input / neutral capture')+'; not physical controller verification'}
    summary['replay_acceptance']=bool(slots==set(range(24)) and saved and last_replay==1988 and
        summary['minimum_free_kib']>=8192 and summary['maximum_asset_failures']==0 and
        summary['world_revisions']==[1,2] and summary['last_world_active'] and not summary['disconnect_after_replay'])
    profiled=[sample for sample in samples if sample['profile_work_ms']>0]
    if profiled:
        summary['profile']={}
        for field in PROFILE_FIELDS:
            values=sorted(sample[field] for sample in profiled)
            summary['profile'][field]={'mean':sum(values)/len(values),'p50':values[(len(values)-1)//2],'p95':values[int((len(values)-1)*.95)],'max':max(values)}
        summary['profile_scope']='Current-frame guest millisecond wall times, including GPU waits where noted. Eight work phases sum to work; pace precedes this work. frame_ms instead measures the preceding frame-start interval. Network worker CPU and telemetry overhead are not separately measured.'
    summary['combat_acceptance']=bool(summary['scenario_kind'] in (1,2) and summary['scenario_last_stage']==8 and summary['maximum_kills']>=1 and summary['maximum_looted_items']>=1 and summary['minimum_free_kib']>=8192 and summary['maximum_asset_failures']==0 and summary['world_revisions']==[1,2] and summary['last_world_active'])
    summary['turnin_acceptance']=bool(783 in summary['completed_quests'] and summary['minimum_free_kib']>=8192 and summary['maximum_asset_failures']==0 and summary['last_world_active'])
    summary['kobold_turnin_acceptance']=bool(summary['scenario_kind']==4 and 7 in summary['completed_quests'] and summary['scenario_last_stage']==8 and summary['world_revisions']==[1,2] and summary['minimum_free_kib']>=8192 and summary['maximum_asset_failures']==0 and summary['last_world_active'])
    summary['boundary_acceptance']=bool(summary['scenario_kind']==3 and summary['scenario_last_stage']==8 and (0,32,48) in summary['tiles_seen'] and (0,32,49) in summary['tiles_seen'] and summary['end_position'][0]<-9080 and summary['maximum_region_failures']==0 and summary['maximum_asset_failures']==0 and summary['minimum_free_kib']>=8192 and summary['world_revisions']==[1,2] and summary['last_world_active'])
    args.output.with_suffix('.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps(summary,indent=2))
    if args.expect_kobold_turnin and not summary['kobold_turnin_acceptance']:raise SystemExit('Kobold quest turn-in acceptance failed; inspect the capture.')
    if args.expect_boundary and not summary['boundary_acceptance']:raise SystemExit('Terrain boundary acceptance failed; inspect the capture.')
    if args.expect_turnin and not summary['turnin_acceptance']:raise SystemExit('Quest turn-in acceptance failed; inspect the capture.')
    if args.expect_combat and not summary['combat_acceptance']:raise SystemExit('Combat acceptance failed; inspect the capture.')
    if args.expect_replay and not summary['replay_acceptance']:raise SystemExit('Replay acceptance failed; inspect the capture.')
if __name__=='__main__':main()
