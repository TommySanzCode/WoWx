"""Strict WXQ1 offline asynchronous character-profile telemetry."""
import struct
FIELDS=('time_ms frame fixture stage cycles completed expected_failures errors work_ms drawn '
        'open_phase pack_phase profile_ready avatar_ready race sex requested_race requested_sex '
        'attempts changes failures pending cancelled open_read_bytes open_read_ops open_scan_bytes '
        'pending_bytes avatar_bytes animation_index_reads compose_phase compose_read_bytes compose_read_ops '
        'compose_work_pixels asset_failures parts atlas_hash budget_enabled '
        'total_read_bytes total_read_ops total_scan_bytes total_allocations '
        'avatar_read_bytes avatar_read_ops avatar_scan_bytes avatar_allocations '
        'payload_read_bytes payload_read_ops payload_scan_bytes index_bytes animation_count animation_reads compose_failures').split()
def decode(data):
    if len(data)!=212 or data[:4]!=b'WXQ1':return None
    v=struct.unpack('<52I',data[4:])
    if v[2]!=12 or v[3]>19 or v[5]>65535 or v[10]>8 or v[11]>5 or v[12]>1 or v[13]>1 or v[14]>8 or v[15]>1 or not 1<=v[16]<=8 or v[17]>1 or v[21]>2 or v[29]>8 or v[34]>32 or v[36]!=1:return None
    for at,limits in ((23,(65536,8,65536)),(30,(65536,8,8192)),(37,(65536,16,65536,3)),(45,(65536,8,65536))):
        if any(value>limit for value,limit in zip(v[at:],limits)):return None
    if v[37:41]!=v[41:45]:return None
    return v
