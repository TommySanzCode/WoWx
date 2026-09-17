"""WXY1 bounded world-environment residency and camera classification."""
import math,struct
FIELDS='time_ms frame fixture stage cycles expected_environment expected_liquid environment known records liquid_type group source bytes pending errors sample_ms ready depth camera_x camera_y camera_z'.split()
def decode(data):
    if len(data)!=92 or data[:4]!=b'WXY1':return None
    v=struct.unpack('<18I4f',data[4:])
    if v[2] not in (0,13) or v[3]>=32 or v[5] not in (0,1,2,0xffffffff) or v[7]>2 or v[8]>1 or v[9]>8192 or v[12] not in (0,1,2,3,0xffffffff) or v[13]>8*1024*1024 or v[14]>4 or v[17]>1:return None
    if any(not math.isfinite(x) for x in v[18:]) or not 0<=v[18]<=200000 or any(abs(x)>100000 for x in v[19:]):return None
    return v
