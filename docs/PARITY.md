# Vanilla 1.12.1 parity target

The target is the functionality, interface and content of the PC build 5875,
with the agreed controller navigation and stock-Xbox rendering/performance
adaptations. A demonstration of one race, class, quest or location does not
complete a category. Missing functionality must not silently become a cut.

## Acceptance rules

Geometry submission/combined integration (September17): two moving world/NPC/
player/appearance cycles pass with28.89MiB free,33/34/34/36ms guest frames and
57.14% fewer index submissions for identical drawn geometry. Staged selection,
index publication and all shared budgets now pass together in this workload.
Actual images retain explicit-relocation loading gaps and ground-contact issues
in low animation poses. No physical input, gameplay, full-world visual or release
gate is closed. See DRAW-SUBMISSION.md. Delivered hardware kits remain frozen.

Staged spatial selection (September17): 2,048-entry frame quota and atomic
candidate publication pass host and native checks. Three offline terrain-water
cycles retain 29.23 MiB free; maximum guest frame interval is 48 ms. Selection is
spread over frames; drawing still spikes to 43 ms. This adds bounded streaming
evidence, not movement, class/gameplay, original UI, physical input or hardware
acceptance. All three delivered hardware kits remain unchanged. See SELECTION.md.

September 17 revision 2 hardware delivery packages the exact accepted staged-index
executable with the existing 24-region merged assets. File hashes, host loader,
Northshire floor, ZIP integrity and current password verifier pass. This adds
packaging evidence only; combined gameplay, physical controls and stock-hardware
gates stay open. All three delivered kits remain reserved and preserved.
See [revision 2 instructions](HARDWARE-20260917-R2.md).

Staged index publication (September17): three offline terrain-water cycles pass
with 29.27 MiB minimum free. Copies are bounded to 256 KiB / 2 ms in this capture;
atomic publication is 0–1 ms. The 56 ms maximum still misses the frame target:
selection and drawing remain blocking. No physical controller, full-game or
additional gameplay/UI coverage is inferred. See INDEX-PUBLICATION.md.

Streaming clock candidate (September17): two offline terrain-water cycles retain
30.02 MiB minimum free and exercise1,432 cooperative world yields. The81 ms
maximum still misses the frame target; index publication, selection and drawing
remain synchronous. No extra gameplay/physical hardware coverage is inferred.
See STREAM-CLOCK.md. Both hardware packages stay frozen for the user's test.

September 17 hardware handoff: the latest executable and merged 24-region asset
set are packaged as `dist/WOWX-hardware-20260917`, with manual login/input and no
bundled password. File hashes, all world packs through the host loader, a saved
Northshire collision floor and authentication-only login pass. The earlier kit
is preserved. This adds packaging evidence, not physical controller, hardware,
combined live gameplay or additional feature-parity acceptance. See
[hardware instructions](HARDWARE-20260917.md).

- Use the supplied Vanilla data and pinned WoWee implementations wherever practical.
- Preserve authentic interface artwork, typography, information and workflows;
  adapt focus/navigation and readability for 640x480 and an Xbox controller.
- Bound CPU/GPU/audio allocations and queues. Maintain at least 8 MiB measured
  free memory on a 64 MiB console configuration, including loading transitions.
- Reusable conversion/streaming handles map data; zones are not separate ports.
- Record protocol fixtures, native replay, actual screenshots, persistence and
  measured timing as separate evidence. Physical controllers and original
  hardware require their own validation.
- The passing 61-minute walking/menu/reconnect run is one scoped stability result.
  It does not satisfy the whole-game performance or release gates.

## Coverage matrix

