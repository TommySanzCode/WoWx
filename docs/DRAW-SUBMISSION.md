# Geometry submission and combined streaming integration

## Implementation

The NV2A world renderer now submits up to 1,536 indices per triangle packet,
compared with 240 previously. Boundaries are multiples of six, preserving both
triangles and packed pairs. An odd final triangle uses ARRAY_ELEMENT32 for its
last vertex. Existing little-endian index storage is copied into the command
buffer; no extra mesh copy, asset conversion, render target or heap allocation
is introduced. The largest method contains 768 data words, below the 11-bit
method count. pbkit still receives every method header, including in debug builds.

The existing 1 MiB pushbuffer and conservative 512 KiB wrap threshold remain.
The base pointer refreshes at a wrap and at the normal frame reset. The build
explicitly tracks the new renderer headers and the previous selection header.

## Timing scope

WXD1 is a 72-byte independent companion; existing telemetry formats are unchanged.
It reports world, NPC, player and liquid submission intervals, nested explicit
wrap wait/reset costs, final drain, index counts and old/new packet counts.
There are four pass-boundary clock reads, with additional reads only on wraps.
There is no clock call per mesh, vertex or index. The fixed counters use52 bytes.

These are guest wall times. Submission includes pb_end cache flush/MMIO and
emulator scheduling; subtracting explicit waits does not produce a pure CPU
measurement. The drain observes completion from the guest and is not a hardware
GPU timestamp. Offline fixtures drain immediately; normal gameplay drains after
CPU UI construction, allowing overlap. That difference is encoded as scope 1/2.
The checker below accepts only scope 1 captures.

Host tests independently decode all emitted method streams for triangle counts
3 through 10,002, including full-range 16-bit indices, tails, bounds and invalid
inputs. The three focused C tests (batch encoding, selection and streaming) pass;
three draw and ten stream telemetry test groups pass. Existing build warnings
remain. A lower submission count alone does not establish higher FPS.

## Native run

Fixture: build/draw-batch-test-20260917. The source archive freezes 345 files;
63 fixture and 132 normal outputs verify. All 61 non-executable disc files match
build/staged-appearance-test-20260917 exactly. This uses the four original terrain
packs, not the separate two v10 terrain-water packs from the selection capture.
The combined case runs eight synthetic NPCs, an equipped Human, rapid animation
and same-profile appearance changes, a terrain boundary, Abbey and camp routes.
Phase 4 deliberately relocates between independent collision routes. It neither
logs into an account nor sends injected or physical controller input.

Initial PID 49036 failed during host graphics initialization, before telemetry,
with 0xc000041d (Windows also recorded c0000005 at offset 509315). Logs, lifecycle
and Windows events remain under native-draw-batch and fixture/startup-failure.
The unchanged-disc retry is native-draw-batch-retry, owned PID 47180. QMP verifies
67,108,864 bytes, native surface scale and a disposable disk overlay.

## Visual limits

Actual xemu window captures preserve the boundary, Abbey, relocation and standing
pose. They confirm textured geometry, NPCs, player equipment and fog were drawn.
They do not certify exact PC appearance. The deliberate relocation exposes
partial world residency and close-camera clipping before ready. Low animation
poses intersect terrain in the boundary/Abbey captures; a later full-body standing
pose is intact. Ground contact and non-looping death animation remain to be
reviewed against original animation data; animation.c currently wraps all clips
against a global clock. The previous numeric four-clip checks are not visual
acceptance of those behaviors.

The three delivered hardware kits, shared account, five saved characters, private
credentials, LAN realm and console files remain protected and unchanged.

## Accepted numeric checkpoint — September 17

Candidate: build/candidates/20260917-draw-submission. The capture contains 8,267
main rows and 8,266 exact stream/actor/draw companions, frames 225–8491, including
two completed route cycles. All five native checkers pass: combined traversal,
draw accounting, staged selection, shared clock and index publication.

- Minimum measured free 29,588 KiB (28.89 MiB); QMP memory 67,108,864 bytes.
- Guest frame p50/p95/p99/max 33/34/34/36 ms; zero intervals over 40 ms.
- Submission p50/p95/p99/max 3/5/5/10 ms; final drain 1/3/5/17 ms.
- Slowest combined draw 20 ms at frame 301: 3 ms submission plus 17 ms drain,
  27 ms whole-frame work and 33 ms following interval. No pushbuffer wrap was
  reached in this workload; wrap instrumentation is not natively exercised.
- 1,036,801 actual index batches versus 2,419,087 under the old 240-index rule
  for the exact same submitted meshes: 57.14% fewer index submissions.
- 5,936 selection-work frames; quota stays at 2,048 entries; 346 complete
  multi-frame selections. Their stream-time p50/p95/p99/max 1/5/8/14 ms
  includes payload and animation work. Byte/read/allocation caps also pass.
- 33 staged-copy frames, maximum 256 KiB/3 ms; three publications take 1/0/0 ms.
- Eight synthetic NPCs retain complete models; no avatar gaps, missing prepared
  models, asset failures or traversal errors. All four requested clips appear.
  Appearance changes commit 32 times and cancel 64 times, with three atlas hashes.

This is the first combined moving-actor/player acceptance of the newer clock,
index-publication and spatial-selection changes. It does not attribute the frame
time difference from older builds solely to index batching. The earlier 43 ms
terrain-water drawing spike used different packs/camera cases and remains to be
measured with WXD1. No claim about emulator first-use causality or physical
performance follows. Initial boot/preload before frame 225 is outside the capture.

All 345 source hashes, 63 fixture outputs and 132 normal outputs still match after
testing. Only owned PID 47180 was quit after checking process/command line, QMP
port ownership, mounted disc and RAM. xemu emitted the previously seen GLib
g_source_destroy assertion after requested QMP quit; its log is retained. A
subsequent process inventory is empty. No new capture or server was left running.

XBE: 1,998,848 bytes, SHA256
2181f9da75bce655fe39c56533631f020264260cf21c6473ecfe7e821f756977.
Normal XISO: 499,515,392 bytes, SHA256
8e251302c12eab7b486e4104cc152361ed92c694ad83d23a0ba6a6eae223a749.
Fixture XISO: 372,047,872 bytes, SHA256
97404e8f11a07a42a13d987580025f992e759129fc60fd4a47ad69664b3b3f59.

Next: address non-looping, per-instance animation/ground contact and loading
transition presentation; retain the terrain-water draw spike as a scoped
profiling follow-up. Continue original gameplay/UI workflow implementation;
this engineering checkpoint does not close those compatibility rows.

### Reproduce the scoped checks

```powershell
python tools/check-streaming.py build/evidence/native-draw-batch-retry.csv --actors --scheduled --frame-budget --appearance
python tools/check-draw.py build/evidence/native-draw-batch-retry.csv --output build/evidence/draw-recheck.json
python tools/check-selection.py build/evidence/native-draw-batch-retry.csv --output build/evidence/selection-recheck.json
```

Use a fresh fixture/capture directory for another run. Preserve frozen source/disc
identities and the shared hardware account reservation; this offline disc has no
authentication material. No new hardware package replaces revision 2.

