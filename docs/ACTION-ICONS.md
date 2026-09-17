# Bounded action icons

## Current behavior

Holding LT, RT or both now draws eight original ability/item icons alongside the
existing action names and controller labels. The selected layer and warrior stance
continue to use the existing authoritative action-slot mapping. Pressed controls
highlight their frame. Empty slots remain empty. Item icons resolve from the
server-query item display; spell icons resolve from their Vanilla spell ID.
Assignments/casting are the existing client operations, not a separate demo path.

This is an icon/presentation implementation, not full action-bar parity. The
complete original bottom bar/menu decoration, cooldown sweeps, mana/range/usability,
charges/counts, tooltips, macros, pet bars and all class/form gameplay remain open.
The controller layer layout remains an adaptation; it is not an exact PC layout.
The current spellbook still uses its previous text presentation.

## Assets and memory

The converter uses pinned WoWee Vanilla DBC layout fields: Spell.IconID 117,
SpellIcon.Path 1, ItemDisplayInfo.InventoryIcon 5. Paths are case-normalized and
legacy .tga references resolve to .blp. All spell and item-display mappings are
indexed, independently of which zones are prepared. RGB pixels are box-filtered
with alpha weighting to 32x32 and stored swizzled for NV2A. Paths share image data.

The private ICONS.WIC contains 51,962 mappings and 2,522 images, 10,745,832 bytes.
47 referenced source paths are absent from the supplied archive set; their keys
map to the supplied question-mark icon. They are listed in the accompanying
.missing.txt and remain coverage gaps; do not silently relabel them as verified
icons. Blank/unknown references use the same fallback. Empty action bindings do
not request/draw a fallback icon.

WXIC v1: six uint32 words (magic, version, index count, image count, pixel offset,
file size), sorted key/image pairs, then 4 KiB 32x32 BGRA/Morton images. Spell keys
are their IDs; item-display keys are 0x80000000 | display ID; key/image 0 is the
original question mark. Bounds are 131,072 mappings and 16,384 images, below 1 GiB.
The runtime checks counts, exact offsets/length, key ordering/types and image IDs.

Runtime storage is 677,840 bytes: a 415,696-byte CPU index and 262,144-byte 256x256
GPU atlas with 64 cells. At most 32 requested keys are resolved per update; the
current gameplay integration warms all 24 bindings. All requested resident cells
are protected from eviction. One missing image is read/uploaded per frame (4 KiB),
after the preceding GPU frame drains. There is no steady-frame allocation. The
existing 8 MiB headroom guard covers both index and atlas allocation.

The common UI renderer now preserves draw order across at most 128 texture batches
inside its unchanged 2,048-quad allocation. Original fonts/artwork and icon quads
can alternate without opaque panels covering icons or icons covering later text.
Only small fixed CPU batch descriptors are added to WxUi. UI graphics storage
remains 1,376,256 bytes. This changes shared UI submission and still needs the
pending combined front-end/live-world regression gate.

## Evidence and native fixture

Host checks exercise malformed index/header/lengths, fallback resolution, pinned
cache eviction, one-upload-per-update, no uploads once warm, allocation failure,
clock rollover, mapped items and the real asset pack. UI tests cover ordered
textures, UV clipping, invalid arguments and the 128-batch bound. The existing HUD
command checks and telemetry version tests also pass.

WXTX telemetry is 1,368 bytes. It appends icon ready/index/images/bytes/load/pending/
failure/draw counts, UI batch count and upload submission milliseconds to WXTW.
Historical packets remain readable with zero-filled new fields.

The isolated PTTEST.BIN = WXPF0002 fixture uses real catalogs/rendering with
synthetic bindings and injected UI state. It cycles all three layers and eight
pressed-control labels, including empty slots and a Hearthstone. It does not start
the auth worker or contain credentials. It proves no additional class combat,
server synchronization, physical input or hardware performance. Its actor/player
portraits are also drawn through the new ordered UI path. Use a new output folder:

```powershell
python scripts/prepare-portrait-test.py --output build/icons-test-NEW --action-icons
scripts/run-portrait-test.ps1 -Fixture build/icons-test-NEW -Capture build/evidence/native-icons-NEW.csv -Seconds 110
python tools/check-icons.py build/evidence/native-icons-NEW.csv
```

Preserve the actual mounted receipt/source ZIP and screenshots. Keep the stable
physical-test kit unchanged. Live-world action/stance/equipment binding updates,
full-world headroom, PC presentation fidelity and hardware remain separate gates.

## Recorded native result

`native-action-icons` passes the isolated 110-second capture: 1,567 received
samples, all three layers with correct slots and 6/8/8 icons, no failures, minimum
43,592 KiB free, frame p50/p95/p99/max 33/34/34/35 ms. Upload p95/p99/max 0/1/1 ms.
All received samples were already warm; the host budget test, not this capture,
proves cold-cache pacing. `native-action-icons-right.png` is an actual screenshot.
The fixture XBE matches the normal candidate; full-world and hardware acceptance
remain pending. Source/disc identities are preserved in `build/icons-test-20260916`.