| System | Current coverage | Required for parity |
|---|---|---|
| Original UI fonts/artwork | Fixed private atlas/bounded renderer and three character screens verified in xemu; candidate adds HUD sprites in the same allocation with legacy format support | All visible screens, localization glyphs, correct styles/anchors/clipping |
| Login/title screen | Account/password/server keyboard, errors/retry/cancel verified; original title mesh,28 emitters,4 authored lights and world transition verified | Advanced effect fidelity/fog, full PC layout/settings/preferences and DNS |
| Realm selection | All 255 entries parsed, status/type/population, controller selection/cancel; native flow verified in xemu | Full PC list filtering/categories/preferences and adverse multi-realm tests |
| Character roster | Controller list/entry/refresh and equipped previews verified in xemu; candidate adds typed deletion confirmation, server deletion/refresh and all seven Vanilla login-error recovery codes with encrypted host fixtures | Native deletion/cancellation/error acceptance using a disposable account, exact layout/framing, complete error states |
| Character creation | Race/class/sex/name/server creation; 16 default previews verified in xemu; candidate adds data-driven skin/face/hair/color/facial controls and Randomize, 3,229 reference appearances, 80 starter outfits and all racial backgrounds checked on host | Native customization/outfit/background/loading acceptance and nondefault saved creation; racial captions, exact layout and complete errors |
| In-game menu/settings | Controller bindings and dead zones | Original menu structure plus functional Xbox graphics/audio/input settings |
| Player/target HUD | Candidate adds original frame/bar artwork, authoritative health/resources/level/name, XP, death/ghost, target clearing and resident-model player/creature portraits; isolated GPU fixture covers 16 player models and 92 creature displays | Live-world HUD/portrait acceptance; exact PC framing/layout, non-idle/death portraits, class/faction/difficulty, buffs/debuffs, target-of-target, player/group targeting |
| Action bars | 24 trigger bindings, warrior stance slots, server persistence; candidate adds original spell/item icons with bounded streaming and all three layers checked in an isolated native GPU fixture | Live-world icon/stance/item synchronization; original complete bar layout, tooltips, complete cooldown rules (below), range/usability, all class/form/pet bars |
| Cooldowns | Base candidate restores server spell/item/category timers and lockouts with radial feedback. Extension adds cast-start GCD, preparation cancellation, haste/ranged timing, zero-duration GCD hints and family-matched signed modifier totals; see GLOBAL-COOLDOWNS.md for scoped evidence | Overlapping aura-mask/charged modifier resolution, local latency prediction, all-class live synchronization, pet/control contexts and physical input/hardware acceptance |
| Spellbook | Learned names/ranks/passives and assignment | Original layout/icons/descriptions and all relevant spell categories |
| Talents | Missing | Every class/tree, requirements, ranks, trainer reset and persistence |
| Bags/equipment | Equip/use/unequip and inventory updates | Original slot layouts/icons, tooltips, splitting, moving, destruction confirmation |
| Quests | Journal, dialogue, objectives, accept/reward; two-quest native loop | All objective/reward types, sharing, abandon, tracking and full quest UI |
| Vendors | Bundle buy, single sell, saved reconnect | Quantity choice, repairs, buyback, costs and original layout |
| Trainers | Missing | Requirements/costs, learning, talent reset, class/profession coverage |
| Banks/mail/auction | Missing | All ordinary operations and authoritative updates/errors |
| Trade/loot groups | Solo loot demonstrated | Trade confirmation, group loot/rolls, permissions and all loot types |
| Chat/social/guild | Missing | Channels, whispers, emotes, friends/ignore, guild operations and readable UI |
| Parties/raids | Missing | Invite/leave/roles, unit frames, permissions, loot and group interactions |
| World map | Zone/continent/exploration/player/corpse, zoom/pan tested | Original presentation, party markers and all applicable map contexts |
| Minimap | Missing | Terrain/world model imagery, heading, POIs, tracking, party/quest indicators |
| Combat | Warrior melee/Heroic Strike; real spell metadata, advisory resources/range/forms and item counts/charges candidate (12 offline native phases) | Combined live acceptance, all nine classes, exact skill/aura scaling, full targeting and combat feedback |
| Spell targeting | Unit targets; cast/channel lifecycle, pushback, interruption and B cancellation candidate verified with encrypted host and synthetic native tests | All-class live synchronization, ground/area/friendly/self rules and native command acceptance |
| Auras/forms/pets | Limited warrior stance action mapping | Buff/debuff lifetimes, transformations, pet control and all class mechanics |
| Death | Spirit Healer and separate corpse-run replay pass | All release/reclaim/resurrection paths, visuals and combined gameplay stress |
| Professions/skills | Missing | Learning, gathering, recipes, crafting, skill progression and complete UI |
| Travel | Local walking/tile crossing and hearthstone tested | Mounts, taxis, boats/zeppelins, portals, transports and all transfers |
| World coverage | Four prepared terrain tiles plus 20 verified global WMO packs; map 450 fails on a referenced NaN UV in its source WMO. Six global maps pass isolated native routing/render/floor checks (GLOBAL-WORLD.md, MATERIAL-MOTION.md); terrain streamer passes two boundary/Abbey/camp cycles | Both continents, all applicable instances/battlegrounds and seamless traversal; full live integration and documented repair/clean-source comparison for map 450 |
| World materials | Seven original blend modes and world-doodad material motion; WXP v8 adds original WMO RGB lighting, indoor baked-light state and authored clamping. Twenty packs retain exact geometry/pixels/motion; six original maps plus gradient chart pass one native run, 34.84 MiB free (VERTEX-LIGHTING.md) | Original colour-alpha fixup/pixel parity, actor material clips, UV rotation/scale, multi-texture shaders, local/doodad lighting, face culling, intersecting/cross-actor transparency and live integration. UV translation has synthetic coverage only. Tram water/bright cards and unexplained cold draw stalls remain; physical hardware unverified |
| Collision/movement | Terrain/building walking, camera and jumps | Swimming, falling, slopes, doors/transports and indoor/outdoor transitions; global-map floor probes do not establish navigable instance routes |
| Character appearance | All sixteen default race/sex profiles pass cancellable native loading, rapid selection and missing-profile recovery (PROFILE-LOADING.md); all 3,229 prepared appearance references and 80 starter outfits checked on host. Staged atomic publication passes 26 synthetic Human changes and 50 cancellations during two native route cycles with no player gaps (STAGED-APPEARANCE.md) | Combined native controller customization/outfits and live equipment persistence; exact PC visuals, all item displays, animations, attachments and transformed models |
| NPC/object visuals | Local supported templates/buildings/props; bounded payloads, complete-pose retention, NPC idle/run coalescing, distance cadence and shared palettes pass native synthetic traversal with eight NPCs and an equipped Human, four clips | Full model catalogue, object states, animation, equipment and interaction feedback; non-looping per-instance playback and ground contact (low poses intersect terrain in the latest offline images); live combined acceptance |
| Water/weather/lighting | Original terrain MCLQ now feeds the shared liquid renderer: 115 grids / 3,835 cells in two tiles, all visible heights/masks exact; ten offline native camera cases pass, 30.02 MiB free, 33/34/34/69 ms guest intervals (TERRAIN-LIQUIDS.md). Lake visible; river/shore appearance and swimming unverified. WXP v10 adds original WMO water/magma surfaces and all 30 original texture frames; 386 patches across 20 packs; 14 native camera cases pass with 29.59 MiB free and 33/34/34/55 ms guest frame distribution (LIQUIDS.md). Earlier diagnostic glyph-loss report was not corroborated on image reread; full original UI acceptance remains open. WXP v9 source-derived WMO camera interior/water classification; 721 group volumes/69 liquid grids across 20 packs; twelve offline native camera cases pass, 30.91 MiB free (ENVIRONMENTS.md). Authored fog, ambient/diffuse and camera-relative sky colours; 374 volumes/26 maps, 440.5 KiB catalog; server clock and local/time blending. Exact Vanilla weather state, grade-based clear/overcast blending, smooth/instant changes and transfer/logout reset pass 12 encrypted localhost scenarios and ten native fixture phases (FOG.md, WEATHER.md) | Exact portal/room containment, local WMO fog, terrain liquid cave/ground containment and submerged-root classification; swimming and all-terrain visual coverage; exact WMO depth opacity, flow UV, waves/reflection/refraction; precipitation, weather audio, full sky/clouds/stars/sun/moon, water effects and exact materials; all-map/live-avatar integration and hardware. Native conditions are synthetic; no live shared-account weather acceptance |
| Spell/particle effects | Native title emitters; candidate adds local particles, inherited size and XY quads for remaining preview scenes; host checked | Native acceptance, precise motion/sprite timing, lighting/fog/sorting and bounded world effects with visibility/priority controls |
| Audio/music | Missing | Positional effects, ambience, UI/voice/music streaming and volume controls |
| Instances/PvP | Global WMO preparation and scoped native map routing/collision/rendering for six maps; instance gameplay and PvP remain missing | Entry, difficulty rules, encounters, battlegrounds and player combat |
| Disconnect recovery | Saved logout/reconnect tested | Forced network loss, errors/timeouts, reconnect with authoritative recovery |
| Addon/UI scripting | Third-party addons explicitly deferred; native core UI first | Original workflows remain required; addons are a later milestone |
| Performance | Latest geometry/selection/clock/index integration: two combined synthetic route cycles, 33/34/34/36 ms and 29,588 KiB minimum free; 57.14% fewer index submissions (DRAW-SUBMISSION.md). Earlier shared streaming/appearance: 33/34/36/50 ms, 32,136 KiB minimum free. Earlier matched route maximum fell 72 to 49 ms (STREAMING.md). Separate staged-profile fixture: 33/34/34/35 ms, 41,660 KiB minimum free; not a world performance result | Remaining work/GPU stalls, appearance planning, synchronous world startup and other UI loaders, full live combat/crowded workloads, all-map streaming and smooth target workloads on physical stock hardware |
| Release | Rebuildable private development images | Full compatibility matrix, repeatable mixed endurance, installation and limits |

