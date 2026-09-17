"""Independent encrypted Vanilla world fixtures, using only a public fixture key."""
import hashlib
import socket
import struct
import subprocess
import threading
import time
from pathlib import Path
from auth_protocol import receive
ROOT=Path(__file__).resolve().parents[1]
KEY=bytes(range(1,41))

def player_update(fields,create=False):
    blocks=max(fields)//32+1;masks=[0]*blocks
    for field in fields:masks[field//32]|=1<<(field%32)
    return struct.pack('<IB',1,0)+bytes([2 if create else 0,1,1])+(bytes([4,0]) if create else b'')+bytes([blocks])+struct.pack('<'+'I'*blocks,*masks)+b''.join(struct.pack('<I',fields[f]) for f in sorted(fields))

def item_metadata(entry,display,kind):
    return struct.pack('<III',entry,4,6)+b'Fixture item\0\0\0\0'+struct.pack('<6I',display,0,0,5,1,kind)+bytes((14+20+15+7+3+30+1)*4)+b'Fixture description\0'+bytes(14*4)

class Cipher:
    def __init__(self,key=KEY):self.index=self.previous=0;self.key=key
    def transform(self,data,encrypt):
        result=bytearray()
        for value in data:
            out=((value^self.key[self.index])+self.previous)&255 if encrypt else ((value-self.previous)&255)^self.key[self.index]
            self.previous=out if encrypt else value;self.index=(self.index+1)%len(self.key);result.append(out)
        return bytes(result)

def case(mode,success=False):
    key=KEY[:-1] if mode=='short_key' else KEY
    errors=[]
    with socket.socket() as listener:
        listener.bind(('127.0.0.1',0));listener.listen(1);listener.settimeout(5)
        def serve():
            try:
                connection,_=listener.accept()
                with connection:
                    connection.settimeout(5);send_cipher=Cipher(key);receive_cipher=Cipher(key);encrypted=False
                    def send(opcode,body=b'',length=None):
                        header=struct.pack('>H',len(body)+2 if length is None else length)+struct.pack('<H',opcode)
                        if encrypted:header=send_cipher.transform(header,True)
                        # Deliberately split every header, exercising receiveExact + cipher boundaries.
                        for value in header:connection.sendall(bytes([value]))
                        connection.sendall(body)
                    def read():
                        header=receive(connection,6)
                        if encrypted:header=receive_cipher.transform(header,False)
                        length=int.from_bytes(header[:2],'big');opcode=int.from_bytes(header[2:],'little')
                        assert 4<=length<=1024
                        return opcode,receive(connection,length-4)
                    if mode=='short_header':connection.sendall(b'\0\6');return
                    send(0x1ec,struct.pack('<I',0x12345678))
                    opcode,auth=read();assert opcode==0x1ed
                    assert struct.unpack_from('<II',auth)==(5875,1)
                    end=auth.index(0,8);assert auth[8:end]==b'FIXTURE'
                    seed=auth[end+1:end+5]
                    digest=hashlib.sha1(b'FIXTURE'+bytes(4)+seed+struct.pack('<I',0x12345678)+key).digest()
                    assert auth[end+5:end+25]==digest and auth[end+25:]==bytes(4)
                    encrypted=True
                    if mode=='invalid_size':send(0x1ee,length=1);return
                    if mode=='truncated_body':send(0x1ee,b'\x0c',length=8);return
                    send(0x1ee,b'\x0c' if mode!='rejected' else b'\x0d')
                    if mode=='rejected':return
                    assert read()==(0x37,b'')
                    if mode=='empty_list':
                        send(0x3b,b'\0');opcode,body=read();assert opcode==0x36
                        assert body==b'Xboxer\0'+bytes([1,1,0,0,0,0,0,0,0]),'Incorrect Vanilla create layout'
                        send(0x3a,b'\x2e');assert read()==(0x37,b'')
                    if mode=='count':send(0x3b,b'\xff');return
                    if mode=='name_terminator':send(0x3b,b'\1'+struct.pack('<Q',1)+b'Xboxer');return
                    appearance=bytes([1,1,0,2,3,4,5,6,1])
                    equipment=bytes(95)+struct.pack('<IB',0x12345,18)
                    fields=appearance+struct.pack('<IIfffIIBIII',12,0,-8949.95,-132.493,83.531,0,0,1,0,0,0)+equipment
                    characters=b'\1'+struct.pack('<Q',1)+b'Xboxer\0'+fields
                    if mode=='truncated_equipment':send(0x3b,characters[:-1]);return
                    if mode=='trailing_characters':send(0x3b,characters+b'\0');return
                    send(0x3b,characters)
                    assert read()==(0x3d,struct.pack('<Q',1))
                    if mode=='cold_map_login':time.sleep(16)
                    if mode in ('clock_early','clock_bad_early'):
                        send(0x42,struct.pack('<If',23*64+59,0) if mode=='clock_early' else bytes(7))
                        if mode=='clock_bad_early':return
                    if mode in ('weather_early','weather_early_bad'):
                        send(0x2f4,struct.pack('<IfIB',1,.25 if mode=='weather_early' else float('nan'),8533,0))
                        if mode=='weather_early_bad':return
                    x=float('nan') if mode=='nan_position' else -8949.95
                    send(0x236,struct.pack('<Iffff',0,x,-132.493,83.531,0))
                    if mode=='nan_position':return
                    if mode.startswith('weather_'):
                        assert read()==(0x26a,struct.pack('<Q',1))
                        wire=struct.pack('<IfIB',1,.25,8533,0)
                        bad={'weather_short8':wire[:8],'weather_tbc9':wire[:9],'weather_short12':wire[:12],'weather_trailing':wire+b'\0',
                             'weather_type':struct.pack('<IfIB',4,.25,8533,0),'weather_nan':struct.pack('<IfIB',1,float('nan'),8533,0),
                             'weather_negative':struct.pack('<IfIB',1,-.1,8533,0),'weather_over1':struct.pack('<IfIB',1,1.1,8533,0),'weather_flag':struct.pack('<IfIB',1,.25,8533,2)}
                        if mode in bad:send(0x2f4,bad[mode]);return
                        if mode!='weather_early':send(0x2f4,wire)
                        next_states=[(1,.9,8535,0),(2,.65,8538,1),(3,.8,8558,0),(0,0.,0,0),(1,1.,8535,1)]
                        for step in range(8):
                            opcode,body=read();assert opcode==0xee and struct.unpack_from('<fff',body,8)==(-8900+step,-160,82),'Weather snapshot not published correctly'
                            if step<5:send(0x2f4,struct.pack('<IfIB',*next_states[step]))
                            elif step in (5,6):
                                send(0x3f,struct.pack('<I',0))
                                if step==6:send(0x2f4,struct.pack('<IfIB',3,.5,8556,1))
                                send(0x3e,struct.pack('<Iffff',0,-8900,-160,82,0))
                                assert read()==(0xdc,b''),'Map transfer acknowledgement missing'
                        send(0x4d);return
                    if mode.startswith('clock_'):
                        assert read()==(0x26a,struct.pack('<Q',1))
                        if mode=='clock_short':send(0x42,bytes(7));return
                        if mode=='clock_nan':send(0x42,struct.pack('<If',0,float('nan')));return
                        if mode=='clock_calendar':send(0x42,struct.pack('<If',24*64,0));return
                        if mode!='clock_early':send(0x42,struct.pack('<If',23*64+59,0))
                        for stage in range(2):
                            opcode,body=read();assert opcode==0xee and struct.unpack_from('<fff',body,8)==(-8900+stage,-160,82)
                            if not stage:send(0x42,struct.pack('<If',6*64+30,0))
                        send(0x4d);return
                    if mode.startswith('cast_'):
                        assert read()==(0x26a,struct.pack('<Q',1))
                        start=b'\1\1\1\1'+struct.pack('<IHIH',133,0,3000,0)
                        if mode=='cast_short_channel':send(0x139,struct.pack('<II',689,3000)[:-1]);return
                        if mode=='cast_bad_channel_time':send(0x139,struct.pack('<II',689,0xfffffffe));return
                        if mode=='cast_short_delay':send(0x1e2,struct.pack('<QI',1,500)[:-1]);return
                        send(0x131,start)
                        for stage in range(11):
                            opcode,body=read()
                            if stage in (3,5):assert (opcode,body)==(0x12f if stage==3 else 0x13b,struct.pack('<I',133 if stage==3 else 689)), 'Wrong Vanilla cancel command'
                            else:assert opcode==0xee and struct.unpack_from('<fff',body,8)==(-8900+stage,-160,82),'Cast state did not reach published view'
                            if stage==0:send(0x1e2,struct.pack('<QI',1,500))
                            if stage in (1,3):send(0x2a6,struct.pack('<QI',1,133))
                            if stage==2:send(0x131,start)
                            if stage==4:send(0x139,struct.pack('<II',689,3000))
                            if stage in (5,9):send(0x13a,bytes(4))
                            if stage==6:send(0x131,start);send(0x132,b'\1\1\1\1'+struct.pack('<IHBBH',133,0,0,0,0))
                            if stage==7:send(0x139,struct.pack('<II',689,0xffffffff))
                            if stage==8:send(0x13a,struct.pack('<I',1000))
                        send(0x4d);return
                    if mode.startswith('gcd_'):
                        assert read()==(0x26a,struct.pack('<Q',1))
                        start=b'\1\1\1\1'+struct.pack('<IHIH',133,0,3000,0)
                        interrupt=struct.pack('<QI',1,133)
                        if mode=='gcd_short_start':send(0x131,start[:-1]);return
                        if mode=='gcd_bad_bit':send(0x266,struct.pack('<BBi',64,21,-500));return
                        if mode=='gcd_bad_op':send(0x267,struct.pack('<BBi',0,29,-20));return
                        if mode=='gcd_short_interrupt':send(0x2a6,interrupt[:-1]);return
                        send(0xa9,player_update({36:0x000101,193:0x05040302,194:6,145:struct.unpack('<I',struct.pack('<f',.5))[0],128:struct.unpack('<I',struct.pack('<f',2000))[0]},True))
                        send(0x131,start)
                        for stage in range(6):
                            opcode,movement=read()
                            assert opcode==0xee and struct.unpack_from('<fff',movement,8)==(-8900+stage,-160,82),'GCD/context state did not reach action view'
                            if stage==0:
                                send(0x2a6,interrupt);send(0x267,struct.pack('<BBi',0,21,-20));send(0x131,start)
                            if stage==1:
                                send(0x132,b'\1\1\1\1'+struct.pack('<IHBBH',133,0,0,0,0));send(0x2a6,interrupt)
                            if stage==2:send(0x134,struct.pack('<QIIII',1,116,7000,133,0))
                            if stage==3:send(0x1de,struct.pack('<IQ',116,1))
                            if stage==4:send(0x2a6,struct.pack('<QI',2,133));send(0x12a,bytes(5))
                        send(0x4d);return
                    if mode.startswith('cooldown_'):
                        assert read()==(0x26a,struct.pack('<Q',1))
                        body=struct.pack('<BHHHHHII',0,0,1,78,0,0,5000,0)
                        if mode=='cooldown_initial_bad':send(0x12a,body[:-1]);return
                        if mode=='cooldown_flags':send(0x134,struct.pack('<QBII',1,0,78,10000));return
                        if mode=='cooldown_short':send(0x134,struct.pack('<QII',1,78,10000)[:-1]);return
                        send(0x12a,body)
                        for stage in range(3):
                            opcode,movement=read()
                            assert opcode==0xee and struct.unpack_from('<fff',movement,8)==(-8900+stage,-160,82),'Cooldown state did not reach the action view'
                            if stage==0:send(0x134,struct.pack('<QII',1,78,10000))
                            if stage==1:send(0x1de,struct.pack('<IQ',78,1))
                        send(0x4d);return
                    if mode.startswith('actions_'):
                        assert read()==(0x26a,struct.pack('<Q',1))
                        send(0x12a,struct.pack('<BHHHHHH',0,2,78,0,6603,0,0))
                        body=bytearray(480);struct.pack_into('<I',body,23*4,6603)
                        if mode=='actions_short':body=body[:-1]
                        if mode=='actions_trailing':body+=b'\0'
                        send(0x129,body)
                        if mode!='actions_valid':return
                        for value in (78,0,6603):assert read()==(0x128,struct.pack('<BI',23,value)), 'Wrong Vanilla action edit'
                        opcode,movement=read();assert opcode==0xee and struct.unpack_from('<fff',movement,8)==(-8900,-160,82)
                        send(0x4d);return
                    if mode.startswith('quest_'):
                        assert read()==(0x26a,struct.pack('<Q',1))
                        assert read()==(0x5c,struct.pack('<I',7))
                        body=struct.pack('<15I',7,2,2,12,0,0,0,0,0,0,25,60,0,0,0)+bytes(24*4)
                        body+=b'Fixture quest\0Defeat ten creatures.\0A fixture description.\0\0'
                        body+=struct.pack('<4I',6,10,0,0)+bytes(12*4)+b'\0\0\0\0'
                        if mode=='quest_short':body=body[:-1]
                        if mode=='quest_trailing':body+=b'\0'
                        if mode=='quest_nan':body=body[:144]+struct.pack('<f',float('nan'))+body[148:]
                        send(0x5d,body)
                        if mode!='quest_valid':return
                        opcode,movement=read();assert opcode==0xee and struct.unpack_from('<fff',movement,8)==(-8900,-160,82)
                        send(0x4d);return
                    if mode=='appearance':
                        assert read()==(0x26a,struct.pack('<Q',1))
                        send(0xa9,player_update({36:0x000101,193:0x0a090807,194:11,452:2362},True))
                        assert read()==(0x56,struct.pack('<IQ',2362,0))
                        send(0x58,item_metadata(2362,18730,14))
                        for stage in range(4):
                            opcode,movement=read()
                            assert opcode==0xee and struct.unpack_from('<fff',movement,8)==(-8900+stage,-160,82), 'Live appearance was not published'
                            if stage==0:send(0xa9,player_update({452:0}))
                            if stage==1:
                                send(0xa9,player_update({452:25}))
                                assert read()==(0x56,struct.pack('<IQ',25,0))
                                send(0x58,item_metadata(25,1542,21))
                            if stage==2:send(0xa9,player_update({190:0x400,193:0,194:0}))
                        send(0x4d);return
                    if mode.startswith('far_'):
                        assert read()==(0x26a,struct.pack('<Q',1))
                        destination=struct.pack('<Iffff',1 if mode=='far_map' else 0,-8900,-160,82,1.5)
                        if mode=='far_bad_pending':send(0x3f,b'\0');return
                        send(0x3f,struct.pack('<I',1 if mode=='far_map' else 0))
                        if mode=='far_nan':destination=destination[:4]+struct.pack('<f',float('nan'))+destination[8:]
                        if mode=='far_trailing':destination+=b'\0'
                        if mode=='far_short':destination=destination[:-1]
                        send(0x3e,destination)
                        if mode not in ('far_valid','far_map'):return
                        assert read()==(0xdc,b''),'Wrong world-transfer acknowledgement'
                        opcode,movement=read();assert opcode==0xee and struct.unpack_from('<fff',movement,8)==(-8900,-160,82)
                        send(0x4d);return
                    if mode.startswith('near_'):
                        assert read()==(0x26a,struct.pack('<Q',1))
                        body=b'\1\1'+struct.pack('<IIIffffI',37,0,1234,-8900,-160,82,1.5,0)
                        if mode.startswith('near_short_'):send(0xc7,body[:int(mode.rsplit('_',1)[1])]);return
                        if mode=='near_wrong_guid':send(0xc7,b'\1\2'+body[2:]);return
                        if mode=='near_nan':body=body[:14]+struct.pack('<f',float('nan'))+body[18:]
                        if mode=='near_trailing':body+=b'\0'
                        send(0xc7,body)
                        if mode!='near_valid':return
                        assert read()==(0xc7,struct.pack('<QII',1,37,1234)), 'Wrong Vanilla near-teleport acknowledgement'
                        opcode,movement=read();assert opcode==0xee and struct.unpack_from('<fff',movement,8)==(-8900,-160,82), 'Stale movement escaped the teleport barrier'
                        send(0x4d);return
                    if mode in ('observer_teleport','truncated_teleport','player_transfer'):
                        assert read()==(0x26a,struct.pack('<Q',1))
                        relocation=bytes.fromhex('dfe83801d10230f100010000ad62ac00cce108c6545a5ac325b5ab420c293cbd00000000')
                        if mode=='player_transfer':send(0x3e);return
                        if mode=='truncated_teleport':send(0xc5,relocation[:-1]);return
                        send(0xc5,relocation);send(0x4d);return
                    assert read()==(0x4b,b'');send(0x4d)
            except (ConnectionResetError,BrokenPipeError):pass
            except BaseException as error:errors.append(error)
        worker=threading.Thread(target=serve,daemon=True);worker.start()
        result=subprocess.run([str(ROOT/'build/host/wowx_world_fixture.exe'),str(listener.getsockname()[1])]+(['--weather'] if mode in ('weather_valid','weather_early') else ['--live'] if mode.startswith('weather_') else ['--clock'] if mode in ('clock_valid','clock_early') else ['--live'] if mode.startswith('clock_') else ['--casts'] if mode.startswith('cast_') else ['--gcd'] if mode.startswith('gcd_') else ['--cooldowns'] if mode.startswith('cooldown_') else ['--actions'] if mode.startswith('actions_') else ['--journal'] if mode.startswith('quest_') else ['--appearance'] if mode=='appearance' else ['--short-key'] if mode=='short_key' else ['--teleport'] if mode in ('near_valid','far_valid','far_map') else ['--live'] if mode.startswith(('near_','far_')) or mode in ('observer_teleport','truncated_teleport','player_transfer') else []),
                              capture_output=True,text=True,timeout=25 if mode=='cold_map_login' else 20)
        worker.join(5)
        assert not worker.is_alive() and not errors,(mode,errors)
        assert (result.returncode==0)==success,(mode,result.stdout,result.stderr)
        if mode in ('near_valid','far_valid','far_map'):assert 'position revision 2; -8900.00 -160.00 82.00' in result.stdout
        if mode.startswith(('near_','far_')) and not success:assert 'position revision 1;' in result.stdout
        print(mode+': '+result.stdout.strip())

if __name__=='__main__':
    for mode in ('valid','empty_list','short_key','cold_map_login'):case(mode,True)
    for mode in ('short_header','invalid_size','truncated_body','rejected','count','name_terminator','truncated_equipment','trailing_characters','nan_position'):case(mode)
    case('observer_teleport',True);case('truncated_teleport');case('player_transfer')
    case('near_valid',True)
    for mode in ('near_wrong_guid','near_nan','near_trailing'):case(mode)
    for length in range(34):case(f'near_short_{length}')
    case('far_valid',True);case('far_map',True)
    for mode in ('far_bad_pending','far_nan','far_trailing','far_short'):case(mode)
    case('appearance',True)
    case('quest_valid',True)
    for mode in ('quest_short','quest_trailing','quest_nan'):case(mode)
    case('actions_valid',True);case('actions_short');case('actions_trailing')
    case('cooldown_valid',True)
    for mode in ('cooldown_initial_bad','cooldown_flags','cooldown_short'):case(mode)
    case('gcd_valid',True)
    for mode in ('gcd_short_start','gcd_bad_bit','gcd_bad_op','gcd_short_interrupt'):case(mode)
    case('cast_valid',True)
    for mode in ('cast_short_channel','cast_bad_channel_time','cast_short_delay'):case(mode)
    case('clock_valid',True);case('clock_early',True)
    for mode in ('clock_bad_early','clock_short','clock_nan','clock_calendar'):case(mode)
    case('weather_valid',True);case('weather_early',True)
    for mode in ('weather_short8','weather_tbc9','weather_short12','weather_trailing','weather_type','weather_nan','weather_negative','weather_over1','weather_flag','weather_early_bad'):case(mode)
    print('99 encrypted world protocol scenarios passed.')
