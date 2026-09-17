# Bounded world streaming

September17 follow-up: [cooperative time slices](STREAM-CLOCK.md) now supplement
the byte quotas. Two native terrain-water cycles pass with30.02 MiB free, but
an81 ms boundary interval still fails the frame target. Its largest index cost
occurs during publication, with zero reads. Joined-table copying and scene
selection need staging; this is not yet a demonstrated performance improvement.

September 16 candidate. This document describes the world terrain/building/prop
loading path. The subsequent NPC/avatar payload extension is documented in
ACTOR-STREAMING.md. Neither establishes crowded-scene or full-game performance.

## Production path

`wx_region_update` selects up to four nearby terrain packs, preparing one index
at a time. A new pack is selected within 220 units of its tile boundary; an
existing one stays until 240 units. Header/file-size checks precede allocation.
The entry table is read in at most 32 KiB per pump, validated in at most 512
entries per pump, with at most eight small animation-header reads. The initial
32-byte header is a separate begin operation. Only the complete validated table
is attached to the scene. Cancellation closes its file and releases the table.
Final table reallocation/copy and detach compaction remain single operations;
their measured cost is included in region timing.

`wx_stream` selects nearby batches and advances one unpublished payload job per
scene. Each call has a 64 KiB read budget, eight read-operation budget, 32 KiB
maximum individual read, 64 KiB validation budget and three-commit budget.
Phases read animation metadata, reserve allocations, read texture/vertices/
indices/skin/poses, validate data, then publish the resident entry. The renderer
and collision system never see partial buffers. Shared textures and poses retain
references throughout pending work. A failed or cancelled job rolls allocations
back. Detaching another pack adjusts pending entry identity after compaction.

Required collision within 45 units can preempt an optional visual job. Visual
selection uses required/prefetch/retention distances of 165/175/195 units;
collision uses 45/55/65. Required entries always outrank optional retention or
prefetch, so hysteresis cannot occupy slots needed by nearer content. A fixed
1,024-entry membership table avoids a resident-cache scan for each world entry.
The 160-unit rendering cutoff is unchanged. Selection still scans the attached
tables when movement exceeds two units; later spatial indexing can improve it.

These are work and byte quotas, not guarantees that a physical disc read takes
a fixed number of milliseconds. File opening, allocation, cache eviction and
index commit can still stall. GPU memory writes/unloads run after the previous
GPU work has drained.

## Memory

Pending payload allocations count against the scene's existing byte budget,
including shared-buffer ownership and CPU skinning data. Every allocation batch
checks the actual Xbox free-page count against 8 MiB plus the new allocations
and a 64 KiB margin. Pending index allocation and final table replacement have
their own conservative free-memory checks. `index_bytes` reports the retained
table capacity even after detach reduces the logical count. Native free-page
telemetry includes all actual allocations; the component counters aid diagnosis.

## Shared frame budget (September 17 implementation)

The render thread now brackets region-index, world-payload, NPC-payload and
player-payload work with one shared budget. Combined limits are 64 KiB of reads,
16 read operations, 64 KiB of entry/payload validation, and three payload
allocation attempts per frame. Existing per-call limits still apply, including
the 32 KiB maximum individual read and two/three commit limits. Deferred work
keeps its unpublished buffers and complete visible fallback; a quota exhaustion
is not an asset failure. Allocation failures remain errors and retain the
existing memory guards.

With all queues active, guaranteed read shares are 8/24/16/16 KiB for index,
world, NPC and player. Validation shares are 16 KiB each; read-operation shares
are 2/6/4/4; payload allocations are 0/1/1/1. Absent queues donate their shares to
world work (or the first active queue). Unused earlier shares flow forward when
each later queue runs. This prevents earlier queues from taking a pending later
queue's guaranteed work. A repeated call cannot renew that frame's quota.