## Current implementation order

1. Complete the shared original-art/font renderer and verify character screens.
2. Build the account/title/realm/character front end on that renderer.
3. Bring the in-world HUD and menus onto the same data-driven UI foundation,
   implementing their missing operations rather than adding decorative controls.
4. Extend gameplay/protocol coverage and general world/asset streaming together,
   followed by audio/effects and mixed-system endurance.

This is an implementation tracker, not a completed-feature claim or a promise
that full parity can be achieved within one day. No category is waived by this order.

## Development and testing cadence — user direction, September 16

Prioritize substantial implementation batches. The user explicitly asked for
more work between test runs because repeated validation was slowing progress.
This supersedes earlier instructions to repeat login, neutral-world and restored-
menu captures at every small checkpoint.

- Implement a coherent group of related changes before full native validation.
  Current candidates cover racial scenes, starter outfits, staged loading and
  character management, shared appearance customization and Randomize. A combined
  controller replay now covers all 80 outfits and 80 appearance changes on host. Native acceptance,
  remaining front-end details and the original in-world HUD are the next gaps;
  continue independent implementation
  while the shared account is reserved for the user's hardware test.
- Compile during development. Use quick, targeted host checks when changing
  packet parsing, asset boundaries, allocation or other failure-prone logic.
  Do not rerun broad suites after every visual adjustment.
