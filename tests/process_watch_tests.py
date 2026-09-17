"""Observe real child process exits, stale PID files and process identity checks."""
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from process_watch import ProcessWatch

checks = 0
def check(value):
    global checks
    checks += 1
    assert value

with tempfile.TemporaryDirectory(prefix='wowx-process-watch-') as directory:
    pid_file = Path(directory) / 'guest.pid'
    watch = ProcessWatch(pid_file, Path(sys.executable).name)
    check(watch.poll() is None and watch.pid is None)
    for value in ('nonsense', '-5', '9999999999999999999999999999999999999999999999', '2147483647'):
        pid_file.write_text(value)
        check(watch.poll() is None and watch.pid is None)
    watch.close()
    for code in (0, 7):
        with subprocess.Popen([sys.executable, '-c', f'import time; time.sleep(1); raise SystemExit({code})']) as child:
            pid_file.write_text(str(child.pid))
            wrong = ProcessWatch(pid_file, 'not-the-child.exe')
            check(wrong.poll() is None and wrong.pid is None)
            wrong.close()
            watch = ProcessWatch(pid_file, Path(sys.executable).name)
            check(watch.poll() is None and watch.pid == child.pid)
            child.wait(timeout=10)
            result = watch.poll()
            check(result['pid'] == child.pid and result['exit_code'] == code)
            check(watch.poll() == result)
            check(watch.handle is None)
            watch.close()
print(f'Process lifetime/identity checks: {checks} passed')
