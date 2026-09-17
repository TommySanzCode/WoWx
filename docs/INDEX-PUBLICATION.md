# Staged world-index publication

The previous terrain-water capture localized an 18–19 ms boundary pause to
joining world index tables. The new loader keeps the current table published
while it prepares the adjacent table in a separate buffer.

- Read and validate the new source with the existing quotas.
- Allocate the joined table only after checking measured free memory against
  the 8 MiB reserve plus allocation size and a 64 KiB margin.
- Copy at most 256 KiB per frame in 32 KiB chunks, also respecting the index
  queue's cooperative time slice. Repeated calls share the same frame quota.
- Publish the complete table atomically on a later frame. Existing geometry
  and cache indices remain usable throughout preparation.
- Cancel an unwanted join safely; restart preparation if a source was detached,
  closed or replaced while copying. First-source tables transfer directly.

Telemetry WXS4 appends copied bytes, total/progress, allocated join bytes and
restart counts. Main cache totals now include world and NPC index tables,
region cells and both unpublished buffers. Actual free-memory telemetry remains
the hardware-budget criterion; host allocator simulations are separate.

Host tests cover exact joined contents, bounded repeated calls, atomic
publication, cancellation at every publication phase, scene mutation,
allocation failure and insufficient headroom. Runtime, avatar, preview,
environment, liquid and streaming consumers pass. One older runtime test
assumed the second tile appeared in a single call; it now waits a bounded number
of updates while asserting that the first tile remains available. Its original
failure log is retained.

Whole-buffer allocation, freeing, synchronous offline attach, detach compaction,
spatial selection and draw/GPU waits remain blocking. This work does not establish
a hard frame limit or complete the full-game performance gate.

## Native result — September 17

Candidate: `build/candidates/20260917-index-publication`. The frozen ten-case
terrain-water fixture completed three cycles: 6,149 main / 6,148 exact companion
samples, frames 241–6389, zero asset/region/fixture errors. All 30 original liquid
texture frames were submitted. QMP confirmed 67,108,864 bytes RAM and native
rendering scale; the dedicated emulator used disposable disk overlays.

Thirty copy frames copied at most 262,144 bytes each and took at most 2 ms in
region work. The three 2,549,824-byte publications took 0/1/0 ms, compared with
18–19 ms in the previous capture. Whole region-update maximum is still 10 ms.
Minimum measured free memory was 29,968 KiB (29.27 MiB), including the temporary
joined table. Peak pending index ownership was 3,781,120 bytes.

Guest frames p50/p95/p99/max: 33/34/34/56 ms. At the first publication, selection
still took 8 ms and drawing took 46 ms; its following interval was 56 ms. Later
publications had 7 ms selection and 33 ms following intervals. This isolates the
index improvement; it does not demonstrate full-world or physical 30 fps.
World queues exercised 456 cooperative time yields. Native NPC/avatar clock
consumers remain outside this fixture; their host checks are separate.

`tools/check-index-publication.py` verifies exact telemetry identities, copy
quota/progress, pending-memory accounting, old-table stability and three atomic
publications. Global-world and stream-clock checks also pass. Source ZIP and
both receipts verify 336 sources, 14 fixture outputs and 132 normal outputs.
Only captured project xemu PID 63464 was quit after executable/command/disc/QMP
owner checks. No emulator remains. No screenshots were added; geometry/materials
are unchanged, and these measurements do not add visual-parity acceptance.

XBE SHA256: `2d7b84b03071fad2c02ca3eab7d444fadab3e56683f093189e9a6eee1379775d`.
Next: stage spatial selection and investigate draw/GPU waits, then combine native
actor/player workloads after the shared account is released. The user's new
hardware request packages this executable separately as September 17 revision 2;
all earlier kits, source/disc identities and saved characters stay preserved.
