# WMO baked vertex lighting

Scoped accepted batch: build/candidates/20260917-vertex-lighting, captured with
build/vertex-lighting-test-20260917. Full Vanilla parity is unfinished.

## Behaviour

Original WMO MOCV RGB colours now reach the Xbox vertex shader. Interior batches
use baked colour with the root's MOHD ambient floor; exterior batches multiply
directional lighting by the authored tint. The cooker reuses the lighting policy
in the [pinned WoWee shader](https://github.com/Kelsidavis/WoWee/blob/19776ef0a8d0377b3cca0264a78a39c9b27bf600/assets/shaders/wmo.frag.glsl):
interiors floor RGB at max(root ambient, 0.15), lit exteriors at 0.25, and unlit
exteriors retain white. Colours are rounded to RGBA8 with at most half a byte of
quantization error per channel. Interior batches bypass the outdoor sun lighting.

MOCV alpha does not become surface opacity. Texture and material alpha keep their
existing blend/cutout behaviour. Groups without vertex colours retain constant
white, and following actors/UI/previews explicitly restore that state. WMO texture
clamping flags are also retained.

This follows WoWee's interpretation, not a verified recreation of the original
client's complete colour-alpha fixup. Dynamic shadows, specular terms, local lights,
per-material complex shaders, doodad placement lighting and automatic interior/
underwater camera classification remain separate work. M2 light cards and water
presentation are not fixed merely by adding building vertex colours.

## Format and bounded runtime

WXP v8 retains the 32-byte header, 64-byte entry and 32-byte geometry vertex. Flag
bit 17 adds four RGBA bytes per vertex before the optional v7 material-motion
block; that motion block still immediately precedes vertex geometry. Bit 18
marks baked lighting. These flags are valid only for static, nonanimated v8
entries. All earlier supported versions 4 through 7 remain readable.

Only resident colour arrays allocate GPU-visible memory. Allocation, pending
bytes, the scene budget and free-memory guard include the entire array. Reads
consume the existing shared frame quota. An entry remains unpublished until all
geometry, motion and colour reads finish. Cancellation, eviction, truncation and
partial allocation failure release the new array. Every RGBA byte is inherently
finite; colour arrays need no floating-point validation scan. No render target or
per-frame heap allocation is introduced.

The NV2A consumes normalized RGBA8 through vertex attribute 3; missing arrays use
an explicit constant white value. Texture/material alpha bypasses vertex alpha.
Repacking preserves exact colour/motion data while compressing texture pixels.

## Host acceptance

Twenty original global packs in build/vertex-lighting-packs-20260917 total
426,894,490 bytes. There are 6,116 coloured batches, 6,374 baked-light batches and
1,874,988 colour vertices (7,499,952 additional bytes on disk). All 57,991 entries
retain exact geometry, indices, mip pixels and preceding material-motion data.
New colours and WMO baked/clamp flags are the only rendering-data changes.
Map 450 remains rejected for the original referenced NaN UV; it was not repaired.

Checks passed: 2,170 lighting quantization, colour-chunk boundaries, publication,
allocation/rollback and cancellation checks; 5,071 material-motion, 453 runtime,
1,467 avatar and 5,144 staged-streaming checks; 50 repacker checks, including
combined colour/motion prefix preservation and deliberate corruption; 14 Python
telemetry tests. Both host tools and the native XBE/XISO build successfully.

The first comparison summary reused the field name bytes for pack size and colour
size; its prepared-byte total therefore incorrectly showed 7,499,952. The geometry
comparison itself passed. The original summary is preserved as
vertex-lighting-summary-field-error.json, and the corrected report separates
pack_bytes from colour bytes without repeating the geometry test.

## Native acceptance

One combined offline xemu run passed: 6,008 main samples, frames 237..6244, with
6,007 each streaming/material companions and complete nonterminal identity
coverage. Three full cycles covered original maps 34, 43, 369, 389, 409 and 429
plus synthetic chart 999. Every map reached resident render/collision holds,
without asset/region/fixture errors or quota violations. The chart has 543 held
fog-on/off samples; original WMO colour/baked flags were also submitted. Original
Dire Maul material motion continues to vary (123 selected held samples, 121 hashes).

Confirmed 64 MiB guest; native scale 1, disposable disk writes. Minimum free memory
was 35,680 KiB (34.84 MiB). Guest frame p50/p95/p99/max: 33/34/34/44 ms. The single
interval over 40 ms followed frame 3294, while loading the fog-on synthetic chart:
draw 40 ms, streaming 0 ms, work 43 ms. Its cause remains unproven. All cold samples
are retained; this is not a matched benchmark or physical hardware measurement.
Index work: 0/0/4/7 ms; payload work: 0/10/11/14 ms. Shared maximum usage stayed at
65,536 read bytes, eight reads, 65,536 validation bytes and three allocation
batches. Scene residency peaked at 6,577,050 bytes; current/pending index each
533,952 bytes. Full live-world 30 fps is still unproven.

Actual screenshots show the expected red/green/blue/white corners and interpolated
gradients, lit versus baked intensity, material blending, fog and an unaffected
white/grey backing board and UI. Dire Maul walls now carry blue authored lighting;
the Tram bridge has its shaded tint. The Tram's overbright distant cards/underwater
scene remain visibly incomplete. Fog-on chart and Tram images named postcapture
were taken after telemetry ended with the same frozen source/disc; the Dire Maul
and fog-off chart images were captured during the recorded scenario.

All 321 sources /17 fixture outputs and 321 sources /132 normal outputs matched
after capture and screenshots. The recorder ended at duration_limit. Only owned
xemu PID 3852 was quit after executable/disc/QMP ownership checks; no emulator
remains. Its post-QMP GLib shutdown assertion is preserved. The source ZIP, both
build receipts and every private asset dependency identity are preserved.

Synthetic map switching is not controller input, gameplay, navigation or hardware
acceptance. The shared account/server, stable hardware kit and earlier candidates
remain untouched. Next work includes general world environment classification,
water/local-light and complex-material coverage, spatial selection, original UI
and all-class gameplay. Every full-port and stock-hardware release gate stays open.
