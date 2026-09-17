# Distance fog

Initial implementation: shared outdoor fallback, 90 to 145 world units. The
existing cull remains 160 units; required visual selection is 165, with 175-unit
prefetch and 195-unit retention in the staged streamer. No extra render
target, texture, vertex format or per-frame allocation.

## Renderer contract

`WxFog` contains validated RGB, start/end, enable and environment fields.
`fog.c` validates finite bounded parameters and provides distance visibility,
background conversion and smooth color/distance transitions within an environment.
Indoor and underwater fallback objects are disabled explicitly. Automatic
indoor/underwater classification and their authored lighting remain unfinished;
the normal world now samples authored clear-weather fog from LIGHT.WLF and the
server login clock. Automatic environmental classification is still pending.

The shared world vertex shader writes depth times a unit multiplier to FOGC.
Actor camera coordinates are divided by scale while its view axes are multiplied
by scale, so their dot product is already world depth. The original scale
multiplier applied scale twice and was corrected after the actor-streaming run.
Uniform registers remain c0..c4, literals c5..c7; output adds one instruction
(21 total). Terrain/buildings/creatures/player parts share this render path.

The NV2A linear fog stage feeds the final RGB combiner; alpha still comes from
the lit texture result. Color register order is ABGR, unlike the ARGB framebuffer
clear. The background uses the exact same RGB. Fog resets at UI/font/map drawing,
world-pass completion and each framed portrait/preview render. Title/background
passes retain their own state setup. No post-process surface is used.

