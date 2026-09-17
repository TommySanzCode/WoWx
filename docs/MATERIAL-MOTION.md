# Original M2 material colour, opacity and UV motion

Scoped accepted batch: build/candidates/20260917-material-motion. This is a
world-doodad material extension; full Vanilla gameplay/visual parity is unfinished.

## Shared data and renderer

The converter reads the original Vanilla RGB, fixed16 colour-alpha and texture
weight tracks, plus the pinned WoWee UV translation tracks. It follows the model's
idle sequence and each track's independent global-sequence period. Exact step and
linear keys are retained; constant RGB/alpha/weight tracks fold into defaults.
Unneeded default-white/opaque/zero-offset blocks are omitted. Invalid ranges,
nonfinite values, unsupported interpolation and excess key counts fail explicitly.

At runtime, visible batches sample colour and opacity and shift their texture UVs.
Material alpha is colour-alpha multiplied by texture weight. Fully hidden batches
are omitted; remaining material colour multiplies lit texture output. Authored
texture wrap/clamp flags are retained. World draw completion restores white tint,
full opacity and zero UV offset for subsequent actors/UI/previews.

The current extension handles UV translation. UV rotation/scale bindings remain
listed separately in preparation reports. Skeletal doodad motion, billboard bones,
multi-texture shaders, local lights/vertex colours and water remain unfinished.
Prepared actor clips still require their own material-track integration and native
acceptance; an idle world-doodad result does not establish all-class/model parity.

## WXP version 7

The 32-byte pack header, 64-byte entry and 32-byte vertex are unchanged. Flag bit
14 marks an optional 2,652-byte material block immediately before that entry's
vertex payload. Flags 15 and 16 select clamping on U and V. Existing placement,
animation and mip fields keep their meaning. Version 4, 5 and 6 files remain
readable; new flags in older versions are rejected.

The block contains a magic, key count, RGBA/weight defaults, four track descriptors
and at most 128 original keys total. Track descriptors hold first/count, period
and step/linear plus local/global-clock flags. Each key holds milliseconds and
four floats. Periods are bounded to one day. No lossy resampling is used to force
an oversized track into the limit.

Only resident materials allocate a block. Its CPU allocation is included in scene
and pending-byte budgets before allocation. Reads and validation consume the
existing shared frame quotas; malformed data cannot publish a drawable batch.
Cancellation, eviction and partial-allocation failures release the block. No new
render target or per-frame heap allocation is introduced. At the 256 render-slot
limit the optional blocks alone would consume about 663 KiB, within the existing
scene budget and measured 8 MiB free-memory gate.

Pack optimization copies material blocks and exact geometry while converting mip
pixels. Independent comparison rejects altered motion data or a format downgrade.
The WXM2 telemetry companion counts submitted material blocks and hashes sampled
uniforms, alongside the original seven blend counts. Those counts establish
submission and variation; actual screenshots assess presentation.

## Validation

Host checks compare the native sampler with pinned WoWee results across independent
local/global times, test step boundaries and stationary globals, parse raw fixed16
alpha, and reject malformed keys/ranges/versions. Runtime checks cover publication,
material allocation accounting, failure rollback and insufficient budgets. The
combined native scenario uses original global maps plus a separately labelled
synthetic chart with animated RGB/alpha/UVs.

Host validation passed 5,071 material-motion checks (including comparisons with
the pinned WoWee sampler), 453 runtime, 1,467 avatar and 5,144 staged-stream checks;
44 repacking checks and 11 Python telemetry tests also passed. One converter
failure exposed an inert Vanilla UV lookup with no transform table. The converter
now treats that absent table as the same no-op as the pinned renderer; a binding
outside an existing table still fails. Both the rejected attempt and corrected
results are preserved.

Twenty original global packs were regenerated in
build/material-motion-packs-v2-20260917, totalling 419,394,538 bytes. All 57,991
entries retain exact geometry and mip pixels. The added 1,370 material blocks
occupy 3,633,240 bytes on disk; 78 batches animate (40 in Blackfathom, 38 in Dire
Maul). None of these twenty maps contains translated UV tracks or nonidentity UV
rotation/scale. UV translation is therefore tested with synthetic data only.
Map 450 remains rejected on its original, referenced NaN UV; no geometry repair
or source substitution was made.

## Combined native acceptance

The frozen fixture build/material-motion-test-20260917 switched between six
original global maps (34, 43, 369, 389, 409, 429) and synthetic chart 999. This is
offline diagnostic map switching and camera rotation, with no account or input.
The Dire Maul point (13.1645545959, 39.8554916382, -33.5449695712) was selected
beside an authored material and checked against original collision triangles;
it is not a verified entrance or navigable instance route.

native-material-motion.csv contains 7,062 main samples, frames 206..7267, and
7,061 each streaming/material companions with complete nonterminal identity
coverage. Four full cycles completed. All maps reached resident render/collision
holds, with no asset/region/fixture failure or streaming-quota violation. The
synthetic chart has 724 held samples across fog-on/off, 14 submitted motion
blocks and 659 sampled uniform hashes. Dire Maul has 164 selected held samples
with authored motion and 162 uniform hashes. These prove submission/variation;
they do not establish pixel-exact agreement with the PC client.

In the confirmed 64 MiB xemu guest, minimum free memory was 35,912 KiB (35.07 MiB).
Guest frame p50/p95/p99/max was 33/34/34/43 ms. The sole interval over 40 ms followed
frame 3282: synthetic fog-on chart loading, draw 38 ms, streaming 2 ms, total
work 42 ms. The cause is not established. This is not a matched benchmark against
the previous 135 ms cold spike, and neither result is a hardware measurement.
Index work p50/p95/p99/max was 0/1/5/9 ms; payload work was 0/10/14/19 ms. Maximum
shared usage stayed at 65,536 read bytes, eight reads, 65,536 validation bytes and
three allocation batches. Scene residency peaked at 6,464,434 bytes; current and
pending indices each peaked at 533,952 bytes.

Actual native screenshots preserve two visibly different chart phases, fog-on
presentation, original Stockades geometry, the Dire Maul brazier and the Tram.
The first chart image was taken during telemetry; files named postcapture were
taken afterward with the same sources/disc and no injected input. The chart's
cutout column is discarded because its animated alpha/weight and texture alpha
stay below the cutout threshold; the previous v6 chart covers visible cutouts.
The Tram still shows excessive brightness and missing underwater presentation.
Material tracks alone do not solve local lighting, complex shaders or water.

All 319 sources and 17 mounted fixture outputs matched after capture, as did 319
sources and 132 normal build outputs. Source ZIP and asset dependency identities
are preserved. The recorder ended at duration_limit; only owned xemu PID 16132
was quit after executable/disc/QMP checks. Earlier checkpoints and the stable
hardware kit remain unchanged. No shared-account or server state was modified.

Next work: carry original WMO environment and lighting/material data into bounded
world selection, address the Tram presentation, and extend the reusable terrain
pipeline. Actor material clips, full live interface/combat acceptance and stock
hardware/controller gates remain open. This scoped batch does not complete M2
visual parity, any instance gameplay gate, or the full port.
