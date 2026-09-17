"""WXS1 streaming companion. Timings are guest milliseconds, not hardware results."""
import struct
from collections import Counter

def full_coverage(main,companion):
    """UDP loss or strict-decoder rejection must not conceal a failed budget.

    Only the terminal main sample may lack its subsequent companion. Duplicate
    or foreign identities fail even when all expected frames also appear.
    """
    main_ids=[(int(x['time_ms']),int(x['replay_frame'])) for x in main]
    ids=[(int(x['time_ms']),int(x['frame'])) for x in companion]
    required=set(main_ids[:-1]);seen=Counter(ids);allowed=set(main_ids)
    missing=sorted(required-seen.keys());foreign=sorted(seen.keys()-allowed)
    duplicate=sorted(k for k,n in seen.items() if n!=1)
    return {'passed':bool(required) and len(set(main_ids))==len(main_ids) and not(missing or foreign or duplicate),
            'expected':len(required),'received':len(ids),'missing':missing,'foreign':foreign,'duplicate':duplicate,
            'duplicate_main':len(main_ids)-len(set(main_ids))}
FIELDS='time_ms frame fixture stage cycles ready job_entry job_phase read_bytes read_ops scan_bytes pending_bytes completed cancelled max_read_bytes max_read_ops max_scan_bytes deferred region_ms stream_ms waits errors index_phase fog index_read_bytes index_read_ops index_checked pending_index_bytes index_bytes scene_bytes index_progress'.split()
FIELDS += ['budget_enabled']+[f'{who}_{field}' for who in ('total','index','world','npc','player') for field in ('budget_read_bytes','budget_read_ops','budget_scan_bytes','budget_allocations')]
FIELDS += ['clock_enabled','clock_yield_mask']+[f'{who}_{field}' for field in ('clock_limit_ms','clock_observed_ms') for who in ('index','world','npc','player')]
FIELDS += 'index_copy_bytes index_join_total index_join_progress index_join_bytes index_join_restarts'.split()
FIELDS += 'selection_entries selection_phase selection_progress selection_total selection_commits selection_restarts selection_revision scene_revision'.split()
def decode(data):
    if (len(data),data[:4]) not in ((128,b'WXS1'),(212,b'WXS2'),(252,b'WXS3'),(272,b'WXS4'),(304,b'WXS5')):return None
    values=struct.unpack('<'+str((len(data)-4)//4)+'I',data[4:])
    if values[2] not in (0,7,8,11,13) or values[3]>(31 if values[2]==13 else 8) or values[5]>1 or values[7]>15 or values[22]>(5 if len(values)>=67 else 2) or values[23]>1:return None
    if len(values)>=52:
        if values[31]>1:return None
        totals=values[32:36]
        if any(a>b for a,b in zip(totals,(65536,16,65536,3))):return None
        if any(totals[i]!=sum(values[36+j*4+i] for j in range(4)) for i in range(4)):return None
        if not values[31] and any(totals):return None
    if len(values)>=62:
        enabled,mask=values[52:54];limits=values[54:58];observed=values[58:62]
        if enabled>1 or mask>15 or any(n>12 for n in limits) or sum(limits) not in (0,12):return None
        if (not enabled or not values[31]) and any(values[52:62]):return None
        for i in range(4):
            if not limits[i] and (observed[i] or mask&(1<<i)):return None
            if bool(mask&(1<<i)) != bool(limits[i] and observed[i]>=limits[i]):return None
    if len(values)>=67:
        copied,total,progress,resident,restarts=values[62:67]
        if copied>262144 or total>8388608 or progress>total or resident not in (0,total):return None
        if any(n%64 for n in (copied,total,progress,resident)) or values[27]<resident:return None
        if values[22] in (4,5) and (not resident or not total):return None
        if values[22]==5 and progress!=total:return None
        if values[22] not in (4,5) and resident:return None
        if not values[31] and copied:return None
    if len(values)==75:
        entries,phase,progress,total,commits,restarts,revision,scene=values[67:]
        if entries>2048 or phase>1 or total>131072 or progress>total:return None
        if phase and (values[5] or revision!=scene):return None
        if not values[31] and entries:return None
    return values+(-1,)*(75-len(values))
ACTOR_FIELDS='time_ms frame fixture requested_clip actors_ready actors_fallback actors_drawn avatar_ready avatar_clip actor_ms avatar_loads actor_loads'.split()
ACTOR_FIELDS += [f'{who}_{field}' for who in ('actor','avatar') for field in 'read_bytes read_ops scan_bytes pending_bytes completed cancelled max_read_bytes max_read_ops max_scan_bytes deferred'.split()]
ACTOR_FIELDS += 'actor_bytes avatar_bytes actor_failures avatar_failures avatar_gaps switches actor_index_bytes avatar_job_phase'.split()
ACTOR_FIELDS += [f'{who}_{field}' for who in ('actor','avatar') for field in 'sampled skipped vertices palettes'.split()]
ACTOR_FIELDS += 'compose_phase compose_read_bytes compose_read_ops compose_work_pixels compose_cancelled compose_commits compose_failures compose_hash compose_revision compose_layer'.split()
def decode_actors(data):
    if (len(data),data[:4]) not in ((164,b'WXN1'),(196,b'WXN2'),(236,b'WXN3')):return None
    v=struct.unpack('<'+str((len(data)-4)//4)+'I',data[4:])
    if v[2] not in (0,8,11) or v[3]>255 or v[4]>32 or v[5]>32 or v[6]>32 or v[7]>1 or v[39]>15:return None
    if len(v)>=48:
        for start in (40,44):
            sampled,skipped,vertices,palettes=v[start:start+4]
            if sampled+skipped>320 or palettes>sampled or (sampled==0 and vertices):return None
    if len(v)==58 and (v[48]>8 or v[49]>65536 or v[50]>8 or v[51]>8192 or v[57]>80):return None
    return v+(-1,)*(58-len(v))
