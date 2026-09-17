"""Control only the dedicated local WOWX xemu QMP endpoint."""
import argparse,json,socket

def command(name,args=None):
    with socket.create_connection(('127.0.0.1',4444),timeout=10) as s:
        f=s.makefile('rwb',buffering=0)
        greeting=json.loads(f.readline())
        if 'QMP' not in greeting: raise RuntimeError('Not a QMP endpoint')
        def call(execute,arguments=None):
            f.write((json.dumps({'execute':execute,'arguments':arguments or {}})+'\n').encode())
            while True:
                r=json.loads(f.readline())
                if 'error' in r: raise RuntimeError(r['error'])
                if 'return' in r:return r['return']
        call('qmp_capabilities')
        return call(name,args)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('command');p.add_argument('argument',nargs='?');a=p.parse_args()
    print(json.dumps(command(a.command,json.loads(a.argument) if a.argument else {}),indent=2))
