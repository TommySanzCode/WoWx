# Current architecture

## Ownership and data flow

```text
Supplied MPQs -> desktop assetc + WoWee parsers -> tile packs/index + actors.wxp
                                                  |
                                    main thread: bounded scene cache
                                                  |
                              terrain/collision + skeletal skinning
                                                  |
                                        nxdk/pbkit -> NV2A

SDL controller -> pure input mapping -> main thread movement/menu
                                           |
                                  fixed movement + command mailboxes
                                           |
                      network worker -> nxdk sockets -> local vMaNGOS
```

The main thread owns SDL, the pack file, scene allocation, CPU skinning and GPU
submission. It waits for outstanding GPU work before modifying or releasing
resident vertex/texture data. The network thread owns SRP, encrypted world sockets
and character-session state. Its 32-record queue coalesces position updates while
preserving movement mode changes. A full queue fails the session visibly.

`wx_world.h`, `wx_character.h`, `wx_input.h`, `wx_pack.h` and `wx_runtime.h` expose
plain data without Vulkan types. WoWee's desktop Entity/UI/renderer coupling has
not been removed globally; those systems are not yet part of this native build.

## Pack and memory

The controller spellbook has a separate `SPELLS.WXS` catalog. Its sorted 16-bit
ID index is resident (44,714 bytes for the supplied 22,357 rows); fixed 108-byte
metadata records stay on disc. At most one record is loaded per frame into the
512-entry learned-spell cache (57,344 bytes). File length, index order, record
bounds and string termination are validated. Loading a catalog reserves 8 MiB
physical headroom plus an allowance; the cache is included in actual free RAM.
An additional 1,024-byte row index sorts active abilities before passives by name.
Vanilla's hidden and trade-spell flags exclude internal/recipe entries from the
top-level book. Profession recipe sublists remain to be implemented.

`CMSG_SET_ACTION_BUTTON` sends a byte slot and a 32-bit packed action. This UI
edits learned spells or clears a slot, and checks passive flags from Vanilla DBC
metadata. It resolves the current controller mapping/stance before confirmation
and rejects a stale confirmation if the spell, form or action changed. Local
state updates only after the network write succeeds. Vanilla provides no success
acknowledgement for this edit; reconnect supplies the authoritative action list.
This does not implement every class's bonus bars, spell targeting or cooldown UI.

WXP v5 has a 32-byte header and 64-byte entry records; the loader also accepts
uncompressed v4 packs. Offsets and lengths are
validated before allocations. Vertices are 32 bytes; triangle indices are 16 bits.
Texture entries record dimensions and mip count in `reserved[1]`; animated entries
reference their animation descriptor through `reserved[0]`. Collision entries use
original collision geometry and are excluded from drawing.

World textures may use NV2A DXT1 (opaque) or DXT5 (alpha) blocks, with complete
mip chains and at least one block for each 1x1/2x2 tail level. Blocks are stored
in row order at each level. Compressed flags cannot combine with swizzled RGBA,
animation, character/collision kinds or a v4 header. Texture sharing includes
the encoding in its key. The GPU consumes compressed bytes directly, so their
smaller size reduces both pack storage and resident memory.

`wowx_packopt` performs this optional host conversion using pinned stb_dxt.
`scripts/compress-world.py` preserves the source directory, verifies unchanged
geometry/placement metadata and full payloads, validates through the runtime
loader and publishes a revised index and SHA256 receipts. Existing output is
reused only with matching source, optimizer, verifier and output hashes.
Character packs are explicitly rejected because companion atlas metadata uses
their byte offsets; avatar clothing composition still needs writable RGBA pixels.
This is lossy color compression. Mesh instancing, cross-pack sharing, audio and
full-world storage budgets remain separate work.

Visual and collision selection have separate quotas: 256 and 64 entries. Actual
residents share one fixed array. At most three new entries load per frame, with
collision serviced first. Identical source-file/texture-offset/dimensions/layout share one
GPU allocation, released after the last resident reference. Partial failures undo
references and allocations. Scene data is capped at 32 MiB and every load reserves
at least 8 MiB measured physical headroom plus a 64 KiB allowance.

A separate 8 MiB actor cache shares templates across up to 32 nearby supported
NPCs. Only visible active clips are CPU-skinned. Identical pose ranges share one
reference-counted allocation. Prior clips survive until all replacement batches load,
and rendering uses a complete resident clip during the transition. Per-actor
animation/blending remains.

`world.wxi` contains at most 8,192 map/tile records, each 32 bytes. The runtime opens
at most four neighboring packs, one per frame, and merges their indexes while
retaining the existing global resident quotas and budget. Each pack has at most
32,768 entries (2 MiB metadata). Opening/merging checks measured free memory.
Selection refreshes after two units of movement. Terrain comes from generic ADT
coordinates; zone names do not drive loading. File names are validated 8.3 names.
The optional `WX_PLACEMENT_ID` flag preserves the ADT placement ID in `reserved[0]`
and local batch number in `id`; this suppresses duplicate building/doodad batches
across tile boundaries. Animated entries cannot also carry that flag.
Metadata is separate from the resident byte counter and included in measured
physical headroom. Current pack opening still performs synchronous metadata I/O;
boundary hitching and more demanding multi-tile scenes require native measurement.

This bounds the prototype's working set, not its future full-game requirements.
The index and fixed metadata live outside the scene-data byte counter but inside
physical free-memory measurements. Network packets are bounded by their 16-bit
framing. Some reused C++ allocations still need explicit failure recovery.

The GPU push buffer is 1 MiB. Drawing drains and resets it at a conservative
512 KiB threshold; debug text receives a separately drained buffer. This avoids
appending a large rectangle-based HUD to an already dense scene command stream.
Native screenshots are used to check legibility after this change.

