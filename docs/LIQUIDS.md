# Original WMO liquid surfaces — 2026-09-17

## Implemented scope

WXP v10 adds original WMO MLIQ surface meshes and original animated texture
sequences. The general WMO path handles global maps and terrain placements.
No individual zone implementation or hardcoded water rectangle is used.

The converter reuses the strict original-grid adapter from ENVIRONMENTS.md.
It emits two triangles per enabled cell, preserves non-planar heights and the
original low-nibble hole mask, and uses the same diagonal as the runtime depth
query. Patches span at most 16 by 16 cells (289 vertices / 512 triangles).
Normals and UVs remain continuous across patch boundaries. Original placement
transforms and placement IDs are retained. Surface parts use the static render
kind, so they do not become solid walkable floors or camera collision.

Liquid IDs 1/2/3/4/21 select original lake/ocean/lava/slime texture paths. Unknown
nonempty types fail preparation explicitly. Thirty BLP frames are decoded from
private archives, reduced to 64 by 64 and mipmapped. No game imagery is checked
into the source tree. The initial water/ocean/slime opacity is .48/.72/.65,
based on the pinned WoWee water policy. Magma is opaque, unlit and unfogged.
Original texture RGB is retained, without a fabricated blue tint.

The frame/path interpretation was cross-checked against the independent
[Vanilla client terrain research](https://github.com/samwhosung/wow-1121-client-internals/blob/main/docs/terrain.md).
This supports the 30-frame / 1250-ms period and texture families. It does not
establish pixel-equivalent rendering. Host timing checks cover nearest-even
rounding of framePhase minus .5, including exact ties and wrap to frame zero.

## Memory, streaming and rendering

V10 uses entry reserved[1] high 16 bits for sequence length and low 16 bits for
mip levels. Legacy entries retain their old interpretation. Each complete
frame/mip chain is padded to 128 bytes for aligned NV2A texture addresses.
The bounded loader reads and validates the whole sequence before publishing a
resident part. All parts sharing its source/offset/format share one allocation.
Playback changes the GPU texture address; it does not allocate or upload a
new frame every tick. All sequence bytes are included in pending/resident
budgets and the existing 8 MiB allocation headroom checks.

One 30-frame 64-square sequence occupies 656,640 bytes as RGBA. The host repacker
also supports every frame and preserves frame alignment: 84,480 bytes for DXT1
or 165,120 bytes for DXT5. Compression chooses a single format across all frames
and excludes padding from alpha classification. Native sequence acceptance uses
RGBA; compressed sequence playback still requires its own native acceptance.

A separate world liquid pass runs after actors/player and before interface and
portraits. It restores ordinary material, fog, motion and vertex-colour state.
This permits opaque actors below transparent water to be covered by the water
blend. Exact sorting of intersecting translucent geometry remains unresolved;
no live actor-under-water acceptance is claimed. Other offline world fixtures
use this same pass. Normal underwater lighting telemetry now reports the water
condition used by the actual lighting sampler.

## Prepared original content

Twenty global packs prepared successfully. The known map 450 original NaN UV
still fails explicitly. The new packs total 436,983,970 bytes. All 57,991 previous
entries retain exact IDs, geometry, vertex lighting, material motion and texture
bytes; original environment blobs remain exact. Added surface data comprises
386 patches, 47,327 enabled cells and 94,654 triangles. Ten per-pack shared
sequences and geometry/material data add 9,526,892 bytes in total. Actual global
liquid types present are water and magma; ocean and slime are not native-tested
by this batch. See build/evidence/liquid-payload-comparison.json.

Host checks pass: 26,359 liquid mesh/clock/layout/publication/cancellation/failure
checks; 223 texture codec checks; 464 environment checks; 453 runtime and 5,144
staged streaming checks; 123 pack-repacking checks; two material telemetry tests.
The initial repacking test incorrectly compared an unused DXT colour endpoint
to the source pixel. Preserved failing logs document that test error. The final
independent decoder follows the block's selector and checks reconstructed colour.

## Native acceptance

One combined capture passes the scoped data/render/streaming checks:
8,673 main samples, 8,672 of every companion, frames 240..8912, two completed
cycles, zero reported errors and all 30 frames submitted on maps 48/230/429.
Minimum free 30,296 KiB (29.59 MiB); guest frame p50/p95/p99/max 33/34/34/55 ms.
The isolated 55 ms interval follows a48 ms first Stockades outdoor draw; cause
unproven. No controlled performance comparison or stock-console claim is made.

The fixture in build/liquid-test-20260917 contains a frozen 331-file source
snapshot and 16 mounted outputs, with no credentials or input replay. Native
RAM query reports 67,108,864 bytes, native surface scale 1 and disposable disk
writes. Captured camera cases cover Stockades, Blackfathom, Blackrock Depths and
Dire Maul, above/below original water and magma. These are synthetic cameras,
not movement, swimming or authoritative gameplay acceptance.

Actual original water and magma screenshots were captured after the timing run,
with the same source/disc and a paused guest; their separate frame identities
are retained. The Blackfathom underwater loading image is within the timing run.
Correction after inspecting the saved images again: the Blackfathom and Blackrock
labels read completely. All 35 expected non-space glyph boxes contain gold
strokes in both original captures and a separate triangle-submission experiment.
The earlier missing-letter diagnosis is not supported by these images. The
experiment is preserved in `20260917-ui-triangles-experiment` and its six source
changes were reverted; no GPU bug or fix is claimed. This narrow check does not
establish full UI visual acceptance. CPU UI failures remain zero.
Cutoff/geometry and exact liquid appearance also remain incomplete.

After capture, 331 source hashes and 16 fixture outputs, plus 132 normal outputs,
match. Only owned xemu PID 66088 was quit. The post-QMP GLib shutdown assertion is
preserved. The stable hardware kit, shared account and server were untouched.

## Remaining parity and release work

Terrain MCLQ surfaces, exact authored flow UV/depth opacity, shore transitions,
waves, reflections/refraction, swimming, all spell/movement interactions and
full water appearance remain unfinished. Room containment still uses group
bounds rather than portal/room topology. Broad map entry, full original UI,
all-class gameplay, hardware controllers and stock-console timing remain open.
Native acceptance must report actual frame distributions and headroom before
this candidate can replace any scoped development checkpoint. The stable
hardware kit and reserved shared account remain protected.
