# Player and target HUD candidate

## Implemented behavior

The native client now builds player and target frames from its existing bounded
entity snapshots. Names, levels, health, active resource and XP respond to the
same Vanilla updates used by gameplay. It handles target disappearance/clearing,
zero/unknown maxima, death and ghost states. Bars clamp their fill to [0,1]; long
names use ellipses and large counters switch to percentages instead of overflowing.

The active resource comes from UNIT_FIELD_BYTES_0 byte 3, not from a class guess.
The five Vanilla slots are mana, rage, focus, energy and happiness. Current values
start at field 23, maxima at 29; health/max-health/level are 22/28/34. Rage text
converts the wire's tenths to displayed points; bar fill and telemetry retain the
wire precision. Unknown resource types never index outside those five slots.
This implements data display, not the missing classes' spell/combat mechanics.

The original supplied UI-TargetingFrame, UI-StatusBar and skull art are included
in the private UI atlas. The shared frame uses the original FrameXML 232x100 crop;
the player copy is reversed horizontally. Status bars clip the source gradient
as their fill changes. Fonts and every original front-end sprite remain in the
same 512x512 atlas. Player and target are visible during ordinary gameplay,
including action layers, and hidden for modal screens and diagnostics.

XP progress is currently under the player frame. Resource labels and counters
remain readable at 640x480. These positions are an intermediate layout: the full
original main action/menu bar and experience-bar placement still require work.
The later portrait candidate fills these circles with the real resident player
and creature models; see [PORTRAITS.md](PORTRAITS.md) for its separate offline GPU
fixture and remaining live-world gates. This is not full player/target-frame
parity. Faction/reaction/difficulty/elite presentation, buffs/debuffs,
target-of-target, other-player name queries and all
original interaction/tooltips remain open. Existing target selection currently
covers creatures; this batch does not add friendly/party/player targeting.

## Formats and allocation

WXU1 version 2 adds three sprites (10 total). Its header magic and dimensions stay
unchanged; the loader accepts exactly seven sprites for version 1 and ten for
version 2. Legacy atlases retain the previous text HUD. No larger atlas or quad
pool is allocated: graphics storage is still 1,376,256 bytes (512-square texture
plus 2,048-quad vertex buffer). The new HUD snapshots and sprite descriptors are
small fixed CPU state, with no frame heap allocation. The 8 MiB allocation guards
are retained. Native headroom and CPU/GPU cost for this candidate remain unmeasured.

WXTV is a 1,288-byte telemetry packet. Seventeen words after WXTU describe HUD
submission count, player health/max/resource type/current/max, target GUID and
health/max/resource type/current/max, XP/next XP and target presence. The recorder
zero-fills these fields for older packet versions. `tools/check-hud.py` requires
new submitted data, compares player health/XP to the world snapshot, checks fixed
UI allocation and >=8 MiB measured free memory. `--targets` additionally requires
two targets followed by a cleared target. It does not validate screenshots.

## Validation and evidence

- 995 host checks cover real Vanilla UPDATE_VALUES resource changes through the
  production packet parser, unknown/zero values, death/ghost, clearing, bounded
  draw commands and the actual prepared atlas. The sample scene emits 117 quads
  with unchanged 1,376,256-byte graphics storage. Host allocation accounting is
  not a measured console free-memory result.
- 182 UI format/draw checks cover strict metadata, allocation failures, clipping,
  mirrored sampling and ellipsized names. Both legacy/current real atlas reads pass.
- Eight synthetic telemetry tests include all historical formats and WXTV fields.
- Native XBE and XISO builds pass. The final link has only the known .edata merge
  warning; the full build also reports existing font indentation and pinned SRP
  unused-variable warnings. No xemu or physical input acceptance is claimed.
- `build/evidence/hud-host-preview.png` is a clearly labelled software rendering
  of production C UI commands with synthetic units. It helps check placement and
  readability; it is **not an xemu screenshot** and cannot prove NV2A rendering.

Private source provenance is recorded in `hud-source-provenance.json`, using the
supplied PlayerFrame/TargetFrame XML/Lua and pinned vMaNGOS 1.12.1 field map. Asset
provenance and the atlas are in `build/hud-prepared-20260916`. The older
`build/ui-prepared` and stable physical-test kit remain unchanged.

The next combined PreviewReplay adds only target cycling/clearing after returning
to the saved world. It contains 961 records / 18,994 frames and remains below the
existing format bounds. Host character-UI replay passes; it does not simulate the
actual networked target selection or GPU draw. Prepare an isolated fixture and
matching plan as described in PLAYTEST.md. Once the shared account is available,
use one 900-second xemu capture for preview/customization/HUD acceptance, with a
fresh saved-state baseline and the actual replay-mounted receipt. Screenshots,
new native headroom/timing and later physical hardware remain release gates.