## Network scope

SRP and Vanilla header encryption are reused from WoWee. LibTomMath and bundled
LibTomCrypt SHA-1 replace OpenSSL on Xbox. Random bytes come from a development
pool generated on the PC; consumption is durably recorded before use.
The auth adapter retains WoWee's SRP arithmetic, but computes proof hashes using
the selected vMaNGOS revision's natural little-endian BigNumber widths. Wire
fields remain padded. World proof and header encryption use that same natural
session-key width. Deterministic host fixtures cover ending-zero keys and proofs,
salts and public values. Native RNG remains fresh; fixture randomness exists only
in the host probe for the fixed public FIXTURE/DISPOSABLE identity.

The world adapter validates authentication, a fixed-capacity Vanilla character
list, world entry, ground/jump movement, ping/pong and logout. The development default enters
`Xboxer`; the controller character roster also supports selection and creation. Object updates
populate a 512-record store. Each record retains the first 192 fields plus player quest log, XP, money and
inventory GUID fields. A compact snapshot follows equipment/backpack and four
container bags. Item names use a bounded cache.
Updates are parsed into scratch state and committed only after full validation.
Decompression uses a 1 MiB output limit and a 128 KiB allocator arena. Up to 128
unit/player/game-object/corpse records are published to the main thread. Self,
selected target and owned corpse precede the nearest sampled objects; distance
uses the current accepted movement origin, with deterministic GUID tie-breaking.
The self/corpse records survive missing movement fields. These
drive nearby supported creature models and targeting. A 16-command queue connects
controller interactions to validated Vanilla combat, loot, quest and item commands.
Merchant stock is bounded to the pinned server's 128 items. Buying sends the
Vanilla bundle count; selling sends one item with its full GUID. Controller
confirmation freezes the selected item and price, then revalidates it before
sending. The server does not send a successful sale response, so a pending sale
completes after authoritative inventory quantity decreases by one and money
increases. Failures or ten-second timeouts clear pending state. Repairs, buyback,
quantity selection and sell-price previews remain.
Dialogue parsing commits complete packets transactionally. Known spells and
action slots support spell/item use; warrior stance bars map to Vanilla slots
72/84/96. Full class bars, cooldown presentation and spell effects remain.
Observer teleports relocate the indicated entity and clear its stale movement path.
Same-map controller teleports validate packed GUID/counter/Vanilla movement data,
publish the destination, discard stale queued input and acknowledge with a raw GUID,
counter and server movement time. A separate position revision rejects movement
submitted by a frame that has not consumed the new position. World transfers
validate the pending/new-world packets, clear old entities and queues, publish
the new location, and acknowledge world entry. Missing terrain prevents walking;
other-map native coverage remains incomplete. Manual release, Spirit Healer
resurrection and corpse-run recovery pass separate native scenarios. The corpse
test includes a ghost reconnect and an alive reconnect after reclaiming.
The corpse controller tracks alive/dead/ghost state, a wrap-safe server delay,
and full three-dimensional reclaim distance. It requests corpse coordinates on
ghost entry and reconnect, with at most three automatic queries per entry.
Only a corpse with the current player's 64-bit owner GUID enables reclaim;
query coordinates alone can guide the player but cannot authorize the request.
The map reuses its existing texture and draws a red body cross without another
allocation. Native telemetry WXTM appends 18 corpse/map fields (984 bytes total);
WXTN adds four sustained-test counters (1,000 bytes total). Older formats remain
readable by the host recorder. A separate 3D corpse body and ghost visual effects
remain outside the verified rendering coverage.
Development failure telemetry captures a bounded prefix of the rejected world
packet, without authentication payloads, for regression fixtures.

The server remains a separate GPL-licensed process. Its database/assets do not
ship in the source tree. vMaNGOS's collision/pathfinding data is separate from the
Xbox's lightweight runtime collision representation.

## Player preparation and cold world entry

Player appearance preparation shares the humanoid geoset/helmet selection used
by NPCs and WoWee's race/component naming rules. A 256-square CPU atlas composites
skin, face, optional facial/scalp overlays, underwear and ordered equipment
regions. The normal pack writer then reduces textures to 128-square mip chains
and bakes the existing animation clips. Held sword/shield attachments use the
owner's bone. Default profiles for all sixteen Vanilla race/sex pairs, equipped
Human and alternate Human pass full runtime pack validation. Some shipped Tauren
rows name absent optional overlays; the cooker records those omissions.

The original `--player` path remains a profile preparation/reference tool;
the modular path below supplies native player rendering. The network view
retains the enumeration appearance as a fallback,
then follows live player create/value updates: PLAYER_BYTES/PLAYER_BYTES_2,
identity/display fields, flags and nineteen visible item entries. Omitted create
fields default to zero; explicit-zero deltas remove equipment. A compact array
adds 84 bytes to each bounded entity instead of retaining the whole player field
range. The common entity store and scratch copy still have fixed capacities.

Full Vanilla item queries map entries to display IDs and inventory types in the
existing 256-item name cache. The parser validates the complete 5875 layout,
including strings, fixed stats/spells, finite damage/range values and the exact
tail, before committing. Negative responses are cached. Only the first 256
distinct requested items form the working set, prioritizing worn equipment;
requests are limited to twenty per second. Unresolved equipment clears the old
display rather than reusing stale gear. An appearance revision changes only
when the published look or resolved gear changes. Telemetry version 9 exposes
the nineteen entry/display/type triples; version 10 (magic WXTA) adds an explicit
scenario ID so one passing test cannot masquerade as a different scenario.

### Modular native avatar

