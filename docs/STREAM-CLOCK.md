# Cooperative streaming time slices

September 17 candidate, separate from both delivered hardware packages.

## Problem and change

The terrain-water capture recorded a 69 ms guest frame after 32 ms of combined
index/payload work and 33 ms drawing. Byte/read/scan quotas bounded the amount
of work but could still permit several slow operations in the same frame.

The production streamer now checks a monotonic millisecond clock before further
payload work, reads, validation batches, allocation reservations and index
publication. Each active queue has its own cooperative time slice:

| Queue | Initial slice |
|---|---:|
| Region index | 2 ms |
| World payloads | 6 ms |
| NPC payloads | 2 ms |
| Player appearance/payloads | 2 ms |

Unused **inactive** queues donate their slices to the world queue, or the first
active queue if world loading is inactive. Active queues keep independent start
times, so a slow early queue cannot consume the later queues' opportunity to
progress. The sum of configured slices is 12 ms. Existing byte/read/validation/
allocation quotas still apply. Expiry is sticky until the next frame; repeated
calls or attempts to replace the clock mid-frame cannot reset it.

The native executable registers GetTickCount through a small platform callback.
Offline host tools retain their existing synchronous behavior unless they
explicitly install a test clock. Pending buffers remain unpublished across a
yield and keep their existing allocation/headroom accounting.

Two deadline boundaries required explicit handling: region preparation now
distinguishes budget deferral from a bad file, and environment-footer reads
check that the actual grant still succeeds after checking for enough room.
Expiry between those checks cannot consume an uncharged partial header or
report a false asset failure.

## Limits and telemetry

This is cooperative scheduling, **not a hard frame-time guarantee**. A read,
allocation or copy already in progress cannot be interrupted. Whole-index
publication/compaction, scene selection, animation, composition CPU work,
collision, GPU waits and drawing still need separate optimization.

WXS3 preserves the WXS2 prefix and appends clock-enabled state, a four-bit yield
mask, four configured slices and four observed elapsed times (252 bytes total).
An observed span ends at a budget check, not at completion of the last I/O, and
can include gaps between calls to that queue. It must not be summed as exact
streaming CPU time. Existing region/payload/actor timers and next-frame intervals
remain the measured costs. Oversized observations are retained as evidence,
rather than rejected as malformed telemetry.

The decoder keeps WXS1/WXS2 support and marks unavailable clock fields with -1.
Native acceptance requires complete companion identities, enabled quotas,
consistent slice/yield accounting, measured memory headroom and exercised yields.
Rendering, assets, input behavior and server state are unchanged by this batch.

## Host/build evidence

- 5,317 streaming checks pass, including expiry, wraparound, independent later
  queues, inactive donation, repeated-call protection and payload publication.
- A deterministic slow-clock payload case completes in 34 frames with 33 timed
  yield frames; vertex/index/texture/skin/pose bytes match the original buffers.
- 806 environment checks pass, including expiry between footer room-check and
  grant, recovery on later frames, metadata bounds and allocation rejection.
- The existing liquid suite passes; eight streaming-telemetry test groups cover
  WXS3 truncation/accounting and backward compatibility.
- The native XBE and normal XISO build successfully. Initial synthetic clocks
  were too fast to trigger expiry before byte quotas (the later diagnostic
  completed in 27 frames with zero yields); those failed test logs are retained.
  The final slow-clock case explicitly exercises expiry without relaxing its
  completion or exact-payload assertions.

## Native acceptance

The frozen fixture is `build/stream-deadline-test-20260917`: the same ten terrain
water/environment camera cases and identical private packs used by the prior
terrain-liquid capture. It contains no credentials or replay input. Source ZIP
and a 334-source receipt were frozen before launching one owned xemu process.

The completed capture passes the terrain/liquid/environment and clock gates:
5,394 main frames and 5,393 complete companions, frames 210–5603, two complete
ten-case cycles, zero asset/region/fixture errors. All 30 liquid texture frames
are still submitted. Measured minimum free memory is **30,736 KiB (30.02 MiB)**
in a QMP-confirmed 67,108,864-byte guest, native rendering scale and disposable
disk overlays. World work yields on its time slice in **1,432 frames**.
No NPC or player workload was included; their clock behavior has host coverage,
and combined native coverage remains pending.

Guest frame p50/p95/p99/max: **33/34/34/81 ms**, with two intervals above 40 ms.
Index time reaches 19 ms and world payload/selection time reaches 22 ms. This
does **not demonstrate a frame-time improvement** over the prior 69 ms maximum.
The remaining spike is more precisely localized:

- Frame1903 finishes validation (39 records, zero index reads), publishes the
  combined 2,549,824-byte table, and records 19 ms region work. The subsequent
  world selection takes 7 ms with zero payload reads; drawing takes 52 ms.
  Total work80 ms precedes the 81 ms interval.
- The next lap's corresponding boundary frame4107 records 18 ms region work,
  7 ms selection and 20 ms drawing, preceding a 50 ms interval.
- Both occur with the world read queue inactive and without a clock yield.
  A new limit on additional reads cannot preempt table reallocation/copy,
  selection or drawing. Separate read operations also overrun their slices;
  observed world spans reach22 ms before the next gate can yield.

Next: stage joined-index allocation/copy and atomic publication, with cancellation
and peak-memory accounting; then stage spatial selection and investigate the
draw/GPU-wait component. Any extra temporary table must retain the 8 MiB headroom
gate. Test the combined NPC/player consumers before adopting the clock batch for
live gameplay. Smaller byte quotas alone are not established as the solution.

Only owned xemu PID63332 was quit after the completed capture and exact
executable/disc/QMP ownership checks. Source, source ZIP and fixture/normal
receipts match (334 sources,14 fixture outputs,132 normal outputs). No new visual
appearance acceptance is claimed: this batch's evidence is render counters and
timing, with unchanged geometry/material code and assets. Physical hardware and
combined live gameplay remain separate gates; both delivered hardware packages
and the shared account/server remain unchanged.
