"""Read-only Windows process lifetime observation for xemu test captures."""
import ctypes
from ctypes import wintypes
from datetime import datetime, timezone
import os
from pathlib import Path


class ProcessWatch:
    def __init__(self, pid_file, expected_name='xemu.exe'):
        if os.name != 'nt':
            raise RuntimeError('Process lifetime capture currently requires Windows')
        self.pid_file = Path(pid_file)
        self.expected_name = expected_name.casefold()
        self.pid = None
        self.handle = None
        self.exit = None
        self.kernel = ctypes.WinDLL('kernel32', use_last_error=True)
        self.kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
        self.kernel.OpenProcess.restype = wintypes.HANDLE
        self.kernel.QueryFullProcessImageNameW.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.LPWSTR, ctypes.POINTER(wintypes.DWORD)]
        self.kernel.QueryFullProcessImageNameW.restype = wintypes.BOOL
        self.kernel.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
        self.kernel.WaitForSingleObject.restype = wintypes.DWORD
        self.kernel.GetExitCodeProcess.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
        self.kernel.GetExitCodeProcess.restype = wintypes.BOOL
        self.kernel.CloseHandle.argtypes = [wintypes.HANDLE]
        self.kernel.CloseHandle.restype = wintypes.BOOL

    def _bind(self):
        try:
            raw = self.pid_file.read_bytes()
            if len(raw) > 32:
                return
            pid = int(raw.decode('utf-8-sig').strip())
        except (OSError, UnicodeError, ValueError):
            return
        if not 0 < pid <= 0x7fffffff:
            return
        handle = self.kernel.OpenProcess(0x00100000 | 0x1000, False, pid)
        if not handle:
            return  # A stale PID file before the next launch is expected.
        name = ctypes.create_unicode_buffer(32768)
        length = wintypes.DWORD(len(name))
        if not self.kernel.QueryFullProcessImageNameW(handle, 0, name, ctypes.byref(length)) or Path(name.value).name.casefold() != self.expected_name:
            self.kernel.CloseHandle(handle)
            return
        self.handle, self.pid = handle, pid

    def poll(self):
        if self.exit is not None:
            return self.exit
        if not self.handle:
            self._bind()
        if not self.handle:
            return None
        state = self.kernel.WaitForSingleObject(self.handle, 0)
        if state == 258:  # WAIT_TIMEOUT: process remains alive.
            return None
        if state != 0:
            raise ctypes.WinError(ctypes.get_last_error())
        code = wintypes.DWORD()
        if not self.kernel.GetExitCodeProcess(self.handle, ctypes.byref(code)):
            raise ctypes.WinError(ctypes.get_last_error())
        self.exit = {'pid': self.pid, 'exit_code': code.value,
                     'observed_utc': datetime.now(timezone.utc).isoformat()}
        self.close()
        return self.exit

    def close(self):
        if getattr(self, 'handle', None):
            self.kernel.CloseHandle(self.handle)
            self.handle = None

    def __del__(self):
        self.close()
