# Shared Vanilla character appearance

Development candidate; native screenshots, timing, memory and physical controller
acceptance are still required. This does not establish full character/UI parity.

## Data and behavior

Creation's Y button opens Skin, Face, Hair style, Hair color and Facial feature.
D-pad changes a choice; A/B returns to the creation form. Right stick retains
preview rotation/zoom. Changes populate the actual Vanilla creation packet.
Changing race/sex resets appearance to that identity's default. Skin changes
repair an unavailable face; hairstyle changes repair an unavailable hair color;
color changes repair unavailable facial detail. IDs are read from the supplied
tables, including sparse IDs, rather than guessed ranges.

The same runtime path handles a server-enumerated or in-world player appearance.
Race/sex selects one shared geometry/animation/equipment pack. Skin, face, hair,
color and facial-feature changes reuse that open pack and select/composite layers
and geosets. Clothing remains composed on top of those layers. The previous
appearance is never displayed as if it matched an unavailable identity.

Primary local rules are the pinned vMaNGOS `Player::ValidateAppearance`,
`GetCharSectionEntry` and `SECTION_FLAG_UNAVAILABLE`, plus WoWee's existing
`assemblePlayer`/geoset conventions. The [Vanilla DBC schema](https://github.com/wowdev/WoWDBDefs/blob/master/definitions/CharSections.dbd)
also distinguishes this layout from later expansions. The
[build-5875 component research](https://github.com/samwhosung/wow-1121-client-internals/blob/main/docs/character-model.md)
documents the base/overlay split and the five customization categories; it is
supporting evidence, not proof that this renderer reproduces every PC pixel.

- CharSections fields: race/sex/kind/variation/color at 1..5, three paths at
  6..8 and flags at 9. Flag 1 means unavailable for player creation in pinned
  vMaNGOS. Later-client flags must not be substituted.
- CharacterFacialHairStyles has no ID column. Geoset channels use columns
  6, 8, 7 for groups 100, 200, 300, matching the existing assembler.
- A missing CharHairGeosets mapping uses bald/scalp geoset 1. Orc female style 7
  is one shipped texture-only example; it must not be removed from the choices.
- Tauren and most female races are exempt from the server's facial-texture-row
  check, but still require a facial-style mapping. Night Elf/Undead females
  require the facial-texture row as well.
- Some facial-color rows cannot be combined with any hair-color row. They remain
  stored; the independent DBC audit distinguishes these unreachable rows from
  missing coverage of a valid player choice.
- Female Tauren has no M2 type-6 slot. Her table's missing type-6 texture paths
  are unused, not replaced with fabricated textures. Existing optional missing
  scalp/facial/underwear overlays are logged. Required, used textures still fail
  conversion when absent. Exact PC behavior for all optional overlays remains a
  visual acceptance item.

## Private asset formats and bounds

Each race/sex has `A<seven appearance bytes>.WXP`, its `.WXA` companion, and
`L<race><sex>.WXL`. Normal bundles prepare the sixteen zero-look WXP/WXA pairs;
other looks use the same pair. WXP v5 is unchanged. Its texture deduplication now
keeps body, hair and extra-skin bindings distinct even if initial pixels match.

WXA v2 retains the 80-byte original header and adds an eight-byte hair/extra
texture-binding record before its item table. WXA v1 exact profiles are still
readable. Version 2 requires its WXL companion. File identity, table locations,
binding dimensions/flags and payload spans are checked before rendering.

WXL v1 has a 32-byte header and up to 512 forty-byte records. Each record stores
kind/variation/color, three payload offsets/region codes and three geoset values.
The fixed index is 20 KiB; only one catalog per resident avatar is open. Payloads
are deduplicated and read on demand from files capped at 64 MiB, with short FATX
filenames. No full appearance combinations are baked into separate models.

Body composition keeps the existing 128-square canvas, 8 KiB overlay scratch and
body/cape mip buffers. Hair/extra bindings add at most two 87,380-byte GPU buffers.
The 4 MiB animation cache and >=8 MiB free-memory allocation guard remain. Native
resident/frame measurements are pending; host allocation accounting is not an
Xbox headroom measurement. Composition currently happens on an appearance/item
change, not every frame; its new worst-case I/O/frame cost still needs profiling.

## Validation and remaining work

`tools/check-look-catalog.py` compares every prepared key and geoset directly to
the supplied raw DBCs. `--references` additionally audits valid section coverage.
`wowx_assetc --look-reference <Data> <prepared-dir> <race> <sex>` creates private
reference hashes by composing the original full 256-square atlas, then reducing
it to the target pixel positions. `wowx_look_runtime_tests` compares those hashes
against the native C compositor's actual body/hair/extra buffers and checks pack
reuse, resource release, GPU allocation failures and the configured cache bound.
This reference uses the existing WoWee-based cooker, not a screenshot of the PC
client; matching it does not prove exact PC compositing/lighting/material parity.

Native acceptance must cover the five controls across all sixteen identities,
roster/world re-entry with a nondefault disposable character, equipment on changed
looks, missing/corrupt layer files, repeated switching, screenshots and >=8 MiB
measured free memory. Preserve existing saved characters and the hardware-test
package. Exact racial captions, PC layout/randomization, all equipment displays,
all animations, attachments, transformed forms and visual fidelity remain open.

## Prepared choice matrix (host verified)

Counts for faces and hair colors use skin/style zero; dependent lookup stays active.
References cover reachable section rows, not the full Cartesian product of looks.

| Race / sex | Skins | Faces (skin 0) | Hair styles | Colors (style 0) | Facial features | Reference looks |
|---|---:|---:|---:|---:|---:|---:|
| Human / male | 10 | 12 | 12 | 10 | 9 | 319 |
| Human / female | 10 | 15 | 19 | 10 | 7 | 345 |
| Orc / male | 9 | 9 | 7 | 8 | 11 | 216 |
| Orc / female | 9 | 9 | 8 | 8 | 7 | 150 |
| Dwarf / male | 9 | 10 | 11 | 10 | 11 | 299 |
| Dwarf / female | 9 | 10 | 14 | 10 | 6 | 234 |
| Night Elf / male | 9 | 9 | 7 | 8 | 6 | 176 |
| Night Elf / female | 9 | 9 | 7 | 8 | 10 | 208 |
| Undead / male | 6 | 10 | 10 | 10 | 17 | 319 |
| Undead / female | 6 | 10 | 10 | 10 | 8 | 229 |
| Tauren / male | 19 | 5 | 8 | 3 | 7 | 124 |
| Tauren / female | 11 | 4 | 7 | 3 | 5 | 68 |
| Gnome / male | 5 | 7 | 7 | 9 | 8 | 160 |
| Gnome / female | 5 | 7 | 7 | 9 | 7 | 103 |
| Troll / male | 6 | 5 | 6 | 10 | 11 | 189 |
| Troll / female | 6 | 6 | 5 | 10 | 6 | 90 |

All sixteen catalogs total 49,841,432 bytes: 3,603 section rows and 280 geoset
rows. Sixty-two unavailable source rows are excluded from player selection;
32 unreachable section keys remain recorded and explicitly audited.


## Randomize and combined acceptance candidate

X on the appearance screen now chooses a valid appearance from the loaded WXL
catalog. It retains the name, race, class and sex. Skin/face and hair/color/facial
combinations are sampled from existing records, including sparse IDs; unavailable
and unreachable combinations cannot be produced. If sampling repeats the current
look, a valid alternate choice is used when available. There are no random retry
loops, added persistent caches or heap allocations. Work is bounded by the
512-row catalog and fixed 256-byte choice/color tables. This is cosmetic PRNG
state seeded from the UI clock, unrelated to authentication entropy.

The combined PreviewReplay visits all five selectors, then Randomize, for each
race/sex after its starter outfits. It returns through the existing form and
keyboard cancellation flow to the original saved character. Native acceptance
must check the rendered look and successful composition counter, rather than
inferring completion from a changed UI draft. WXTU reports those values and a
hash of the composed full-resolution CPU atlas before mip generation; hashing
runs only when appearance/equipment changes. This adds no GPU allocation.

Host evidence in build/evidence/customization-* covers catalog-valid randomized
sequences for all sixteen identities, preserved draft identity/name, the combined
952-record trace and parser/checker rejection of stale/missing evidence. The
actual Human male compositor verifies the new CPU hash against the unswizzled prepared texture buffer
on the host (no GPU execution) for 319 existing reference appearances. These checks do not
replace native timing, memory, screenshots or physical controller acceptance.
Synchronous appearance reads/composition and their frame-time spikes still need
native measurement. Full PC layout, racial labels, portrait/framing and the
remaining parity matrix are still open.
