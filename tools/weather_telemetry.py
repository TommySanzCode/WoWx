"""Strict WXW1 weather-state companion; no main-packet growth."""
import math,struct
FIELDS='time_ms frame fixture phase type sound instant revision valid mix_revision mix_type snap environment errors grade weight'.split()
def decode(data):
    if len(data)!=68 or data[:4]!=b'WXW1':return None
    v=struct.unpack('<14I2f',data[4:])
    if (v[2],v[3])!=(0,0) and not (v[2]==10 and v[3]<=9):return None
    if v[4]>3 or v[6]>1 or v[8]>1 or v[10]>3 or v[11]>1 or v[12]>2:return None
    if any(not math.isfinite(x) or not 0<=x<=1 for x in v[14:]):return None
    return v
