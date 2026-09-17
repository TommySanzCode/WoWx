"""Resolve local tool and game paths without storing machine details in Git."""
import json
import os
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
LOCAL_CONFIG = ROOT / 'config/local.paths.json'


def configured_path(name, default=None):
    value = os.environ.get(name)
    if not value and LOCAL_CONFIG.exists():
        value = json.loads(LOCAL_CONFIG.read_text(encoding='utf-8-sig')).get(name)
    return Path(value).expanduser() if value else default


def game_data():
    return configured_path('WOWX_DATA_DIR', ROOT / 'game/Data')


def xiso_tool(override=None):
    selected = override or configured_path('WOWX_XISO')
    if selected:
        if Path(selected).is_file():
            return Path(selected)
        raise SystemExit('The configured extract-xiso executable does not exist.')
    nxdk = configured_path('NXDK_DIR')
    if nxdk:
        for name in ('extract-xiso.exe', 'extract-xiso'):
            candidate = nxdk / 'tools/extract-xiso/build' / name
            if candidate.is_file():
                return candidate
    found = shutil.which('extract-xiso')
    if found:
        return Path(found)
    raise SystemExit('Pass --xiso or configure WOWX_XISO / NXDK_DIR in the environment or config/local.paths.json.')
