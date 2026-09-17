# WMO environment classification — 2026-09-17

This batch adds source-derived interior and liquid classification to the shared
world renderer. It does not implement water surfaces, swimming, portal visibility,
local WMO fog volumes, all-map entry, or complete Vanilla visual parity.

## Implementation

The converter preserves original WoWee WMO group bounds/flags and inverse rigid
placement transforms. It also reads the original MLIQ grid: 30-byte header,
eight-byte vertex records, heights and per-tile hole flags. Strict chunk lengths,
finite heights, dimensions and cross-checks against WoWee reject incomplete data;
there is no invented flat-water fallback or map-specific waterline offset.

Liquid IDs follow the original-file interpretation checked against the pinned
vMaNGOS vmap extractor. Height sampling uses its diagonal two-triangle convention;
exact original-client tessellation is still unverified. Water/ocean select the
underwater palette. Magma, slime and unknown IDs remain explicit diagnostic data
without being mislabelled as blue water. WMO liquid material IDs are not blindly
used as liquid-type IDs.

WXP v9 world packs append a WXE1 environment blob/footer. Existing geometry,
indices, pixels, vertex colours and material motion retain their exact byte
layout. The 32-byte pack header, 64-byte draw entries and 32-byte vertices retain
their ABI. Records are 128 bytes, with blob-relative height/mask offsets. Each
pack permits at most 2,048 records and 2 MiB metadata, with grids at most 256 tiles
per axis. Earlier packs remain readable and report unknown environment coverage.
Character/atlas packs remain v8; v9 is currently world-only.

Each of four resident pack sources owns its metadata. Loading uses the existing
world-lane byte/read/scan/allocation quotas. Each metadata read is at most 16 KiB;
validation and large height arrays continue across frames. Unpublished data never
classifies a camera. Cancellation, detach, truncation and memory failures release
all reserved bytes. Metadata is independent of the 256 rendered-model slots,
counts in scene residency, and respects the same 8 MiB free-memory guard.

The normal client samples at its collision-adjusted camera. Indoor group boxes
suppress outdoor sky/fog; water/ocean uses authored water lighting where present
and a documented blue 5–60-unit fog fallback otherwise. UI and preview render
state remains separately restored. Overlapping interior boxes choose the smallest
containing volume. These boxes are a coarse approximation: exact portal/room
containment, camera transition smoothing and overlapping floors/liquid volumes
remain further work. Terrain MCLQ, submerged-only root flags and local WMO fog
records are not yet implemented.

## Original-asset preparation

Twenty packs in `build/environment-packs-v2-20260917` contain 721 group volumes
and 69 liquid grids (790 records). Metadata totals 562,268 bytes; packs total
427,457,078 bytes. The largest metadata blob is 134,828 bytes, largest record set
123, largest original grid axis 139 tiles. All 57,991 entries retain byte-exact
pre-footer v8 index/payload data. Map 450 remains rejected for its original NaN UV.

The initial adapter capped grids at WoWee's renderer's 64-tile limit. Original
Blackfathom has a 34-by-68 grid; four maps failed that cap. Those first outputs and
logs remain in `build/environment-packs-20260917`. Raising the bounded metadata
limit to 256 permits all 20 previously accepted maps within the measured small
metadata allocation. No geometry or liquid tiles were dropped. The ordinary tile
converter also carries WMO environment records; new terrain packs still require
native route acceptance before replacing installed assets.

## Targeted host validation

464 environment checks pass: rotated/translated bounds, overlap priority, original
liquid-type interpretation, both nonplanar height triangles, tile holes, exact
waterline exclusion, malformed footers/offsets/matrices/heights, incomplete-source
reads, memory-budget failure, staged publication, four-source detach/reload and
legacy unknown coverage. Runtime 453, avatar 1,467, staged streaming 5,144,
fog 1,715 and lighting/clock 20,831 checks pass. Repacking passes 55 checks,
including exact metadata preservation and malformed-footer rejection. Sixteen
telemetry tests pass, including strict WXY1 environment packets.