Reference: [nxdk NV2A fog tests](https://github.com/abaire/nxdk_pgraph_tests/blob/main/src/tests/fog_tests.cpp).
The implementation uses their documented linear bias/multiplier convention and
final-combiner behavior, independently integrated with this client's material state.

## Acceptance scope

Host checks cover numerical bounds, visibility, colors, smoothing and disabled
indoor/underwater fallback modes. WXPF0006 is a credential-free fixture with the
real Northshire pack and three matched camera directions, alternately fog off/on.
Assets are resident before timing; captures measure rendering only. Screenshots
must establish visible blending, alpha cutouts and unchanged readable UI. Timing
must be reported as xemu guest performance, not physical hardware performance.

Pending: authored interior fog and automatic underwater/weather classification;
world effects; live movement and material edge cases; stock-console measurement. Fog cannot fix missing geometry or streaming stalls.

The original resident numeric capture passed on September 16: 2,292 samples,
minimum free 40,144 KiB. That attempt had no visual acceptance after physical
Escape stopped Computer Use. The subsequent staged-world-streaming batch now
has actual matched `native-streaming-fog-off.png` / `native-streaming-fog-on.png`
screenshots in build/evidence. Distant trees blend into the matching horizon;
nearby surfaces, foliage cutouts and overlay text remain readable. Actual Abbey
and outdoor-travel screenshots are preserved too. This establishes the fallback
in those views, not every material, authored indoor/underwater lighting, world
effects or live avatar/portrait interactions. See STREAMING.md and latest STATUS.

## Authored Vanilla fog catalog (September 16)

`python scripts/prepare-lighting.py` reuses the pinned WoWee light coordinate and
one-based band-block helpers, with exact 1.12 DBC column counts. Private WXL1
`LIGHT.WLF` is 135,072 bytes: 374 volumes on 26 maps, 395 referenced profiles.
Resident arrays consume 135,040 bytes (131.875 KiB); all curves load once at
startup. No frame-time reads, cache churn, allocation, texture or extra surface.
The loader caps the catalog at 384 KiB, keeps 8 MiB free, validates sizes,
references, sorted keys/IDs, finite values, duplicate map defaults, and commits
atomically. Both allocation failures retain the prior complete catalog.

Fog uses color band 7 and float bands 0/1; source distances divide by 36.
Zero-radius volumes are map defaults (Northshire uses profile 12). Local falloff
uses smoothstep; the two highest weights are ordered by weight, then tighter
radius, then ID, following WoWee. Their residual fades into the map default so
leaving a lone local volume cannot snap to the default. Midnight interpolation
wraps in 2,880 half-minutes. The converter sorts authored keys (six bands needed
it); five referenced profiles contain 15 empty source bands and remain explicit
fallbacks. The asset receipt identifies them; this is not full geometry coverage.

Distance adaptation: end is capped at 145 world units, below the 160-unit cull.
Positive starts preserve authored near clarity, capped to end * 90/145. Negative
starts retain the authored start/end fraction, so rain/underwater haze is not
mistakenly discarded as invalid data. Frame transitions use the existing smooth
fog blend. Unknown interiors explicitly disable fallback fog. Supported rain and
underwater profiles are callable, but the normal client still requests CLEAR /
OUTDOOR until reliable weather and indoor/water classification is implemented.

The world reader accepts the exact eight-byte Vanilla LOGIN_SETTIMESPEED packet
before or after LOGIN_VERIFY_WORLD. Calendar fields and finite speed are checked
before publication; updates are locked, millisecond wrap is handled, logout
clears the clock, and a missing clock has an explicit noon fallback. Game speed
is minutes per real second, not seconds-per-second. Primary wire reference:
https://gtker.com/wow_messages/docs/smsg_login_settimespeed.html and pinned
vMaNGOS Server/Packets/Misc.cpp plus Objects/Player.cpp. Additional game-time
opcodes are not implemented. Full sky, sun, ambient/diffuse day/night lighting,
weather rendering and underwater/interior materials remain separate work.

Host acceptance: 28,120 checks, including actual catalog center/time/condition
sampling, malformed files, both heap-allocation failures, headroom refusal,
transactional reload, midnight and local-boundary continuity, signed starts,
clock packet/calendar/speed/tick-wrap validation. Six encrypted localhost clock
scenarios pass, including early/late publication and malformed disconnects.
No shared server/account was used. WXL2 companion telemetry is 84 bytes; the
1,472-byte main packet was not enlarged.


Native acceptance: native-lighting.csv,2626main/2625companion samples,39,488KiB
minimum free in64MiB xemu; guest frame33/34/34/79ms p50/p95/p99/max; fogwork
0/1/1/2ms. One79ms frame follows first scale comparison. Eight phases/three scaled
wolves pass; zero failures. ActualJPGs noon/midnight/dawn/rain and scale-off/on
complete scoped non-unit fog-depth visual acceptance. First receivedframe156
excludes preload. Native formatter omits fixture float labels; WXL2 has numeric
ranges. No physical input/hardware, live weather, traversal or full night-material
lighting claim. Latest STATUS/OVERNIGHT records the preserved source/disc identity.


## Authored material and sky colours (September 16, implementation batch)

WXL1 version 2 adds eight RGB bands to each profile: ambient, diffuse, sky top,
middle, band 1, band 2, smog and sun. The 374 volumes / 395 profiles consume
451,040 resident bytes (440.47 KiB), under a 512 KiB allocation ceiling. Version 1
files still load into this bounded representation with explicit palette fallbacks.
There is no frame-time catalog allocation or I/O. Rebuild privately with
`scripts/prepare-lighting.py`; its receipt lists 29 partially/fully empty profiles,
199 empty bands, source DBC hashes, and the two sun keys in profile 499 whose high
byte is ignored exactly like pinned WoWee's dbcColorToRGB. Twenty-one source bands
needed key reordering. This catalog coverage does not imply prepared map geometry.

World materials use sampled RGB ambient + diffuse times clamped normal/light dot
product. The light direction follows pinned WoWee's time calculation, with its sign
reversed because normals need the direction toward the light. Actor orientation
rotates that vector into model space; actor scale does not change its intensity.
Nine explicit shader constants replace compiler-literal patching. Cg output has
23 instructions and c0..c8 in the declared order. Framed previews and portraits
retain their own fallback lighting and disable fog, independent of world time.

A camera-relative 16 by 12 gradient mesh blends the fog horizon through authored
sky band 2, band 1, middle and top. It uses the existing UI white sample and GPU
vertex buffer, then waits for that small draw before reusing the buffer for menus.
No new framebuffer, texture or persistent GPU allocation is added. Horizon colour
matches fog exactly; looking up/down moves the sky correctly. Sky submission is
suppressed for explicitly indoor/underwater requests. The normal client still
requests clear/outdoor pending reliable environmental classification. Smog/sun
colours are retained for subsequent effects, not claimed as rendered celestial
objects. Clouds, stars, sun/moon sprites, weather, water and interior lighting are
unfinished; this is an authored colour gradient, not complete Vanilla sky parity.

Host checks pass: 74,690 lighting/clock checks with actual assets, legacy format,
volume/time/palette boundaries, allocation rollback and headroom; 2,226 UI/sky
checks including pitch, horizon continuity, queue ownership and unchanged memory.
Six encrypted localhost clock scenarios pass without using the shared account.
The protocol suite now includes those six scenarios by default. WXL3 adds palette
colours, direction, availability mask and sky quad count in a separate 136-byte
packet; the 1,472-byte main packet is unchanged and WXL2 decoding stays supported.
Native acceptance and exact candidate identity will be appended after capture.


Native material/sky acceptance now passes: 3,052 main / 3,051 WXL3 samples,
39,184 KiB minimum free, guest frame p50/p95/p99/max 33/34/34/35 ms, combined
lighting/sky work 1/2/3/4 ms. Actual noon/midnight/dawn/rain JPGs demonstrate
near-world colour changes, sky/fog matching and independent readable portrait/UI.
All three .5/1/2-scale actors render after warmup, zero asset/UI errors; explicit
indoor/underwater requests submit no sky. Captured frames154..3205 exclude preload.
Source/disc are frozen in build/material-sky-test-20260916; candidate, scope and
unmet live/hardware/environmental gates are in latest STATUS.md. No complete
Vanilla sky/material or full-world-performance claim follows from this run.
