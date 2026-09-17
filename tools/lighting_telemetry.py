"""WXL2/3 atmosphere companion. WXTZ stays within one Ethernet MTU."""
import math
import struct
FIELDS='time_ms frame fixture phase condition authored volume0 volume1 profile0 profile1 global_profile catalog_bytes failures enabled environment rgb light_ms time_half_minutes fog_start fog_end'.split()
FIELDS+='palette_mask sky_quads ambient diffuse sky_top sky_middle sky_band1 sky_band2 sky_smog sun direction_x direction_y direction_z wire_version'.split()
def decode(data):
    if len(data)==84 and data[:4]==b'WXL2':v=struct.unpack('<17I3f',data[4:])+(0,)*13+(2,)
    elif len(data)==136 and data[:4]==b'WXL3':
        v=struct.unpack('<17I3f10I3f',data[4:])+(3,)
        if v[20]>255 or v[21] not in (0,192) or any(x>0xffffff for x in v[22:30]):return None
        if any(not math.isfinite(x) for x in v[30:33]) or not .98<sum(x*x for x in v[30:33])<1.02:return None
    else:return None
    if v[2] not in (0,9,10,13) or v[3]>(31 if v[2]==13 else 9 if v[2]==10 else 7) or v[4]>2 or v[5]>1 or v[13]>1 or v[14]>2:return None
    if any(not math.isfinite(x) for x in v[17:20]) or not 0<=v[17]<2880 or not -640<=v[18]<v[19]<=160 or v[19]<=0:return None
    return v
