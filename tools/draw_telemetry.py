"""WXD1 geometry submission wall times and explicit pushbuffer waits."""
import struct
FIELDS=('time_ms frame fixture scope submit_ms flush_wait_ms flush_reset_ms drain_ms flushes '
        'batches indices largest_batch legacy_batches world_ms actors_ms player_ms liquid_ms').split()

def decode(data):
    if len(data)!=72 or data[:4]!=b'WXD1':return None
    values=struct.unpack('<17I',data[4:]);v=dict(zip(FIELDS,values))
    if v['fixture'] not in (0,7,8,11,13) or v['scope']!=(2 if not v['fixture'] else 1):return None
    if v['submit_ms']!=sum(v[k] for k in ('world_ms','actors_ms','player_ms','liquid_ms')):return None
    if v['flush_wait_ms']+v['flush_reset_ms']>v['submit_ms']:return None
    if not v['flushes'] and (v['flush_wait_ms'] or v['flush_reset_ms']):return None
    if v['largest_batch']>1536 or v['largest_batch']%3 or v['indices']%3:return None
    if not v['batches']:
        if v['indices'] or v['largest_batch'] or v['legacy_batches']:return None
    elif not (3<=v['largest_batch']<=v['indices']<=v['batches']*v['largest_batch'] and
              v['batches']*3<=v['indices'] and v['batches']<=v['legacy_batches']<=v['indices']//3):return None
    return values
