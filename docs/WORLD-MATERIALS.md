# World materials - September 17, 2026

This batch extends the native renderer and reusable converter. Full Vanilla
rendering, gameplay and interface parity remain incomplete.

## Implementation

WXP version 6 adds explicit material flags without enlarging the 64-byte entry or
32-byte vertex. The runtime still accepts version 4 and 5 assets. New semantics
in old versions, conflicting cutout/blend flags, unknown modes and material flags
on collision entries are rejected before publication.

The pinned WoWee M2/WMO loaders supply original material IDs and flags. The
converter retains modes 0 through 6: opaque, alpha key, alpha blend, additive,
alpha additive, multiply and multiply by two. Additive textures retain their
authored mode even when their alpha is uniformly opaque. Only alpha key uses the
alpha test; transparent materials stop writing depth by default. M2 depth-test
and depth-write exclusions, unlit materials and unfogged materials are retained.
The animated-asset producer emits the same metadata for future preparations;
existing avatar and NPC packs remain unchanged.

Each world draw collects at most 320 resident batches on the stack (2,560 bytes).
Opaque/cutout draws precede transparent draws, which are stably sorted from far
to near by camera-space center. No extra render target or GPU allocation is
introduced. Unlit batches use white ambient and zero diffuse. Fog preserves
texture alpha and fades additive output toward zero, multiply toward one and
double-multiply toward one half. Render completion restores world lighting,
fog, depth writing and opaque blending before another actor/UI/preview pass.

This is single-texture material support. Multi-texture WMO shaders, texture
animation, M2 colour/opacity tracks, authored vertex colours and local lights/fogs still need work. Face
culling remains disabled. Center sorting cannot resolve intersecting transparent
triangles or transparency spanning separate actor/world draw calls.

## Asset evidence

Private version-6 outputs: build/world-material-packs-20260917. Twenty global WMO
packs pass the runtime asset verifier. All 57,991 entries have byte-identical
geometry and mip pixels compared with the prior accepted batch; 5,397 entries
have changed material flags. Per-map reports now count all seven blend modes,
lighting/fog/depth flags and unsupported complex shaders separately.

Map 450 remains excluded. Its failure is now localized to original group
world/wmo/kalimdor/ogrimmar/kl_pvpbarracks_010.wmo, raw vertex 9601. The source MOTV
contains NaN in both UV components (bytes 00fcffff00fcffff); four nondegenerate
triangles reference it. This is not an unused vertex or a transform-origin
failure. Raw inspection evidence is retained in
build/evidence/world-material-map450-raw.json. No triangles or source coordinates
were silently discarded or replaced. A documented repair or clean-source
comparison remains necessary before this map can pass preparation.

The normal development disc retains the previously prepared terrain/avatar
assets. New global packs are isolated on the acceptance fixture; the stable
hardware package and shared-account saved state are unchanged.

## Verification scope

Host checks cover blend arithmetic and fog identity colours, stable bounded
ordering, material/version corruption, allocations, staged streaming and avatar
compatibility. Repacking preserves material flags and exact geometry while
changing texture encoding. The initial repacking test found a stale v5-only
header check; the corrected check and rerun are retained alongside that failure.

The resident Tram screenshot still exposes very bright light cards and missing
water/environment presentation. Correct blend factors alone do not reproduce
authored opacity/colour tracks or underwater lighting. These remain explicit
visual gaps, and the screenshot is not evidence of original-client parity.

The combined native scenario cycles the five prior global inspection surfaces
and a synthetic material chart. Fixture-only map 999 is not game content. Its
two rows cover all seven modes, lighting/fog exclusions and depth flags, with fog
alternating each cycle. Main, streaming and material telemetry are matched by
frame identity; a missing companion fails acceptance. Material counts establish
GPU submission, not pixel-exact original-client equivalence. Actual screenshots
are required to assess appearance and UI state restoration.

Native results are recorded in the checkpoint after the combined run. The test
does not authenticate, move saved characters, exercise instance gameplay or
verify a physical controller. xemu timing remains separate from stock hardware.


## Combined native result

Capture: native-world-materials.csv. Five original global inspection maps and the
synthetic seven-mode chart pass the scoped renderer/routing checks. 7,359 main and
7,358 each streaming/material records have complete nonterminal frame coverage.
Five cycles, 905 held chart samples with both fog states, no fixture/asset failures.
Actual fog-on/off screenshots and a resident Tram view are retained.

| Measure | Result |
|---|---|
| Guest configuration | 64 MiB, native scale, disposable writes |
| Minimum measured free | 36,180 KiB /35.33 MiB |
| Frame p50 /p95 /p99 /maximum | 33 /34 /34 /135 ms |
| Index work p50 /p95 /p99 /maximum | 0 /0 /4 /8 ms |
| Payload work p50 /p95 /p99 /maximum | 0 /10 /12 /16 ms |
| Shared read /validation maxima | 65,536 bytes each |
| Shared read operations /allocation batches | 8 /3 |

The only interval above 40 ms followed the first Tram double-multiply draw: 123 ms
render wait and 134 ms total work. Later visits did not repeat it. First-use host
pipeline creation is a hypothesis, not a measured cause. Keep the spike in the
performance report and investigate; this batch does not pass full-world 30 fps.

Candidate: build/candidates/20260917-world-materials. Sources and both disc receipts
matched after the run; only owned xemu PID 58028 was quit. Its post-quit GLib
assertion is preserved. No emulator remains and the shared account was not used.
