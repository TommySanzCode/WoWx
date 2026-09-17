# Third-party code

WOWX is an experimental native Xbox adaptation of WoWee, pinned by
`config/dependencies.json`. WoWee code is covered by the upstream MIT License
with Additional Restriction (no commercial game use without permission).
Its separately licensed original music is excluded from this project.

The Xbox renderer uses patterns and math code from XboxDev/nxdk's mesh sample.
The bounded font atlas retains the bitmap from pinned nxdk's pbkit_print.c;
its MIT/SPDX notices and upstream file hash are in src/font_data.h. The font
layout preserves pbkit's 16-row grid and character spacing. No global toolchain
files are modified by the texture-based renderer.
nxdk and pbkit notices are retained in `licenses/`. GLM and StormLib are used
by the host asset preparation tools. Game assets are supplied locally by the
user and are not part of the source repository.

zlib handles bounded server update decompression. LibTomMath supplies Xbox/host multiprecision arithmetic; bundled LibTomCrypt SHA-1
sources are selected from StormLib. SDL2, lwIP, libc++, PDCLib and libusbohci are
linked through nxdk. Their collected notices are in `licenses/`; individual nxdk
source files also retain their SPDX copyright/license headers. vMaNGOS is a separate
GPL-licensed local server process, with its source pin and license retained.

The world, movement and character adapters follow the Vanilla wire layouts in
WoWee's Classic packet parsers, checked against the pinned vMaNGOS server.
WoWee `Packet`, `SRP`, `VanillaCrypt`, asset loaders
and animation-track sampling code are compiled directly from the pinned checkout.
Player body-atlas coordinates and component layering follow WoWee's character
renderer and appearance composer. Shared race naming, item texture paths,
geoset and helmet rules are reused from its pinned headers.

The host-only texture compressor uses stb_dxt v1.12, pinned in
`config/dependencies.json` and vendored in `third_party/stb/`. The MIT alternative
is selected; its copyright and license are retained in the header and
`licenses/stb_dxt.txt`. It is not linked into the Xbox client.


The host-only UI atlas preparer uses Pillow 11.3.0 (requirements in
config/ui-requirements.txt). Its installed distribution and bundled dependency
notices, including FreeType, are retained in licenses/Pillow.txt. The Xbox build
does not link Pillow or FreeType. Original interface fonts and artwork are decoded
from the user's local game archives into ignored private build outputs; those
assets are not included in the source repository.

The world-environment adapter checks WMO liquid-entry interpretation against
`upstream/vmangos/contrib/vmap_extractor/vmapextract/wmo.cpp` (CMaNGOS Project,
GPL-2.0-or-later; upstream notices retained). It uses the pinned WoWee group
transforms and liquid parsing, with strict original-file boundary validation.

The terrain-liquid adapter also checks original MCLQ layouts and deep-water flags
against `upstream/vmangos/contrib/extractor/loadlib/adt.h` and
`upstream/vmangos/contrib/extractor/System.cpp`. Their upstream GPL notices remain
in that pinned checkout. WoWee supplies the ADT identities and terrain coordinates;
the adapter preserves original enabled-cell heights and masks with strict bounds.