`--avatar` produces a WXP geometry file and bounded WXAV version-1 companion
manifest. The manifest records one exact seven-value appearance, an unpainted
128-square body atlas and up to 128 display/slot item definitions. Clothing uses
the eight Vanilla body regions, alpha/color-key composition and authored layer
order. Body batches keep their geoset identities; independently attached gear
uses component families starting at 4096. The same skeleton poses are deduplicated
across the body's and equipment's idle/walk/run/corpse/attack clips.

Native `avatar.c` validates every metadata span, component reference, geometry
binding and companion size. Live equipment selects the prepared components and
WoWee-derived mesh variants, respecting helmet/cloak hide flags. The body is
recomposed only on appearance changes, then mipmapped and swizzled on the CPU.
The scene has a 4 MiB resident budget, a maximum of 32 selected components and
two loads per frame. Its auxiliary allocations and metadata are included in
telemetry, in addition to system-wide 8 MiB free-memory enforcement. At the
tested Human's idle state the combined avatar allocation is 1,005,828 bytes.
All selected components share a complete animation clip during streaming.

WXTB telemetry adds matched/ready/draw/missing state, avatar allocation, visible
equipment slots, camera distance, selected clip and a hash of animated vertices.
This distinguishes actual geometry submission from live network appearance data.
The native gear replay verifies draw removal/restoration and reconnect. The pack
currently stages one Human appearance and five item displays; host tests prepare
all sixteen default race/sex combinations, but do not establish native coverage.
Unknown appearances hide this template, unknown equipment increments a visible
diagnostic, and transformed display IDs do not reuse an incorrect Human body.

The third-person camera traces its center and four near-plane corners against
resident terrain/WMO/M2 collision triangles, pulls forward from the closest hit,
and collapses to the focus while required collision is pending. Results are
reused until the camera or collision residency changes. This is a bounded camera
collision approximation; broad terrain, doorway and hardware validation remain.
Weapon grips/sheathing, complete appearance coverage and transformation rendering
remain implementation work.

`WxAvatarSelection` builds an `A` plus fourteen hexadecimal digit filename from
the seven live appearance bytes. It releases the old avatar after prior GPU work
drains, opens the new WXP/WXA pair and verifies the metadata look matches exactly.
It never renders a mismatched fallback. A remembered appearance/connection key
prevents retrying missing or malformed profiles every frame; a different key
allows another attempt. Only one profile uses the existing 4 MiB mesh cache.
WXTG telemetry records attempts, successful profile changes and failures.
The preparation helper creates exact profiles in a new directory, verifies them
through the runtime, and writes per-file hashes. Sixteen defaults are prepared
with the small test item catalog; arbitrary looks and complete equipment catalogs
still require preparation. No game assets enter the source repository.

The trigger HUD shares the action-slot resolver with gameplay and the spellbook.
Eight bounded `WxActionPrompt` records resolve saved controller indices through
the current supported form to server slots, packed actions and cached names.
The same records feed rendering and WXTH telemetry, allowing comparison with a
separate server action snapshot. No files or network queries are issued during
HUD drawing. Each name is bounded and strips control bytes before entering the
16-row text grid. LT/RT/both choose the layer; UI/death/diagnostic state suppresses
the panel. Cooldown/resource checks and remaining class form mappings are pending.

The utility wheel uses a pure timed input state machine. A 350 ms Black hold
opens four implemented destinations; a shorter tap toggles bags. It captures
both sticks, action mappings and button edges from the opening press through
activation/cancellation. Confirming while Black remains held latches capture
until release, preventing a later modifier/button edge from reaching the newly
opened UI or gameplay. Disconnect/unavailable-context transitions cancel without
activation. The unsigned timer subtraction tolerates the tick counter wrapping.
The renderer draws a bounded four-sector scanline ring with existing pbkit fill
commands and text. WXTI adds pending/open/selection/action/revision plus inventory
visibility; the native test checks destination use and gameplay isolation.

World entry has a separate 60-second timeout after character selection. A cold
local server took more than 30 seconds to load its first map; the previous shared
15-second handshake budget failed. The timeout stays bounded, and packet I/O
returns to the normal budget after world entry. A delayed-login fixture covers
the regression. The normal connection HUD reports both authentication and world
session status, so an authenticated session failure is visible.

## Character lobby and controller creation

The world worker owns the encrypted character session. A bounded ten-character
snapshot and a single command mailbox expose roster/select/create/delete/refresh/cancel
to the render thread. Menu Y requests an acknowledged logout and reauthentication
before opening the roster. Ordinary reconnect uses the last successfully entered
GUID within the running process. Empty live accounts open creation; the standalone
protocol probe retains its legacy disposable bootstrap for fixture compatibility.

Creation encodes the build-5875 name plus nine appearance/outfit bytes. Name and
race/class checks happen before any destination bytes are written. The server
still decides availability and realm restrictions. Replies must contain exactly
one recognized result byte; failures retain the draft and roster. Successful
creation refreshes the roster before selection. The lobby sends keepalives and
uses bounded request deadlines. Deletion is available only through its explicit
typed-confirmation flow; automated native preview/creation replays never delete
characters or reset progress.

The pure controller state machine covers the roster, creation form, 28-key name
keyboard and confirmation. The native pbkit view uses the same state. Pending
requests suppress repeated commands; Start finishes a name while that keyboard
is open. Races/classes/sex are selectable, but appearance bytes default to zero.
Account entry and default equipped previews are now implemented (see later
sections); appearance customization and full appearance coverage remain open.
Exact prepared-profile selection is automatic.
WXTC telemetry adds lobby/UI/result
state and player GUID/level. WXC9 exercises normal controller inputs from the
original character through creation, world entry and return to saved progress.
WXTD adds the effective controller connection bit; replay runs may inject that
bit, so only normal-input runs measure the adapter. The host recorder stops on a
guest clock restart. It preserves older telemetry layouts, with -1 for unknown
connection state in recordings made by the updated recorder.