- Use brief xemu inspections when they resolve a specific rendering or runtime
  uncertainty. Run one combined acceptance scenario for each meaningful batch,
  covering its affected UI, memory lifetime and gameplay transitions.
- Repeat or broaden tests only for an actual failure, a subsequent relevant
  change, or an unresolved concern. Do not automatically chain separate login,
  neutral-world and final-menu recordings when one run covers the change.
- Reserve endurance captures for substantial streaming/memory/concurrency
  changes and release milestones. Keep the existing known-working checkpoints;
  create new source/binary archives at meaningful milestones.
- Preserve current recordings and their mounted images until they finish. Mark
  unvalidated work clearly; retain the stock-64-MiB, 8-MiB-headroom and eventual
  full-functionality/hardware acceptance requirements.
- Progress reports should emphasize new working functionality and remaining
  blockers. Routine test counts are supporting evidence, not the main result.

The approved milestone order and release gates are in [ROADMAP.md](ROADMAP.md).
Action/cast batch evidence and limits are in the latest STATUS.md entry.


### September 17 staged appearance checkpoint

- Equipment/skin/face/hair texture work spans bounded updates. Published appearance
  stays complete until textures and required families can switch together.
- Host: 3,229 original appearance references, all 16 profiles, 80 starter outfits,
  112 full mip chains, malformed/cancellation/atomic-publication/memory checks.
- Native: 26 captured Human changes, 50 cancellations, two offline route cycles,
  no player gaps/failures, 31.38 MiB free, 33/34/36/50 ms guest frame distributions.
- Full companion identity coverage is enforced. Synthetic state and pixel probes
  do not satisfy frontend input, all-race native, live gameplay or hardware gates.
- Profile opening and initial atlas setup remain synchronous. Stage these next;
  see STAGED-APPEARANCE.md. Full Vanilla scope and release gates remain open.
