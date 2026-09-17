# Staged player appearance composition

## Implemented September 17

The native avatar now builds equipment and same-profile skin/face/hair changes in
an unpublished texture set. The published body, cape, hair, extra texture, family
list, equipment key and atlas hash change together only after the entire new idle
model is resident. The old complete appearance continues animating during reads,
compositing and replacement geometry loading. Live requested animation resumes
after publication. Race/sex/profile selection still releases the previous profile
before opening the new one, so failed selection cannot show another character.

- Up to 80 prevalidated layer operations, 32 selected families and one pending
  appearance per avatar. A changed request cancels unfinished composition and
  reuses only the unpublished buffers. No queue of obsolete appearances grows.
- Reads use at most 32 KiB per call, 64 KiB and eight calls per update, also charged
  against the shared AVATAR frame lane when its scope is active. Zero grant defers.
- Compositing is limited to 8,192 pixel work units per update. A hash/alpha blend,
  mip output, copy or fill counts one pixel; mip reduction counts four input
  pixels. Row-by-row mip generation preserves the original integer rounding,
  BGRA channel order and Morton addressing. This is a work quota, not a fixed
  millisecond guarantee. Production invokes each avatar once per frame.
- One additional 2–4 texture set costs 174,760–349,520 bytes depending on profile
  bindings. It is allocated lazily with the 8 MiB headroom guard and accounted in
  avatar bytes. An allocation attempt reserves a shared allocation batch; partial
  failures release the whole unpublished allocation. Buffers are reused, swapped
  on publication and freed on profile close.
- Appearance geometry uses explicit resident-only hold IDs. They protect all
  published families, including removed equipment, until the new set is complete.
  Optional animation caches still yield to requested allocations. If replacement
  cannot fit, the old appearance stays complete and failures remain visible.
- Rejected reads never publish partial texture data. A failed request remains
  failed until the requested appearance changes. Previously published content
  remains available. Allocation pressure before a staging attempt defers work.

## Verification scope

Host tests cover partial reads, zero quotas, rapid cancellation, exact atomic
pixel/family publication, memory pressure, invalid offsets, rollback, recovery,
clip transitions and profile failure. All 3,229 prepared appearance references
across 16 race/sex profiles are checked against the original converter's body,
hair and extra hashes. Eighty legal starter outfits and the equipped Human's
idle/run/attack/death transitions are covered separately on host.

`WXN3` extends actor telemetry with composition phase, reads, pixel work,
cancellations, commits, failures, published hash/revision and layer progress.
`WXN1` and `WXN2` remain readable with unavailable fields marked -1.
`WXPF0011` extends the real-asset route with synthetic equipment removal/restoration,
skin/hair changes, repeated cancellation, and helmet/cloak visibility flags. It
checks published pointers, sampled GPU pixels, family keys and complete resident
geometry. This is offline synthetic state, not gameplay or controller input.

The `--frame-budget` checker now requires every nonterminal main frame to have
exactly matching time/frame companion identities. Duplicate or foreign identities
also fail. Dropped UDP or rejected over-budget packets cannot pass through a 90%
alignment tolerance. `--output` writes a new report when checking older evidence.

## Still open at this checkpoint

The later PROFILE-LOADING.md batch implements the profile-opening follow-up below.
This section retains the limits of the preserved staged-appearance capture.

Profile pack/metadata/looks opening and initial atlas setup are synchronous.
Appearance request planning and family/index lookup are bounded by table sizes,
but are not spread across frames. Frontend backdrop/title/icon work remains outside
the global world streaming scope. Skinning, GPU waits, filesystem latency and
allocation/eviction latency are not fixed-time operations. The new publication
path does not add unprepared equipment, unsupported forms or other missing Vanilla
appearance coverage. Full frontend/live gameplay and stock hardware gates remain.

Native acceptance passed two cycles, 26 captured appearance changes and 50
cancellations with no player gaps/failures. Minimum free: 31.38 MiB; frame
p50/p95/p99/max: 33/34/36/50 ms. See STATUS.md and the preserved candidate
20260917-staged-appearance for identities, complete results and release limits.

## Follow-up: profile opening

Next source batch should stage the remaining profile transition itself:

1. Extend a generic pack-opening job with an explicit lane, preserving full pack
   header/source identity and the contiguous animation-header table optimization.
   An empty scene can take ownership of its validated table without copying it;
   terrain attachment still needs its separate combined-table path.
2. Read/validate WXA metadata and WXLK catalogs in bounded slices. Preserve exact
   range and duplicate checks, size guards and all currently rejected fixtures.
3. Build a bounded family membership index during validation to avoid rescanning
   the whole character index for each geoset and item component.
4. Keep the synchronous verifier as a drain of the same validated stages, outside
   a frame scope. Frontend/world production should pump, cancel and retry explicitly.
5. Preserve the current rule that a new race/sex/profile cannot display the old
   character. Same-profile publication already has the separate atomic path above.
6. Measure cold open, rapid roster/race changes and profile failure alongside the
   existing same-profile customization/appearance test. Keep native captures tied
   to their frozen sources and disc; do not reuse the active capture for new code.
