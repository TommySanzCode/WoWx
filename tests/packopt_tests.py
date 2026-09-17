"""Exercise repacking against real file boundaries, versions and output protection."""
import importlib.util
import struct
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORK = Path(tempfile.mkdtemp(prefix='packopt-', dir=ROOT / 'build'))
EXE = ROOT / 'build/host/wowx_packopt.exe'
spec = importlib.util.spec_from_file_location('compress_world', ROOT / 'scripts/compress-world.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
checks = 0
import sys
sys.path.insert(0,str(ROOT/'tools'))
from material_probe import pack as motion_probe

def check(value):
    global checks
    checks += 1
    assert value, f'Check {checks} failed'

def fixture(name, width=4, height=4, kind=1):
    # One finite triangle and an opaque red texture; v4 remains supported.
    texture = bytes((0, 0, 255, 255)) * width * height
    vertices = struct.pack('<24f', 0,0,0, 0,0,1, 0,0, 1,0,0, 0,0,1, 1,0, 0,1,0, 0,0,1, 0,1)
    indices = struct.pack('<3H', 0, 1, 2)
    data = struct.pack('<4sIII3fI', b'WXP1', 4, 1, 64, 0,0,0, 198 + len(texture))
    data += struct.pack('<9I4f3I', kind, 7, 3, 3, 96, 192, 198, width, height, 0,0,0, 2, 0,0,0)
    data += vertices + indices + texture
    path = WORK / name
    path.write_bytes(data)
    return path

def run(source, target, success):
    result = subprocess.run([str(EXE), str(source), str(target)], capture_output=True, text=True)
    check((result.returncode == 0) == success)
    if success:
        check(module.geometry_matches(source, target) == 1)
    else:
        check(not Path(str(target) + '.tmp').exists())
    return result

source = fixture('source.wxp')
target = WORK / 'compressed.wxp'
run(source, target, True)
check(target.stat().st_size == 206)
check(struct.unpack_from('<I', target.read_bytes(), 32 + 13 * 4)[0] == 32)
saved = target.read_bytes()
run(source, target, False)
check(target.read_bytes() == saved)
run(source, source, False)
check(source.read_bytes()[4:8] == struct.pack('<I', 4))
run(fixture('character.wxp', kind=3), WORK / 'character-out.wxp', False)
check(not (WORK / 'character-out.wxp').exists())
run(fixture('nonsquare.wxp', width=4, height=2), WORK / 'nonsquare-out.wxp', True)
check(struct.unpack_from('<I', (WORK / 'nonsquare-out.wxp').read_bytes(), 84)[0] == 0)
bad = WORK / 'truncated.wxp'
bad.write_bytes(source.read_bytes()[:-1])
run(bad, WORK / 'bad-out.wxp', False)
check(not (WORK / 'bad-out.wxp').exists())
run(target, WORK / 'again.wxp', True)
check((WORK / 'again.wxp').read_bytes() == saved)
for mode in range(2,7):
    material=fixture(f'material-{mode}.wxp',kind=2);blob=bytearray(material.read_bytes())
    flags=(mode<<7)|0x3c00
    struct.pack_into('<I',blob,4,6);struct.pack_into('<I',blob,84,flags);material.write_bytes(blob)
    output=WORK/f'material-{mode}-dxt.wxp';run(material,output,True)
    check(struct.unpack_from('<I',output.read_bytes(),4)[0]==8)
    check(struct.unpack_from('<I',output.read_bytes(),84)[0]==flags|32)
motion=WORK/'motion.wxp';motion.write_bytes(motion_probe(True));result=subprocess.run([str(EXE),str(motion),str(WORK/'motion-out.wxp')],capture_output=True,text=True)
check(result.returncode==0);check(module.geometry_matches(motion,WORK/'motion-out.wxp')==16)
for animated in (False,True):
    lighting=WORK/f'lighting-{animated}.wxp';lighting.write_bytes(motion_probe(animated,True));target=WORK/f'lighting-out-{animated}.wxp'
    result=subprocess.run([str(EXE),str(lighting),str(target)],capture_output=True,text=True)
    check(result.returncode==0);check(module.geometry_matches(lighting,target)==16)
    blob=bytearray(target.read_bytes());entry=struct.unpack_from('<16I',blob,96)
    offset=entry[4]-(2652 if animated else 0)-entry[2]*4;blob[offset]^=255
    changed=WORK/f'lighting-changed-{animated}.wxp';changed.write_bytes(blob)
    rejected=False
    try:module.geometry_matches(lighting,changed)
    except ValueError:rejected=True
    check(rejected)
print(f'{checks} pack optimizer integration checks passed; fixtures retained in {WORK}')

# A v9 world footer is part of the geometry identity, including after repacking.
env_source=fixture('environment-source.wxp')
b=bytearray(env_source.read_bytes());offset=len(b)
r=[1,9,0x2000,0]+[1,0,0,0,0,1,0,0,0,0,1,0]+[-2,-2,-2,2,2,2]+[0]*10
blob=struct.pack('<4I18f4I4f2I',*r)
b+=blob+struct.pack('<4sIII',b'WXE1',1,len(blob),offset)
struct.pack_into('<I',b,4,9);struct.pack_into('<I',b,28,len(b));env_source.write_bytes(b)
env_target=WORK/'environment-compressed.wxp';run(env_source,env_target,True)
z=env_target.read_bytes();check(z[-144:-16]==blob)
b[-16]=0;bad_env=WORK/'environment-invalid.wxp';bad_env.write_bytes(b)
run(bad_env,WORK/'environment-invalid-output.wxp',False)
print(f'{checks} checks including environment footer preservation passed')

# Thirty aligned texture frames remain independently addressable after DXT.
seq=fixture('sequence-source.wxp',kind=2);b=bytearray(seq.read_bytes()[:198]);frames=bytearray()
for frame in range(30):
    # One mip, 4x4, with explicit 128-byte GPU alignment between frames.
    frames+=bytes((0,0,frame*8,255))*16+bytes(64)
b+=frames;offset=len(b);b+=struct.pack('<4sIII',b'WXE1',0,0,offset)
struct.pack_into('<I',b,4,10);struct.pack_into('<I',b,28,len(b))
struct.pack_into('<3I',b,84,0x180000,0,(30<<16)|1);seq.write_bytes(b)
seq_out=WORK/'sequence-dxt.wxp';run(seq,seq_out,True);z=seq_out.read_bytes()
check(struct.unpack_from('<I',z,4)[0]==10);entry=struct.unpack_from('<16I',z,32)
check(entry[13]==0x180020 and entry[15]==(30<<16)|1)
colors=[]
for frame in range(30):
    block=z[entry[6]+frame*128:entry[6]+(frame+1)*128];check(len(block)==128)
    c0,c1,indices=struct.unpack_from('<HHI',block)
    a,b=(c0>>11)*255/31,(c1>>11)*255/31
    palette=[a,b,(2*a+b)/3,(a+2*b)/3]
    colors.append(palette[indices&3]);check(not any(block[8:]))
check(all(abs(value-frame*8)<=3 for frame,value in enumerate(colors)))
again=WORK/'sequence-again.wxp';run(seq_out,again,True);check(again.read_bytes()==z)
print(f'{checks} checks including aligned animated texture preservation passed')