## Test interpretation

The renderer explicitly disables `NV097_SET_CONTROL0_Z_PERSPECTIVE_ENABLE` at
the start of world rendering. nxdk's back-buffer target selection sets that bit
each frame, while this client's vertex shader already emits projected Z. The
extra perspective interpolation caused lower building surfaces to occlude the
floor in the tested interior. Texture perspective correction remains enabled.
Single-floor and progressively expanded building diagnostics isolated the
interaction; the complete scene renders cleanly with this state correction.
See pinned nxdk `lib/pbkit/pbkit.c` (set_draw_buffer) and
`lib/pbkit/nv_regs.h` (SET_CONTROL0). xemu's corresponding implementation is
[fragment depth interpolation](https://github.com/xemu-project/xemu/blob/master/hw/xbox/nv2a/pgraph/glsl/psh.c).
This is an observed fix for the saved camera, not broad rendering or hardware
acceptance. All temporary placement/batch filters were removed.

Host fixtures test parser boundaries, cryptography, allocation rollback, collision
math and pure input mapping. Native telemetry tests the actual XBE in xemu.
Input replay supplies samples after the SDL adapter; it does not verify USB or a
physical controller. Guest frame time does not establish physical Xbox speed.
Short Northshire sessions do not establish crowded-scene or full-world stability.


### Walking collision rejection

The floor query first tests conservative XY triangle bounds, expanded for its
existing barycentric edge tolerance. Walking tests a triangle against the swept
box enclosing all six body rays before doing ray/triangle arithmetic. These
checks retain the existing floor, slope, wall and missing-collision rules and
allocate no memory. Camera collision keeps its separate five-ray bounds check.
An 8,000-query real-pack comparison against the previous collision code checks
floor results/heights, allowed/blocked steps and blocker identity at Northshire's
spawn, Abbey, camp and the saved inn. Native timing is verified separately.


### Frame phase measurements

WXTJ adds ten guest-millisecond counters to the frame packet (206 words/824 bytes):
state/input/UI handling, movement/collision submission, world streaming, camera
setup/collision, actor/avatar streaming and animation, geometry submission,
UI preparation, text/GPU presentation, pacing wait, and total active work.
The eight work intervals are contiguous and sum exactly to total work. They
measure wall time including any waits within each phase, not pure CPU samples.
Pacing precedes the current frame's work; `frame_ms` remains the interval since
the previous frame started. Do not sum current work+pacing and equate it with
the same row's `frame_ms`. Network worker CPU and telemetry work are not isolated.
Older packets decode with zero phase counters and remain valid historical data.
New journey replays hide the large diagnostics overlay at startup; the small
test-state caption remains, and memory/timing are still recorded through telemetry.

### Bounded avatar animation reuse

The avatar records the last complete sequence and retains every selected body
and equipment component until the next sequence is fully resident. Drawing uses
one sequence across all components throughout that transition. NPC template
streaming keeps its separate behavior.

Idle and run components may remain in the existing 4 MiB avatar cache after use,
so short movement changes do not repeatedly reload them. This adds no prefetch
and keeps the two-load-per-frame limit. A third sequence drops the optional
sequence and preserves the complete drawable fallback during loading. Optional
retention is disabled below 12 MiB measured free memory; every allocation still
checks the 4 MiB scene budget and 8 MiB system headroom plus allocation margin.
Changing equipment discards unselected families, and profile changes release the
whole old profile. Allocation failure leaves a complete fallback when available.

### Bounded font renderer

The UI retains pbkit's 16x60 text grid, 8x16 glyphs, character spacing, scrolling
and print-at behavior, with a bounded 512-byte formatting buffer. A 128x128
swizzled RGBA atlas holds the retained nxdk bitmap. One fixed vertex buffer holds
at most 960 quads. Total requested GPU storage is 157,696 bytes (154 KiB), reported
in text telemetry and the cache total; measured system free memory also includes
allocation alignment. No frame-time allocation or file I/O is used.

Three passthrough vertex instructions at slot 96 render the font using the
existing texture/color pixel program. The world program restores slot 0 for 3D
draws; a compile-time size assertion prevents overlap. Text draws after the scene
drains, with depth writes/testing disabled and alpha-tested nearest sampling.
The next world pass restores depth, attributes and its texture state.

Missing memory or invalid font configuration falls back to pbkit text. For
development comparison only, `-LegacyText` writes FONT.BIN=WXFL to force that
backend; normal preparation writes NONE. Both use the same bounded grid.
The original nxdk font and license notices are retained in src/font_data.h.

WXTK telemetry is 214 words/856 bytes. Eight appended words split presentation
into scene drain, text submission, text drain and swap, then report glyph count,
backend, requested allocation bytes and initialization failures. The four timing
intervals sum to the existing presentation phase; tools/check-text.py verifies
this and the allocation/backend limits. Old packets decode with zero new fields.


### Controller map and exploration

`wx_map.h` defines a bounded catalog of at most 128 coordinate rectangles and a
single resident 512x512 BGRA texture. The desktop exporter reads Vanilla's actual
WorldMapArea (eight fields), WorldMapOverlay (17 fields) and AreaTable schemas.
It joins the 12 base BLP tiles, clips overlays to the 1002x668 coordinate rectangle,
and downsamples by two with alpha-weighted colors. Small overlay edge textures
remain transparently padded. All 51 maps contain 526 overlays in 80,498,624 bytes;
the largest individual file is 2,090,496 bytes. Assets stay outside source control.

MAPS.WMI has a 16-byte versioned header followed by 76-byte map records. Each
Znnnnnnn.WMP contains a 16-byte header, up to 128 36-byte patch records, a linear
512-square base texture and tightly packed BGRA overlay rectangles. Each patch
carries four AreaTable exploration bits. The runtime validates finite/nonempty
bounds, strings, unique IDs, dimensions, exact file length and contiguous patch
offsets before displaying an image. Invalid or missing images produce a map error.

The self entity retains all 64 Vanilla exploration words at 0x457. On map or
exploration changes, the loader swizzles one base image using a 2 KiB row buffer,
then alpha-composites only server-revealed patches into the same allocation.
There is no second full-size staging texture. Four vertices need 96 bytes; the
texture needs 1,048,576. Allocation guard/rollback and measured free memory include
alignment overhead. Loading takes place after the previous GPU frame drains;
closing releases both allocations. At most one load occurs per changed selection.
The three-instruction screen-space shader begins at slot 100. Font uses 96 and
world uses 0; all passes restore their own attributes/depth/texture state.

Coordinates use u=(left-player.y)/(left-right), v=(top-player.x)/(top-bottom).
The smallest containing rectangle selects the current map, with an explicit
continent option. Overlapping maps need further field testing. Zoom/pan stay
inside the valid image; marker drawing clips at the viewport. The UI captures
movement, camera, triggers and buttons, including the close edge and held input
until neutral, and closes on lost world/context/controller.

WXTL telemetry appends 14 map words to WXTK (228 words/912 bytes): open, map ID,
ready, loads, failures, requested bytes, zoom x1000, center u/v x100000, marker
valid, player u/v x100000, revealed overlay count and UI revision. Profiling keeps
map load/composite work in the streaming phase and map drawing in UI/scene drain.


## Original interface assets

`INTERFACE.WUI` contains a 512x512 BGRA swizzled atlas, 285 font glyph records
(three English ASCII sizes/faces) and seven menu sprite records. The host tool
extracts local FRIZQT__/MORPHEUS fonts and original Glue artwork through the pinned
StormLib/WoWee decoders; Pillow rasterizes only during desktop preparation.
The fixed native atlas and 2,048-quad vertex store request 1,376,256 bytes total.
Header lengths, dimensions, IDs, glyph bounds and metrics are checked before GPU
allocation, including an 8 MiB headroom guard. Drawing clips to 640x480 and rejects
non-finite/excessive coordinates. Overflow increments a failure counter instead
of expanding storage. Shader slot 104 follows the existing text/map programs.
Each pbkit CPU block stays below 128 words. New WXTO telemetry adds four UI
counters, totaling 1,016 bytes and retaining older capture decoding.

This is the reusable rendering foundation, not a complete FrameXML/Lua port.
The original character-selection/create/name flows use its artwork and pass
native replay and screenshot inspection. Appearance preview and the broader
interface still need work.

## Account and realm front end (native workflow verified)

`LOGIN.BIN` selects the ordinary account menu (MENU), explicit development
auto-login (NONE), or a private account-flow replay (WXLT). Missing/unrecognized
configuration opens the account menu. Normal initialization copies only the
configured server/account name, clearing the password; the test mode deliberately
preloads disposable credentials. Submitted passwords are cleared from the UI,
command mailbox and worker copies. The controller keyboard covers all 95
printable ASCII characters, with 16-character account/password fields. Server
entry currently accepts IPv4 plus port; DNS resolution remains unfinished.

The network worker owns authentication and the world session. A fixed command
mailbox and generation token keep cancelled requests from entering a stale
session. Authentication waits poll cancellation at 100 ms intervals, within the
existing overall timeout. Realm cancellation forgets the authentication key.
World-session cancellation is still bounded by its existing connect/entry timeout.
The character picker now observes logout while waiting for a command.

`realm.c` parses the build-5875 payload with a fixed 65,535-byte receive buffer
and at most 255 records. It preserves realm name, endpoint, type, flags,
population, character count and category. Two validation passes prevent partial
publication without a large stack copy; the bounded working sets are static.
The vMaNGOS Vanilla response does not append the later version tuple for flag4.
Empty lists remain valid authentication results and offline/invalid realms cannot
be selected. Session/list copies are protected by a short lock. No credentials
or session keys appear in the UI telemetry.

WXTP extends WXTO with eleven public account/replay counters: 265 words/1,060
bytes. The receiver accepts up to 2,048 bytes and retains old-format decoding.
The original title scene, exact PC layout and remembered-account preferences
remain separate implementation work.


## Prepared animated glue scenes

`TITLE.WXB` is a separate bounded scene asset (`WXB1`, version1), generated by
`wowx_assetc --backdrop <Data> <M2 member> <output>`. It uses pinned WoWee M2/BLP
parsers, preserves independently timed bone/alpha/weight tracks and uses native
C quaternion interpolation/hierarchy/skinning. Nine timestamps across global
and local sequence boundaries agree with WoWee/GLM to 0.000001431 units on the
supplied title. Textures have 128x128 top mips, Morton mip chains and original
wrap flags. Total requested CPU+GPU residence is capped at8MiB with a separate
8MiB free-memory guard. Title residence is released before the character lobby
or online world; no per-frame allocation is used.

The current title has7,970 vertices,57bones,22batches,20mesh textures,79tracks and
352keys. File size2,139,220bytes; requested total4,142,740bytes. Texture/bone/track
ranges, geometry indices, parent ordering, finite values, quaternion normalization,
allocation failures and independent periods have host tests. Runtime rejects
non-finite/excessive composed transforms. NV2A program112 has18instructions,
with Cg literal c8 explicitly uploaded. Public WXTQ telemetry adds8scene words
(273words/1,092bytes), including CPU+GPU bytes, draws, animation updates, failures
and lifetime loads. The recorder still accepts older captures.

The raw M2 camera FOV needs conversion before vertical perspective projection.
The initial direct-radian screenshot exposed card edges. Current conversion uses
0.6radians per stored unit, consistent with [Open Realm's camera evaluator](https://github.com/corepunch/open-realm/blob/main/games/world-of-warcraft/renderer/m2/r_m2.c)
and approximately34.5degrees in [WoW Model Viewer](https://wowmodelviewer.github.io/wowmodelviewer/ModelCamera_8cpp_source.html).
The title's authored camera translation/target/roll tracks were inspected in the
supplied asset: each has one zero key, so its static base camera is sufficient.
That does not prove animated cameras on other glue scenes are supported.

Pending fidelity:28particle emitters, authored lights/fog, animated RGB tracks,
other-scene camera animation, and exact PC screen layout. Billboard bones and UV
animation are rejected by this exporter until supported. The title contains no
such bones/UV animation and no spline tracks. Generic spline tangents remain a
pinned-loader limitation, explicitly reported by conversion. This is geometry/
animation/material progress, not completed title or full-game parity.

## Title scene effects and lighting (September 16)

The private WXB scene format now supports a version-2 extension, leaving the
124-byte v1 header and geometry tables readable. It carries up to64 emitters,
8 lights, per-material RGB tracks and a fixed2048-particle capacity. The local
VanillaScene adapter validates the256 M2 layout, reads its uint16 blend/type
fields correctly, exports enabled tracks and preserves original lifecycle
colors/scales (including zero alpha and very small scale). Upstream stays pinned.

The title uses28 emitters,4 lights,25 textures and416 independently timed tracks.
The native runtime allocates all simulation, quad and lighting buffers at load,
checks the existing8MiB scene budget and8MiB free-memory margin, and releases
these buffers on leaving the login scene. No per-frame heap allocation occurs.
A deterministic PRNG, bounded time slices and counted overflow/clock skips bound
work after stalls. Emission uses the prepared rate/life/position/bone and color/
scale data. The NV2A shader and existing material blend modes draw the quads.

Lighting samples original ambient/diffuse/intensity/attenuation/visibility and
bone transforms. Only lit material vertices require it. Per-vertex dirty bytes
and an exact sampled-light comparison reuse unchanged results; no approximate
light-update frequency is introduced. Bone tracks remain evaluated at the frame
rate; vertices retain their previous skinning result only when every contributing
bone matrix is byte-for-byte unchanged. Host checks compare both caches against
forced recomputation at the same animation time.
A shared camera basis/projection structure serves mesh and particle rendering
and supports a smaller viewport projection for forthcoming character previews.
Character-preview composition, viewport clipping and camera-track animation are
not implemented by that API alone.

Fidelity gaps remain explicit: advanced particle drag/spin/follow/tumble,
precise sphere distribution and legacy head-cell repeat/decay timing, translucent
sorting, camera tracks, fog, and exact PC widgets. Current sprite sheets advance
through their cells over particle life; this is not a verified reproduction of
all original cell-range semantics. These effects are a first native implementation,
not completion of spell effects or full PC title parity.

Format references (schemas/behaviour inspected; no third-party code copied):
https://docs.rs/wow-alchemy-m2/latest/src/wow_alchemy_m2/chunks/particle_emitter.rs.html
https://github.com/corepunch/open-realm/blob/main/games/world-of-warcraft/renderer/m2/r_m2.c
https://github.com/wowmodelviewer/wowmodelviewer/blob/master/src/games/wow/particle.cpp

## Character previews (September 16)

WXB v3 appends a20-byte stand-mark record after the24-byte effects extension;
v1/v2 stay readable. The converter exports M2 attachment0, its bone and position.
Runtime validation bounds the record and rejects non-finite positions/invalid
bones. The current sampled bone matrix places the character on that mark.
Human and Dwarf original scenes are staged; Gnomes share the Dwarf scene.

WxPreview builds a display-only WxWorldView from the selected roster entry or
creation draft. It copies the server's appearance/equipment/hide flags, without
submitting position or gameplay commands. The avatar loader keeps one profile,
composes equipment, streams coherent Stand parts and updates the pose. Selection
releases old allocations before loading a replacement; unavailable appearances
cannot leave the previous model visible. The world avatar is released on lobby
entry; the preview avatar/background release on leaving the ready lobby.

The existing avatar renderer accepts a framing/appearance context. Projection
and near/far depth match the background; vertex/texture/depth/blend state resets
when moving from the scene shader to the avatar shader. The world rendering path
retains its prior projection. Right-stick rotation/zoom are bounded, consume no
world movement and reset when switching character/race/sex. No frame allocations
are added once assets are resident. Current preview loading is synchronous:
measured transitions reach906ms even though ready frames have33ms median timing.
This must become bounded incremental loading before smooth-transition parity.

WXTS appends13 preview telemetry fields (294words/1176bytes). Older telemetry
remains decodable. Native evidence covers16default profiles, equipped roster,
rotation/zoom, draft cancellation and saved-world re-entry; no character creation
or deletion occurs. Missing racial backgrounds, starter outfits, customization,
scene gaps/near-plane behaviour and caption framing remain explicit fidelity work.

## September 16 afternoon candidate: outfits and staged preview loading

Implemented in a new development candidate; NOT yet accepted in xemu or on
physical hardware. The user's transferred dist/WOWX-hardware-20260916 package
and checkpoint 20260916-1212 remain the verified executable/assets for that test.
No console files, server settings or saved characters were changed in this batch.

- Creation previews now select starter equipment from the supplied packed
  CharStartOutfit.dbc. The bounded WXOF catalog contains 82 rows/8216 bytes;
  two rows are unused Dwarf Mage variants. All 80 legal race/class/sex outfits
  compose through the native C avatar path on the host, without missing items.
  Sixteen appearance packs contain reusable item components, not prebuilt outfits.
- Fixed the Vanilla RGB table offset (colors 0x54, transparency 0x64). First RGB
  values cross-check against pinned WoWee. Regenerated title/Human/Dwarf scenes.
  Zero-duration global tracks retain their initial key; Scourge exports correctly.
  Tauren's 244 bones fit the new 256-bone bound (u8 skin indices). Both additional
  backgrounds match pinned skeletal sampling at nine independent timestamps.
- Background loading now has cancellable read/validate-allocate/upload/ready
  stages. Each pump reads at most 64 KiB or uploads at most 64 KiB of textures.
  Partial allocations are accounted and release on cancellation or failure.
  The allocation/validation/initial-pose stages remain bounded but need native
  timing; this is not a claim of a proven 33 ms worst-case frame.
- Prepared avatar animation headers are contiguous within the existing WXP v5
  format. Runtime prefetch is capped at 64 KiB, with the old layout still readable.
  Sixteen profiles use one index read instead of 235-385 seeks/reads. Comparison
  proves every vertex/index/texture/skin/pose payload and entry meaning unchanged.
- WXTT appends class/outfit/loading/index telemetry (299 words, 1196 bytes).
  Synthetic host UDP verified old WXTS and new WXTT column alignment.
- Revised PreviewReplay combines title/login and the complete 80-outfit draft
  inspection with world return. 472 records / 11734 raw frames, native streamed
  record cap 1024, no frame allocation. Host UI replay visits all 80 combinations,
  cancels the keyboard/draft and selects the original character without creation.
  Login fixture yields to the raw trace after its completed stage. Planned native
  capture is 600 seconds; check-preview.py --outfits plus check-backdrop.py
  --world-transition. Use fresh saved-state evidence; checker no longer assumes
  the user's character still has XP 768/money 10 after physical play.

Host results: targeted backdrop cancellation/allocation, Vanilla adapter, outfit
schema/malformed cases and runtime boundary suites pass. Actual preview lifetime
and 80-outfit composition passes 502 checks. Logs: preview-outfits-host.log,
preview-outfits-trace-host.log, preview-index-layout.json, preview-cook-*.log.
Source/copy/native build identity: preview-outfits-untested-receipt.json. Native
XBE is 1,855,488 bytes, XISO 431,226,880 bytes, normal MENU/live-input fixtures.
No new native screenshot, frame-time or hardware result is claimed. The prior
906 ms selection stall remains the last native measurement until this build runs.

Native acceptance is intentionally deferred while the user is preparing the
physical test of the stable package. Do not log into the shared test account from
an automated replay during that session. Keep advancing independent implementation.
Orc/NightElf emitter-flag semantics, scene edge/near-plane gaps, appearance
customization, full UI/gameplay/audio/world coverage remain open. The ambiguous
particle flags have not been silently accepted or removed.

## Racial preview particle spaces

See [PARTICLE-FLAGS.md](PARTICLE-FLAGS.md) for the build-5875 mapping. The runtime
now keeps local particles in bone space until quad preparation, preserves the
full spawn vector transform for released particles, and caches scale/XY bases
once per emitter. WXB ABI and fixed capacities are unchanged. All six racial
scenes (eight race selections) now have cooked assets; native acceptance remains
pending while the user prepares a physical test of the earlier checkpoint.


## Character deletion and failed-login recovery

The lobby has one in-flight command and publishes pending/result kinds separately
from their revision. The controller stores a GUID/name snapshot and a bounded
seven-byte DELETE entry buffer; no per-frame allocation was introduced. White
opens confirmation; the shared 28-key keyboard returns to a separate final A
confirmation. Pending requests disable input. Reordering updates the selection
by GUID, while disappearance or a changed name cancels the pending confirmation.
Deletion screens retain the roster subject/equipment rather than a creation draft.

The wire implementation uses pinned vMaNGOS build-5875 ResponseCodes and
Character.h/CharacterHandler.cpp: CMSG_CHAR_DELETE 0x38 with uint64 GUID;
SMSG_CHAR_DELETE 0x3c with exactly one terminal byte (0x39 success, 0x3a failure,
0x3b transfer lock). Do not substitute modern-client success codes such as 0x47.
Each result triggers enumeration, and success additionally requires absence of
that GUID. The 15-second reply/refresh budgets are separate and bounded. An
interrupted or inconsistent transaction is explicitly unconfirmed, with no
automatic resend; only a new connection/roster can resolve its actual outcome.

Character-response waits reject other roster/create/delete/login replies rather
than skipping them. World-entry wait accepts SMSG_CHARACTER_LOGIN_FAILED 0x41 as
an alternative to LOGIN_VERIFY_WORLD 0x236; valid error codes 0x3e-0x44 refresh the
roster and require another selection. The existing 60-second cold-map success
budget remains. Idle keepalives are unchanged. Encrypted loopback fixtures test
these paths without connecting to vMaNGOS or loading private account configuration.
Native screenshots, physical controller input, destructive operation on an isolated
server account and memory/frame measurements remain acceptance gates.


## Shared appearance layers (September 16 candidate)

WXA v2 and WXL replace per-combination preparation with shared race/sex geometry,
indexed texture layers, dynamic geosets and separate hair/extra texture bindings.
The UI reads the loaded avatar catalog through a non-owning pointer; catalog
identity checks prevent using the previous race/sex's choices during a switch.
The avatar owns and closes its catalog. A changed look invalidates composition
while retaining the geometry pack/cache; race/sex changes release the previous
profile before loading the next. Creation packet fields and server appearance
bytes converge on the same runtime selection path. WXA v1 remains supported.

Source-data rules, byte bounds, per-race choice counts, reference checks and
native acceptance gates are in [CHARACTER-APPEARANCE.md](CHARACTER-APPEARANCE.md).
Composition remains bounded but synchronous when a look/item changes; its native
worst-case read/upload/frame cost still needs measurement. The existing preview
replay does not yet exercise customization. No additional memory or frame-time
claim should be inferred from host reference or allocation tests.


### Customization telemetry and replay isolation

WXTU extends WXTT by six little-endian uint32 words (1,220 total bytes, below one
ordinary UDP payload): rendered packed skin/face/style/color, rendered facial
feature, composition revision, CPU RGBA atlas FNV-1a, selected UI appearance row
and randomization count. Rendered values are zero unless the preview avatar is
ready and composed. They come from the avatar's active header, not the UI draft.
The CPU atlas hash is taken on composition only, before destructive mip building.
The recorder accepts all previous packet versions and rejects mismatched magic,
length and non-finite floats. `preview_validation.py` checks that selector changes
actually produce drawn samples and new compositions, without treating hashes as
proof of complete PC visual fidelity.

`prepare-replay.py --output <folder>` creates isolated fixture files after argument
and size validation. The destination argument cannot itself trigger automatic
login. The combined trace remains within the unchanged 1,024-record native bound.
Manual-input staging and its archived receipt remain distinct from a later
replay-mounted image and its receipt.


### Unit HUD and WXU1 v2

The fixed UI atlas now contains ten sprites; WXU1 v1 (seven sprites) remains
readable, retaining the older text HUD. Dimensions, glyph count and graphics
allocation are unchanged. `wx_ui_image_region` supports bounded source rectangles
and horizontal reversal; bar filling crops the gradient instead of stretching it.
`wx_ui_text_fit` clips long names with an ellipsis. All additions share the existing
2,048-quad pool, and no frame heap allocation is required.

`hud.c` reads fixed entity snapshots, selecting power fields from Vanilla's active
resource byte. Data is recreated each frame, so absent targets cannot retain an
old name or resource. The UI suppresses the new unit frames for modal screens and
diagnostics. The native WXTV extension records the actual HUD snapshot/submission
counts; host software layout captures remain explicitly distinct from xemu proof.
See HUD.md for source provenance, exact scope and missing full-parity behavior.


### Action feedback, casting and WXS v2

WXS v2 extends 108-byte name/rank records to 196 bytes with Vanilla resources,
costs, ranges, targets and stance/attribute data. Converter joins SpellRange by
ID; file boundaries and loaded records are validated. v1 names remain readable.
Fixed storage: 512 learned entries plus eight queued cast-name entries; one record
read per frame. Unknown skill/aura costs remain advisory and never block commands.

wx_cast owns a 28-byte state behind the world lock. wx_spell_wire shares strict
START/GO decoding with cooldowns. UI uses read-only cast/availability snapshots.
B sends the spell-only Vanilla cast or channel cancellation command.

WXTZ remains 1,472 bytes. Separate WXF1 datagrams (264 bytes) hold cast/action
snapshots, frame/fixture identity and fixed state sizes. Recorder writes a separate
.actions.csv; auxiliary traffic cannot extend the capture lifetime. WXPF0005 is
credential-free and cannot be staged in a normal image by build-xbox.ps1.

### Staged world asset jobs and terrain indices

World payloads now use an unpublished incremental job, bounded byte/read/validation
quotas, cancellation rollback, collision priority and prefetch/retention distances.
Terrain indices prepare independently and publish only after complete validation.
Index capacity, pending data and actual free-page telemetry cover transition
memory. WXS1 is a separate 128-byte companion; WXTZ remains 1,472 bytes. See
STREAMING.md for the contract and two-cycle offline native result. Actor/avatar
payload scheduling, profile opening and composition remain separate work.

### Bounded animation payloads and model-space fog

NPC and player streaming now advance the same staged loader with per-scene
64 KiB read/validation limits and eight read operations, two commits per call.
Only the offline pack verifier drains synchronously. Complete-pose retention
covers pending modular transitions; paired idle/run retention extends to pending
player jobs. WXN1 adds 164-byte actor/player resource telemetry, separate from
WXTZ. See ACTOR-STREAMING.md for native scope and remaining scheduling work.

The world fog depth multiplier is one. Model rendering divides the camera by
model scale and multiplies view axes by that scale, so its shader depth is already
world depth. Scaling fog again was an error for non-unit actors; this follow-up
build now has scoped native visual acceptance at scales .5/1/2 in the authored
fog batch. Historical capture/source identities remain separate.


### Authored fog and authoritative time

WXL1 LIGHT.WLF has validated fixed-size volumes/sorted fog bands; private host
conversion reuses pinned WoWee mapping and strict Vanilla DBC layouts. Startup
loading has a384KiB ceiling and actual free-page guard; all-map fog-data arrays
occupy135,040bytes, accounted in telemetry. Sampling allocates/reads nothing.
Empty source bands and unknown environments use explicit fallbacks. Full sky/
material lighting is separate; see FOG.md. The network reader validates the
Vanilla clock even before world verification and exposes it under the existing
mailbox lock. Graphics samples half-minutes from that snapshot. WXL2 carries
84bytes of profile/fog/timing telemetry without enlarging WXTZ. Full live-world,
physical controller and hardware gates remain.
