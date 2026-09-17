"""Encrypted character picker/create/delete/login fixtures; loopback synthetic data only."""
import hashlib,socket,struct,subprocess,threading,time
from pathlib import Path
from world_protocol import Cipher,KEY
from auth_protocol import receive
ROOT=Path(__file__).resolve().parents[1]
def entry(guid,name):
    appearance=bytes([1,1,0,0,0,0,0,0,1])
    return struct.pack('<Q',guid)+name.encode()+b'\0'+appearance+struct.pack('<IIfffIIBIII',12,0,-8949.95,-132.493,83.531,0,0,1,0,0,0)+bytes(100)
def case(mode,success):
    errors=[]
    with socket.socket() as listener:
        listener.bind(('127.0.0.1',0));listener.listen(1);listener.settimeout(5)
        def serve():
            try:
                connection,_=listener.accept()
                with connection:
                    connection.settimeout(38 if mode=='keepalive' else 5);enc=Cipher();dec=Cipher();encrypted=False
                    def send(opcode,body=b''):
                        header=struct.pack('>H',len(body)+2)+struct.pack('<H',opcode)
                        if encrypted:header=enc.transform(header,True)
                        for value in header:connection.sendall(bytes([value]))
                        connection.sendall(body)
                    def read():
                        header=receive(connection,6)
                        if encrypted:header=dec.transform(header,False)
                        length=int.from_bytes(header[:2],'big');assert 4<=length<=128
                        return int.from_bytes(header[2:],'little'),receive(connection,length-4)
                    send(0x1ec,struct.pack('<I',0x12345678));opcode,auth=read();assert opcode==0x1ed
                    end=auth.index(0,8);assert auth[8:end]==b'FIXTURE'
                    digest=hashlib.sha1(b'FIXTURE'+bytes(4)+auth[end+1:end+5]+struct.pack('<I',0x12345678)+KEY).digest()
                    assert auth[end+5:end+25]==digest;encrypted=True;send(0x1ee,b'\x0c');assert read()==(0x37,b'')
                    create=mode.startswith('create');deleting=mode.startswith('delete_')
                    delete_guid=0x123456789abcdef0
                    roster=b'\2'+entry(delete_guid if deleting else 1,'Xboxer')+entry(2,'Second')
                    if mode=='delete_last':roster=b'\1'+entry(delete_guid,'Xboxer')
                    send(0x3b,b'\0' if create else roster)
                    if mode=='cancel':assert connection.recv(1)==b'';return
                    if mode=='refresh_bad':assert read()==(0x37,b'');send(0x3b,b'\1');return
                    if deleting:
                        # Exactly eight little-endian bytes, including the high GUID word.
                        assert read()==(0x38,struct.pack('<Q',delete_guid))
                        if mode=='delete_disconnect':return
                        if mode=='delete_timeout':
                            connection.settimeout(18);started=time.monotonic()
                            assert connection.recv(1)==b'','Deletion was retried without a confirmed response'
                            assert 13<=time.monotonic()-started<18;return
                        if mode=='delete_empty':send(0x3c);return
                        if mode=='delete_extra':send(0x3c,b'\x39\0');return
                        if mode=='delete_zero':send(0x3c,b'\0');return
                        if mode=='delete_modern_code':send(0x3c,b'\x47');return
                        if mode=='delete_progress':send(0x3c,b'\x38');return
                        if mode=='delete_wrong_reply':send(0x3a,b'\x2e');return
                        if mode=='delete_early_roster':send(0x3b,roster);return
                        result=0x3a if mode=='delete_refused' else 0x3b if mode=='delete_transfer' else 0x39
                        send(0x3c,bytes([result]));assert read()==(0x37,b'')
                        if mode=='delete_refresh_disconnect':return
                        if mode=='delete_duplicate':send(0x3c,b'\x39');return
                        if mode=='delete_bad_roster':send(0x3b,b'\1');return
                        if mode=='delete_pong':send(0x1dd,struct.pack('<I',123))
                        send(0x3b,roster if result!=0x39 or mode=='delete_still_present' else
                             b'\0' if mode=='delete_last' else b'\1'+entry(2,'Second'))
                        # No automatic repeat, even after refusal or an empty roster.
                        assert connection.recv(1)==b'';return
                    if create:
                        assert read()==(0x36,b'Newhero\0'+bytes([1,1,0,0,0,0,0,0,0]))
                        if mode=='create_empty':send(0x3a);return
                        if mode=='create_extra':send(0x3a,b'\x2e\0');return
                        if mode=='create_invalid':send(0x3a,b'\0');return
                        if mode=='create_retry':
                            send(0x3a,b'\x31');assert read()==(0x36,b'Goodname\0'+bytes([1,1,0,0,0,0,0,0,0]))
                        send(0x3a,b'\x2e');assert read()==(0x37,b'')
                        send(0x3b,b'\1'+entry(2,'Goodname' if mode=='create_retry' else 'Newhero'))
                    if mode=='keepalive':
                        opcode,body=read();assert opcode==0x1dc and len(body)==8;send(0x1dd,body[:4])
                    recovering=mode.startswith('login_recover_')
                    assert read()==(0x3d,struct.pack('<Q',1 if recovering else 2))
                    if recovering:
                        send(0x41,bytes([int(mode.rsplit('_',1)[1],16)]));assert read()==(0x37,b'')
                        send(0x3b,b'\2'+entry(1,'Xboxer')+entry(2,'Second'))
                        assert read()==(0x3d,struct.pack('<Q',2))
                    if mode=='login_rejected':send(0x41,b'\x41');return
                    if mode=='login_empty':send(0x41);return
                    if mode=='login_extra':send(0x41,b'\x41\0');return
                    if mode=='login_progress':send(0x41,b'\x3c');return
                    if mode=='login_success_error':send(0x41,b'\x3d');return
                    if mode=='login_invalid':send(0x41,b'\x45');return
                    if mode=='login_wrong_reply':send(0x3c,b'\x39');return
                    send(0x236,struct.pack('<Iffff',0,-8949.95,-132.493,83.531,0))
                    assert read()==(0x26a,struct.pack('<Q',2));send(0x4d)
            except (ConnectionResetError,BrokenPipeError):pass
            except BaseException as error:errors.append(error)
        thread=threading.Thread(target=serve,daemon=True);thread.start()
        result=subprocess.run([str(ROOT/'build/host/wowx_lobby_fixture.exe'),str(listener.getsockname()[1]),mode],capture_output=True,text=True,timeout=45)
        thread.join(5);assert not thread.is_alive() and not errors,(mode,errors)
        assert (result.returncode==0)==success,(mode,result.stdout,result.stderr)
        assert result.returncode!=2,(mode,'probe invariant failed',result.stdout,result.stderr)
        if not success:assert 'phase=4' in result.stdout
        if mode.startswith('delete_') and not success:assert 'code=256' in result.stdout
        if mode=='refresh_bad':assert 'roster=2' in result.stdout
        print(mode+': '+result.stdout.strip(),flush=True)
def management():
    for mode in ('delete_success','delete_last','delete_refused','delete_transfer','delete_pong'):case(mode,True)
    for mode in ('delete_disconnect','delete_timeout','delete_empty','delete_extra','delete_zero','delete_modern_code','delete_progress',
                 'delete_wrong_reply','delete_early_roster','delete_refresh_disconnect','delete_duplicate',
                 'delete_bad_roster','delete_still_present'):case(mode,False)
    for code in range(0x3e,0x45):case(f'login_recover_{code:x}',True)
    for mode in ('login_empty','login_extra','login_progress','login_success_error','login_invalid','login_wrong_reply'):case(mode,False)

if __name__=='__main__':
    for mode in ('select','create','create_retry','cancel','keepalive'):case(mode,True)
    for mode in ('create_empty','create_extra','create_invalid','refresh_bad','login_rejected'):case(mode,False)
    management()
    print('41 encrypted character management scenarios passed.')
