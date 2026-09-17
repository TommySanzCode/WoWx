"""Exercise the exact host-built Xbox auth adapter against bounded fake peers."""
import hashlib
import socket
import struct
import subprocess
import tempfile
import threading
import time
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
PROBE=ROOT/'build/host/wowx_auth_probe.exe'
N=int('894B645E89E1535BBDAD5B8B290650530801B18EBFBF5E8FAB3C82872A3E9BB7',16)
H=lambda value:hashlib.sha1(value).digest()
def little(value,minimum=0):return value.to_bytes(max(minimum,(value.bit_length()+7)//8),'little')
salt=bytes(range(1,33));identity=H(b'FIXTURE:DISPOSABLE')
v=pow(7,int.from_bytes(H(salt+identity),'little'),N)
b=0x112233445566778899AABBCCDDEEFF123456789
B=(3*v+pow(7,b,N))%N
challenge=b'\0\0\0'+little(B,32)+b'\x01\x07\x20'+little(N,32)+salt+bytes(17)
def receive(connection,length):
    result=b''
    while len(result)<length:
        part=connection.recv(length-len(result))
        if not part:raise EOFError('Client closed early')
        result+=part
    return result
def send(connection,data,fragmented):
    if fragmented:
        for value in data:connection.sendall(bytes([value]))
    else:connection.sendall(data)

def parameters(salt_value=salt,secret=b):
    verifier=pow(7,int.from_bytes(H(little(int.from_bytes(salt_value,'little'))+identity),'little'),N)
    public=(3*verifier+pow(7,secret,N))%N
    packet=b'\0\0\0'+little(public,32)+b'\x01\x07\x20'+little(N,32)+salt_value+bytes(17)
    return salt_value,verifier,secret,public,packet

def proof_values(A,values):
    salt_value,verifier,secret,public,_=values
    u=int.from_bytes(H(little(A)+little(public)),'little')
    shared=pow(A*pow(verifier,u,N),secret,N);raw=little(shared,32)
    even,odd=H(raw[::2]),H(raw[1::2]);key=bytes(x for pair in zip(even,odd) for x in pair)
    natural=lambda raw:raw.rstrip(b'\0')
    xor=bytes(a^c for a,c in zip(H(little(N)),H(b'\7')))
    client=H(natural(xor)+H(b'FIXTURE')+natural(salt_value)+little(A)+little(public)+natural(key))
    server=H(little(A)+natural(client)+natural(key))
    return key,client,server

def case(name,change=None,after=None,expected=False,fragmented=False,values=None,ephemeral=None):
    values=values or parameters()
    salt_value,verifier,secret,public,packet=values
    errors=[]
    with socket.socket() as listener,tempfile.TemporaryDirectory(prefix='wowx-auth-') as directory:
        listener.bind(('127.0.0.1',0));listener.listen(1);listener.settimeout(8)
        port=listener.getsockname()[1]
        def serve():
            try:
                connection,_=listener.accept()
                with connection:
                    connection.settimeout(8)
                    header=receive(connection,4);payload=receive(connection,int.from_bytes(header[2:4],'little'))
                    assert header[:2]==b'\0\3' and payload[7:9]==struct.pack('<H',5875)
                    if after=='stall':time.sleep(1);return
                    send(connection,change(packet) if change else packet,fragmented)
                    if change:return
                    proof=receive(connection,75);assert proof[0]==1
                    A=int.from_bytes(proof[1:33],'little')
                    K,M1,M2=proof_values(A,values)
                    assert proof[33:53]==M1,'Client SRP proof differs from independent fixture'
                    if after=='proof':M2=bytes(20)
                    send(connection,b'\1\0'+M2+bytes(4),fragmented)
                    if after=='proof':return
                    assert receive(connection,5)==b'\x10\0\0\0\0'
                    realm=bytes(4)+b'\1'+bytes(5)+b'Fixture\0'+b'10.0.2.2:8086\0'+struct.pack('<fBBB',0,0,0,1)+b'\0\0'
                    if after=='flagged':realm=realm[:9]+b'\x04'+realm[10:]
                    if after=='offline':realm=realm[:9]+b'\x02'+realm[10:]
                    if after=='empty':realm=bytes(5)+b'\x02\0'
                    if after=='many':realm=bytes(4)+b'\xff'+realm[5:-2]*255+b'\x02\0'
                    if after=='nan':realm=realm[:-9]+struct.pack('<f',float('nan'))+realm[-5:]
                    if after=='length':send(connection,b'\x10\xff\xff',False);return
                    if after=='count':realm=realm[:4]+b'\xff'+realm[5:]
                    if after=='unterminated':realm=bytes(4)+b'\1'+bytes(5)+b'X'*300
                    if after=='trailing':realm+=b'\0'
                    send(connection,b'\x10'+struct.pack('<H',len(realm))+realm,fragmented)
            except (ConnectionResetError,BrokenPipeError):pass
            except BaseException as error:errors.append(error)
        worker=threading.Thread(target=serve,daemon=True);worker.start()
        config=Path(directory)/'fixture.bin'
        config.write_bytes(struct.pack('<64s32s32sI',b'127.0.0.1',b'FIXTURE',b'DISPOSABLE',port))
        result=subprocess.run([str(PROBE),str(config)]+(['--cancel-auth'] if after=='stall' else ["--fixture-a="+little(ephemeral,19).hex()] if ephemeral is not None else []),capture_output=True,text=True,timeout=20)
        if after=='stall':assert 'cancelled' in result.stdout,(name,result.stdout)
        worker.join(timeout=8)
        assert not worker.is_alive(),name+': server did not stop'
        assert not errors,(name,errors)
        assert (result.returncode==0)==expected,(name,result.stdout,result.stderr)
        print(name+': '+result.stdout.strip())

def main():
    case('valid fragmented Vanilla authentication',expected=True,fragmented=True)
    case('Vanilla build flag without extension',after='flagged',expected=True,fragmented=True)
    case('offline realm cannot be selected',after='offline',expected=True)
    case('empty list remains authenticated',after='empty',expected=True)
    case('255 realms beyond old 4096-byte limit',after='many',expected=True)
    case('nonfinite realm population',after='nan')
    case('cancel a stalled login',after='stall')
    case('truncated challenge',lambda c:c[:50])
    case('oversized generator',lambda c:c[:35]+b'\xff')
    case('unsupported modulus length',lambda c:c[:37]+b'\xff')
    case('zero public ephemeral',lambda c:c[:3]+bytes(32)+c[35:])
    case('public ephemeral equal to modulus',lambda c:c[:3]+little(N,32)+c[35:])
    case('modified modulus',lambda c:c[:38]+bytes(32)+c[70:])
    case('unsupported security extension',lambda c:c[:118]+b'\x04')
    case('invalid server proof',after='proof')
    for mode in ['length','count','unterminated','trailing']:case('invalid realm '+mode,after=mode)
    # Search deterministic public ephemerals for each rare width boundary. The
    # fixture applies the selected server's independent BigNumber hash rules.
    values=parameters();found={}
    for a in range(1,10000):
        A=pow(7,a,N);key,client,_=proof_values(A,values)
        if key[-1]==0:found.setdefault('short session key',a)
        if client[-1]==0:found.setdefault('short client proof',a)
        if len(little(A))==31:found.setdefault('short public ephemeral',a)
        if len(found)==3:break
    assert len(found)==3
    for label,a in found.items():case(label,expected=True,values=values,ephemeral=a)
    case('short salt',expected=True,values=parameters(salt[:-1]+b'\0'),ephemeral=73)
    for secret in range(1,10000):
        values=parameters(secret=secret)
        if len(little(values[3]))==31:break
    else:raise AssertionError('No short server ephemeral found')
    case('short server ephemeral',expected=True,values=values,ephemeral=79)
    print('24 auth protocol scenarios passed.')

if __name__=='__main__':main()
