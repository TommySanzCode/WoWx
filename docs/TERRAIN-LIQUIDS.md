# Terrain liquid development — 2026-09-17

This batch extends the shared converter to original Vanilla MCLQ grids. It does
not complete swimming, all maps, full water appearance or hardware acceptance.
Native acceptance and final measurements are recorded below when available.

## Source interpretation

WoWee remains the pinned terrain, placement and texture parser foundation. Its
tolerant MCLQ path already produces 42 and 73 water layers in these two files.
The native pipeline previously did not consume those layers. The new strict
adapter follows the vMaNGOS extractor layout, reads the original masks and checks
all 256 chunk identities against WoWee. Original offsets include the eight-byte
MCNK header. The two supplied Azeroth ADTs have an inner MCLQ length of zero and an
authoritative MCNK liquid length of 812 bytes: header, 720-byte grid and 84-byte
flow tail. Ambiguous family flags, truncated/duplicate chunks, non-finite heights
and unexpected lengths fail preparation.

Each visible original cell retains its two triangles, height values and mask.
The low nibble 0x0f marks a dry cell. Bit 0x80 marks deep water and remains
visible. Lake, ocean, magma and slime families map to the shared 30-frame
texture pipeline. No per-zone mesh implementation or flat-water fallback is used.

Initial conversion rejected original FLT_MAX height sentinels. Independent raw
inspection found 792 in Azeroth_32_48 and 1,051 in Azeroth_32_49; none belongs to
an enabled cell. Only these unreferenced slots are encoded as finite zero. A
sentinel referenced by any visible triangle still fails. Terrain normals never
sample an undrawn corner. Raw ADTs, failed logs and the precise sentinel report
remain under build; this canonicalization changes no drawn vertex or depth query.

Terrain grid columns travel toward negative world Y and rows toward negative
world X, matching the existing terrain mesh. The shared mesh builder corrects
triangle winding for that axis exchange. All ordinary WMO meshes continue to
use their previous normal policy. Surface patches remain at most 16x16 cells;
an MCLQ grid is 8x8. Surfaces remain non-collidable and use the existing bounded
resident texture sequence and post-actor liquid rendering pass.

## Explicit limitations

The camera metadata describes a vertical liquid column; it does not yet exclude
caves or terrain below the water. Ground/cave containment and server movement
must be integrated before claiming swimming. Original per-vertex depth opacity,
authored flow UV/tails, shore transitions, waves and reflection/refraction remain
unimplemented. Ocean/slime have host family coverage, not native acceptance in
the two lake/river tiles prepared here. The normal installed world index and the
stable hardware kit remain unchanged.

## Host validation

797 host environment checks pass, including original offset interpretation,
finite/malformed data, 0x80 deep water, hidden cells, exact sea-level zero,
source-family mapping, reflected triangle winding and runtime depth sampling.
The existing 26,359 liquid mesh/clock/streaming/allocation-failure checks pass.
Initial strict-sentinel failures are retained in the first conversion folder;
the corrected candidate uses a fresh folder. A separate byte-level comparator
checks every visible original terrain cell before native testing.

Two original tiles prepared successfully, 340,641,826 bytes total: 115 MCLQ
grids, 3,835 enabled cells and 7,670 triangles. The independent comparison
matches every enabled triangle, visible height and mask against the original
ADTs. Tile 32/48 also retains all 19,164 ordinary earlier entries and 369 WMO
environment records exactly. Source comparison and coverage reports are under
`build/evidence/terrain-liquids-*` and `build/terrain-liquid-packs-20260917b`.

## Native checkpoint

One 64 MiB, native-scale xemu capture covers ten camera cases: four original
water locations above/below their surfaces and both sides of the prepared
Northshire tile boundary. These are camera relocations, not walked routes.
6,816 main samples and 6,815 of each companion span frames 210..7025, with
three complete cycles and no fixture/asset/region errors. All 30 liquid texture
frames were submitted, up to 13 liquid patches in a frame. Both terrain sources
remain resident at the boundary; metadata reaches 849 records / 197,332 bytes.

Minimum measured free memory is **30,736 KiB (30.02 MiB)**. Guest frame
p50/p95/p99/max is **33/34/34/69 ms**. Index work reaches 22 ms, payload work
22 ms and environment sampling 1 ms. Maximum scene residency is 9,386,560 bytes,
current index 2,549,824 bytes and pending index 1,318,528 bytes. Shared streaming
maxima are 64 KiB read, nine reads, 64 KiB scanned and three allocation batches.

The 69 ms interval follows boundary frame 1779: 32 ms combined index/payload
work plus 33 ms drawing, 68 ms total work. Later laps repeat smaller 49/43 ms
boundary intervals. This identifies a scheduling cost to address next; it does
not demonstrate sustained full-game 30 fps or physical-console performance.

The initial global-world checker rejected the legitimate two-source count
(`region_loaded=2`) and expected an indoor case absent from this explicitly
outdoor/underwater fixture. A separately preserved corrected checker accepts
positive source counts and the environments requested by the manifest. The
same capture passes that reanalysis; no additional emulator run was needed.
The original failed report and both checker identities are retained.

Actual screenshots show the original lake surface and a below-water view.
The river probe faces a bank and does not provide a clear water-surface view;
it is retained rather than treated as visual acceptance. Source measurements
place water above terrain at all four probe positions, and cooked terrain
heights there match original MCVT exactly. Full shoreline/flow/opacity parity
remains unverified. The fixture contains only two tiles; missing neighboring
content is visible beyond their coverage. Fog does not fill those missing maps.

Above-surface screenshots were captured after the timing run with the identical
source/disc and a paused guest; their frame identities are separate from the
timing capture. An initial pause landed at a transition while the displayed
frame still belonged to the previous case; the subsequent pause waits 30
frames into the requested case. No controller input was injected or physically
tested. Only owned xemu PID 52064 was quit after capture and ownership checks.

The frozen 332 source files, 14 fixture outputs and 132 normal outputs match.
Normal installed terrain assets and the stable hardware kit are unchanged.
The source/disc checkpoint is preserved independently from later analysis-tool
corrections. Shared account, characters, credentials and server remain untouched.
