"""Bounded WXF1 action/cast telemetry companion; no growth of the WXTZ datagram."""
import struct
FIELDS='time_ms frame fixture step cast_spell cast_phase cast_elapsed cast_duration cast_remaining cast_delay cast_infinite cast_revision'.split()
FIELDS += [f'action_{slot}_{field}' for slot in range(8) for field in ('flags','cost','current','distance_milli','count','charges')]
FIELDS += 'catalog_version cast_state_bytes spellbook_bytes cooldown_state_bytes inventory_bytes'.split()

def decode(data):
    if len(data)!=264 or data[:4]!=b'WXF1':return None
    values=struct.unpack('<65I',data[4:])
    if values[5]>5 or values[10]>1 or values[3]>120 or values[4]>65535:return None
    return values