Reservations use the preceding tick's unfinished requests. Fully resident queues
do not reserve I/O; a new request may wait one frame before its queue becomes
active. Collision selection retains priority within world work, and movement
still refuses unavailable collision. NPC/player clips retain complete fallbacks.
The first index can use idle queues' shares, so it does not always receive only
8 KiB. The offline full-pack verifier refuses to drain while a frame quota is
active, preventing an exhausted-budget loop.

This bounds the coordinated reads/validation/allocation attempts, not all frame
latency. Filesystem calls, index allocation/commit/compaction, scene selection,
eviction, skinning and GPU waits still have variable cost. Avatar profile opening
and atlas composition remain synchronous and are outside these counters, as are
icons, title/preview and other UI loaders. Those remaining paths are not claimed
to satisfy the shared budget.

WXS2 extends the companion from 128 to 212 bytes with enabled state, totals and
four per-queue usage records. The header read in an index begin is included in
the global record, even though the legacy pending-index counters reset at pump.
The decoder preserves WXS1 compatibility with explicit unavailable counters.
Native acceptance is pending until the new fixture's combined gate is recorded.

## Evidence and fixture

`wowx_stream_tests` exercises large assets, partial visibility, cancellation,
detach/compaction, texture/pose sharing, malformed late payloads/index entries,
and memory/allocation rejection. `wowx_streamcheck build/xbox/world.wxi` performs
one production region and payload tick per simulated frame using actual packs.
Host allocator stubs verify lifecycle, not Xbox RAM consumption or timing.

An isolated `WXPF0007` disc runs the same collision routes and renderer in xemu:
cold index preparation; fixed fog-off/on pair; the verified boundary route at
Y=-160; an explicit relocation to Willem; Abbey entrance/interior/return; camp
and return. The relocation separates independent routes and is not a server
teleport or player input test. No credentials, account, NPC/avatar entities,
network gameplay or controller injection are involved. The first network-ready
telemetry can arrive after initial loading; subsequent reopen/transition samples
must supply index timing evidence. Visual evidence must be actual screenshots.

WXTZ stays at 1,472 bytes. Separate 128-byte WXS1 packets record frame identity,
job state, quotas, pending/resident memory, index progress, separate region and
payload timing, and traversal state. `record_telemetry.py` writes `.stream.csv`;
`check-streaming.py` correlates work with the following frame-start interval.
Its acceptance is scoped to this fixture and does not replace live-world,
physical-controller, stock-console or release qualification.

The straight south route at Y=-132.493 stops at X=-9049.047 with no floor in both
this loader and the archived unchanged fog-foundation loader. This pre-existing
route failure remains open; it was not relabeled as passing. The accepted
boundary route is the previously verified Y=-160 path.

## September 16 native result

`build/evidence/native-streaming.csv` plus `.stream.csv` contains 7,057 main and
7,056 aligned companion samples. Two complete boundary/Abbey/camp cycles passed
with zero blocked steps, traversal errors or asset/region failures. Guest frame
p50/p95/p99/max: 33/34/34/35 ms. Minimum actual free memory: 36,340 KiB in the
QMP-confirmed 67,108,864-byte guest. Region-update p99/max: 4/9 ms; world payload
streaming plus animation p95/p99/max: 9/13/21 ms. These are offline scene results,
without server/NPC/avatar workloads; they are not full-world 30 fps acceptance.

Measured read/validation maxima reached the configured quotas. Peak scene
allocation was 3,034,782 bytes, pending payload 170,962, attached table capacity
2,538,240, and pending table 1,311,744. Telemetry began at fixture frame 240, after
the first cold index load; the measured index timing comes from subsequent
unload/reopen transitions. The complete cold-boot latency remains unmeasured.

Earlier travel logs associated 350-400 ms stalls with whole-pack opening. This
run exercises reopened packs with bounded work, but excludes the actors and
network activity in those historical runs, so it is not a controlled whole-game
speedup comparison. The unchanged 64 MiB hardware release gate is still open.

