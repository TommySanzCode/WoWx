#!/usr/bin/env python3
"""Start only this workspace's existing private test server, without duplicates.

Run inside WSL Ubuntu. Configuration, credentials and earned game state are not
rewritten. Logs append so a restart does not discard earlier failure evidence.
"""
import fcntl
import os
import socket
import subprocess
import time
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SERVER=ROOT/'server'

def listening(port):
    try:
        with socket.create_connection(('127.0.0.1',port),timeout=.3):return True
    except OSError:return False

def owners(binary):
    result=[]
    for folder in Path('/proc').iterdir():
        if not folder.name.isdigit():continue
        try:
            if (folder/'exe').resolve(strict=True)==binary:
                arguments=(folder/'cmdline').read_bytes().split(b'\0')
                if b'-c' not in arguments:raise RuntimeError(f'{binary.name} has an unexpected configuration')
                index=arguments.index(b'-c')
                config=Path(os.fsdecode(arguments[index+1]))
                if not config.is_absolute():config=(folder/'cwd').resolve(strict=True)/config
                if config.resolve()!=SERVER/'etc'/f'{binary.name}.conf':
                    raise RuntimeError(f'{binary.name} belongs to another configuration')
                result.append(int(folder.name))
        except (FileNotFoundError,PermissionError,ProcessLookupError):continue
    return result

def start(name,port):
    binary=(SERVER/'bin'/name).resolve(strict=True)
    config=(SERVER/'etc'/f'{name}.conf').resolve(strict=True)
    running=owners(binary)
    if len(running)>1:raise RuntimeError(f'Multiple {name} processes exist; refusing another instance')
    process=None
    if running:
        pid=running[0]
        print(f'{name}: existing workspace process {pid}',flush=True)
    else:
        if listening(port):raise RuntimeError(f'Port {port} belongs to another process')
        with (SERVER/'logs'/f'{name}-console.log').open('ab',buffering=0) as log:
            process=subprocess.Popen([str(binary),'-c',str(config)],cwd=SERVER,
                                     stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,
                                     start_new_session=True,close_fds=True)
        pid=process.pid
        print(f'{name}: started workspace process {pid}',flush=True)
    (SERVER/'logs'/f'{name}.pid').write_text(str(pid)+'\n')
    deadline=time.monotonic()+60
    while time.monotonic()<deadline:
        if process is not None and process.poll() is not None:
            raise RuntimeError(f'{name} exited with {process.returncode}; inspect its console log')
        if listening(port):
            print(f'{name}: listening on loopback port {port}',flush=True)
            return
        time.sleep(.5)
    raise RuntimeError(f'{name} did not open port {port}; inspect its console log')

def main():
    (SERVER/'logs').mkdir(exist_ok=True)
    with (SERVER/'logs'/'startup.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        if not listening(3306):
            if os.geteuid()!=0:raise RuntimeError('Local MariaDB is stopped; run the launcher as WSL root')
            subprocess.run(['service','mariadb','start'],check=True)
        start('realmd',3725)
        start('mangosd',8086)

if __name__=='__main__':main()