The first test build lacked the GLM include path; the first liquid-type test
incorrectly expected group type 15 to ignore its tile fallback. Both were fixed.
The old fog test assumed underwater fallback was disabled and was updated to test
the new documented behavior. Original failed logs are retained. Host and native
builds pass; native warnings include zero-initialized aggregate tails and existing
indentation/initializer warnings. No runtime sources changed after capture freeze.

## Native acceptance

Accepted scoped candidate: `build/candidates/20260917-environment`.
The single combined capture is in `build/environment-test-20260917`.
Twelve offline camera cases cover original Stockades, Wailing Caverns, Blackfathom
and Deeprun Tram bounds, including above/below original water grids. Cases come
from the prepared original data and an independent host decoder/sampler. They are
synthetic camera positions, sometimes outside walkable rooms, and are not player
navigation, swimming, controller input, live-server or physical-hardware tests.

All source and mounted-disc identities remain frozen during the capture. The
stable hardware kit, prior candidates, normal terrain/actor index, saved accounts,
credentials and shared server remain protected. Full port and stock-console
release gates remain open.

One combined offline xemu capture passed: 9,179 main samples, frames 210–9388,
and 9,178 each stream/environment/lighting companions with complete nonterminal
identity coverage. All twelve cases completed; the cycle counter reached four.
No asset, region, classification, fixture or shared-quota errors occurred.

Confirmed 67,108,864 bytes guest RAM, native rendering scale 1 and disposable HDD
writes. Minimum free memory: 31,648 KiB (30.91 MiB). Guest frame p50/p95/p99/max:
33/34/34/193 ms. The sole interval over 40 ms followed frame 278, the first outdoor
fog draw in Stockades: draw 186 ms, stream 3 ms, work 192 ms. Cause is unproven;
cold GPU pipeline creation is only a hypothesis. All cold samples remain in the
report. The 30 fps/stall release gate is not met; these are different camera
workloads from prior material captures, not a matched performance comparison.

Environment queries measured 0/0/1/1 ms at p50/p95/p99/max. Maximum metadata
residency in this native fixture was 55,880 bytes. Region-index work measured
0/0/4/11 ms; payload work 0/11/17/20 ms. Peak scene residency was 10,154,346 bytes;
current/pending index capacity each reached 373,504 bytes. Shared maximum use was
65,536 read bytes, eight reads, 65,536 validation bytes and three allocation
batches. These are xemu guest times, not physical Xbox performance.

Actual screenshots show Stockades interior classification, an outside-box Wailing
Caverns diagnostic view, and blue fog below Blackfathom's original water grid.
The above-water Blackfathom image is post-capture with the identical source/disc.
It shows fog disabled and also clearly exposes missing liquid surfaces/nearby
geometry from this synthetic camera position. No claim of normal player-view
fidelity or valid walkable camera placement is made. UI remains readable through
the environment changes; no controller input was injected or physically tested.

Capture ended at duration_limit. Only owned xemu PID 12644 was quit after exact
executable, mounted-disc and QMP-owner checks. No emulator remains. Its post-QMP
GLib shutdown assertion is retained. 328 source hashes and 15 mounted outputs,
and 328 sources with 132 normal outputs, match the frozen receipts. The source
ZIP and all private asset/camera-case dependencies also match.

Normal XBE: 1,990,656 bytes, SHA256 `8fb98c37d631eb9c4bb7262a6b68b82eb31b714b71d29c1e198ae4c6fc440e4e`.
Normal XISO: 499,515,392 bytes, SHA256 `78c787b2004ed835656ab041e9598681bcad03fde483897450f35fdd7589f0d0`.


After native capture ended, the general tile converter also prepared map 0 tile
32/48 in `build/environment-terrain-20260917`: 169,821,468 bytes, 346 WMO group
volumes and 23 liquid grids (369 records, 69,252 metadata bytes). Full runtime
asset verification passed on the host. This candidate terrain pack has no native
route acceptance and does not replace the normal installed terrain index.
