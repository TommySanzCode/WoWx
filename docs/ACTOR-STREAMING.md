# Bounded NPC and player animation payloads

The staged payload loader is shared by terrain, NPC templates and modular player
components. NPC/player calls have the same 64 KiB read, eight-operation and
64 KiB validation budgets as world calls, with two completed batches per call.
Each scene owns one pending job. Scene budgets and the actual 8 MiB free-memory
guard are unchanged. The offline pack verifier alone drains a job synchronously.

Pending geometry/texture/skin/pose data never becomes a resident entry until
validation finishes. NPC rendering falls back to a complete prior animation for
the same template. The player renderer switches all selected body/equipment
components to the new clip together. Pending jobs pin shared textures and pose
buffers; obsolete requests cancel and roll back before slots are reused.

NPCs and the player keep the idle/run partner when the existing memory guard allows it.
Rapid idle/run requests also finish an in-flight partner for the same component
instead of cancelling its partially read data every frame. A third animation or
a changed appearance can cancel obsolete work. Allocation rejection stops this
call at its first failure and retains the complete fallback; subsequent calls can
retry. This changes diagnostic failure counts from the former two-failures-per-
call behavior, without weakening allocation limits or hiding a failed request.

## Verification scope

Host cases cover large payloads across multiple calls, per-call quotas, late
malformed data, partial invisibility, shared data ownership, cancellation/detach,
allocation rejection, complete modular transitions and rapid idle/run requests.
The real equipped Human profile is checked through idle/run/attack/death and back.

WXPF0008 adds eight synthetic NPC instances from six prepared templates and an
equipped Human to the existing world boundary/Abbey/camp fixture. Positions follow
the deterministic route, and clip requests cycle independently through idle, run,
rapid idle/run, attack and death. These are deliberately synthetic states, not
server combat, saved-character updates or player input. Native evidence must
report complete fallback counts, player gaps, memory, quotas and frame timings.

The original 164-byte WXN1 companion adds per-scene quotas/pending memory,
clip/fallback state and combined actor timing. WXN2 is 196 bytes and adds per-scene
sampled/skipped batches, skinned vertices and palette builds. The decoder retains
WXN1 compatibility with -1 for unavailable new counters. Recorder writes `.actors.csv`. The production
client also reports these resource counters; synthetic ready/fallback/gap/switch
counters are only populated in the fixture and are zero in normal-client packets.
WXTZ remains unchanged at 1,472 bytes.

## Remaining work

Byte quotas do not eliminate filesystem or allocation latency. Actor catalog and
avatar profile opening, appearance composition, index selection, skinning and draw
work have separate costs. Those are not bounded by the payload reader's limits.
NPC request coalescing, distance-based animation cadence and shared palette
computation are implemented as described below; broader NPC/equipment coverage,
further scheduling and representative crowded combat remain outstanding. A scoped
synthetic scene cannot prove all-class, server-persistence or physical hardware
performance. The original gameplay and UI parity goal remains unchanged.

## September 16 native result

`native-actor-streaming-retry.csv` completed two boundary/Abbey/camp cycles: 7,112
main and 7,111 world/actor companion samples, zero asset/region/traversal failures,
all eight complete NPC models retained after warmup, and zero incomplete-player
gaps across 964 synthetic clip changes. Idle, run, attack and death are observed.
All per-scene read/operation/validation quotas are respected. NPC rapid idle/run
requests still cancel obsolete payloads; only the player's paired pending clip
gets retention. NPC request coalescing remains an optimization opportunity.

In 64 MiB xemu, minimum actual free memory was 32,968 KiB. Guest frame p50/p95/
p99/max: 33/34/36/55 ms. Actor streaming plus skinning p50/p95/p99/max: 5/15/20/
34 ms; region maximum 9 ms, world payload plus animation maximum 23 ms. Peak NPC
payload allocation was 2,088,446 bytes; avatar payload plus auxiliary allocation
1,203,140. Pending maxima were 383,580 and 353,376 respectively. The frame result
does not demonstrate sustained full-game 30 fps, representative city/raid load,
or physical hardware performance.

The first launch exited with host exception 0xc000041d before telemetry, with its
log stopping during host graphics initialization. Failed capture/lifecycle,
Windows application event and launch files are preserved. The same unchanged
disc succeeded on retry. No client success is inferred from the failed launch.
Actual screenshots preserve the equipped player/NPCs in run, attack and the
completed route. They show synthetic fixture units, not a live gameplay session.

## September 17 animation scheduling batch

Pending NPC idle/run entries can finish when the matching template switches to
its partner. Complete optional partners stay in the bounded cache, avoiding
repeated reads during short movement changes. Unrelated templates and other
sequences cancel obsolete pending work. The last complete fallback is protected;
optional partners are removed as whole clips before a requested allocation needs
their memory or slot. A conservative reservation includes unshared texture/pose
bytes; actual allocation still accounts for sharing. Tight system headroom disables
optional retention. Allocation errors remain reported, not hidden.

Visible NPC idle/run animations sample every frame within 35 camera units, every
66 ms through 80 units, and every 100 ms farther away. Selected units and other
animation sequences always sample the current frame. Requests for a shared
template use the fastest visible instance's cadence. Every part of a template
uses the same timestamp; reusing a sample skips vertex skinning for that part.
Gameplay state, movement, targeting and model visibility still update every frame.
The player and framed previews retain full-rate sampling.

Consecutive parts sharing the same pose data, bone/frame counts, duration and
sample time reuse one interpolated matrix palette. Only the existing 12 KiB
palette is staged; no extra matrix cache allocation occurs. Per-resident sample
identity adds eight bytes, plus sixteen bytes of counters per scene. Native free
page measurements must still cover this static structure growth.

Host checks include large multipart alternating NPC clips, retained fallback,
whole optional-clip eviction, low headroom, unrelated cancellation, exact cadence
boundaries, focused/combat priority, duplicate template requests, timestamp wrap,
shared palettes and repeated-sample work avoidance. The prepared equipped Human
passes all four clips and returns. Avatar profile opening and atlas composition
remain synchronous; this batch does not claim those stalls are fixed.

The updated WXPF0008 fixture places unique templates at middle/far distances and
periodically focuses the far unit, while a duplicated template has near and far
instances. It retains the boundary/Abbey/camp route and rapid clip changes. Native
results are pending until the current frozen capture passes its combined gate.

## September 17 native result

The updated near/far fixture passed 6,500 main frames, one full route cycle plus
part of the next, eight complete NPC fallbacks and no player gaps or asset/route
failures. Across 862 clip requests, NPC cancellations stayed zero. Distant work
was skipped on 2,467 frames (up to five batches); twelve player parts reused one
palette. Minimum free32,456 KiB in64 MiB xemu; frame p50/p95/p99/max33/34/42/72 ms,
actor work5/7/22/40 ms. This is feature acceptance, not the performance release gate.

The72 ms worst interval coincides with both NPC and avatar reading64 KiB and
13 ms world work in the preceding frame. The earlier55 ms fixture used different
placements/rendering, so no overall speedup is claimed. Cross-scene streaming
budgets, synchronous profile opening/composition and physical performance remain
open. See STATUS.md and actor-cadence-stall-analysis.json for exact evidence.
