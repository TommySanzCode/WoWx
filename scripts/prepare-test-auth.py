"""Package only the disposable local test account, never a personal account."""
import json
import struct
from pathlib import Path
root=Path(__file__).resolve().parents[1]
credentials=json.loads((root/'server/local-credentials.json').read_text())
data=struct.pack('<64s32s32sI',b'10.0.2.2',credentials['username'].encode(),credentials['password'].encode(),3725)
(root/'build/xbox/testauth.bin').write_bytes(data)
print('Packaged disposable local test credentials (ignored by source control).')