## September 17 shared-budget native result

Two route cycles passed with 6,797 main frames, 6,796 complete stream/actor
companions, eight NPC fallbacks, no player gaps and no asset/traversal failures.
All shared maxima stayed within 64 KiB /16 reads /64 KiB validation /3 payload
allocation attempts. Supplemental exact frame-identity coverage passed; the
strict decoder cannot hide missing quota-violation rows behind alignment tolerance.

Minimum free32,452 KiB; frame p50/p95/p99/max33/34/37/49 ms in64 MiB xemu.
In matched fixture frames254..6721, previous maximum72 ms fell to49 ms, with
intervals over40 ms dropping97 to17. Actor work maximum40 to24 ms. Loading phases
can shift between the runs; this is not a physical or full-game benchmark.
The33.3 ms performance gate remains open. Full details, source identities and
remaining synchronous paths are recorded in STATUS.md and the candidate.


## Staged appearance consumers

Avatar layers now share AVATAR read/allocation reservations with animated payload
work. Repeated reservations cannot replenish the global quota. Composition adds
an 8,192-pixel CPU work cap, separate from indexed-entry validation. Complete
published families remain resident while replacement geometry loads.

The combined appearance run passed two cycles with 6,847 complete stream/actor
companions, 31.38 MiB free and 33/34/36/50 ms guest frames. --frame-budget now
requires exact nonterminal identities; missing/duplicate/foreign records fail.
Decoder rejection cannot conceal a quota violation. See STAGED-APPEARANCE.md.

## Staged index publication

Joined tables now copy into an unpublished buffer in 32 KiB chunks, at most 256
KiB per frame, with cooperative index-clock checks. Complete tables publish on
a later frame. Source revisions detect detach/close/replacement while copying;
cancellation frees both unpublished buffers. First-source tables transfer without
an extra allocation. WXS4 reports progress, copy quotas and total pending memory.

Three offline terrain-water cycles pass with 29,968 KiB minimum measured free.
Copy region work is at most 2 ms; final publication takes 0/1/0 ms across three
joins, versus 18–19 ms previously. Guest frame maximum remains 56 ms because
selection and drawing still block. Native actor/player integration and physical
hardware are separate open gates. Full implementation/evidence: INDEX-PUBLICATION.md.

## Staged spatial selection

Selection now shares a 2,048-entry frame quota across repeated calls, with clock
checks between groups of 32. A fixed per-scene candidate list publishes only
after the entire table is scored. Previous valid residents/payloads remain usable.
Ordinary movement finishes snapshots; large relocation and index mutation restart
them. Readiness remains false during a scan, so following frames reserve work.

Native WXS5 records progress, total, commits/restarts, revisions and entry use.
Three terrain-water cycles pass with 29.23 MiB free and 33/34/34/48 ms guest
frames. The 425 selection frames stay within 2,048 entries; 394 frames without
payload work have stream-time maximum 4 ms. First-publication draw cost remains
43 ms. Selection completion latency, moving scenes and combined actor/player
integration remain open; fixed per-frame work does not imply a hard time bound.
Implementation and acceptance limits: SELECTION.md.


## Combined moving consumers with geometry batching

DRAW-SUBMISSION.md records the first combined traversal of the newer cooperative
clock, index publication and staged selection: two cycles with eight NPCs and an
equipped Human,32 completed appearance changes and64 cancellations. No model
gaps or asset/traversal errors. All quotas,346 multi-frame selections and three
index publications pass;29,588KiB minimum free,33/34/34/36ms guest intervals.
The renderer uses57.14% fewer index submissions for the same submitted geometry.
This is not a physical performance pass or a controlled single-change FPS result.
Four normal terrain packs are used; the earlier v10 terrain-water43ms drawing
spike remains a separate profiling case. Actual relocation/loading gaps and
low-pose ground contact stay open. No shared-account or hardware-kit changes.
