"""Strict WXM1 world-render material submissions (not pixel correctness)."""
import struct
FIELDS='time_ms frame fixture'.split()+[f'blend_{i}' for i in range(7)]+['flags','fog','motion_draws','motion_hash','liquid_draws','sequence_draws','sequence_frames']
def decode(data):
    if (len(data),data[:4]) not in ((52,b'WXM1'),(60,b'WXM2'),(72,b'WXM3')):return None
    row=struct.unpack('<'+str((len(data)-4)//4)+'I',data[4:])
    total=sum(row[3:10])
    if row[2]!=13 or total>320 or row[10]&~(2|0x3f80|0x60000|0x180000) or row[11]>1:return None
    if len(row)>=14 and row[12]>total:return None
    if len(row)==17 and (row[14]>row[15] or row[15]>total or bool(row[15])!=bool(row[16])):return None
    return row+(-1,)*(len(FIELDS)-len(row))
