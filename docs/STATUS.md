# Status - 2026-09-17 overnight

## Latest hardware handoff — September 17 revision 2

Ready: `dist/WOWX-hardware-20260917-r2.zip` and its extracted `WOWX` folder.
The requested latest build includes the staged index-publication checkpoint,
cooperative streaming clocks and all previous UI/fog/environment/liquid work.
XBE SHA256: `2d7b84b03071fad2c02ca3eab7d444fadab3e56683f093189e9a6eee1379775d`.
All 153 files / 1,060,926,510 bytes match the manifest; all 24 world packs and
the saved Northshire floor pass the current host loader. ZIP CRC and all archived
game-file hashes pass. The ZIP is 439,884,400 bytes; a separate 1,061,683,200-byte
XISO is included alongside it. See [revision 2 instructions](HARDWARE-20260917-R2.md).

The exact executable passed three offline terrain-water cycles in 64 MiB xemu,
with 29.27 MiB minimum free and 33/34/34/56 ms guest frames. Combined live
gameplay, physical controller and actual Xbox performance remain unverified.
No new gameplay coverage or full Vanilla release acceptance is claimed.

WOWXTEST's requested password matches both the private store and a read-only
server SRP calculation. Five characters remain; no world login, character edit,
repeat password reset or server restart was needed. The existing LAN server at
192.168.50.156:3725 / world 8086 is ready. No password is packaged.
Every game file in both earlier hardware kits still matches its original manifest.
All three kits, shared account, saved progress and LAN realm remain reserved for
hardware testing. No emulator remains and no files were transferred to the console.
Delivery/checksum evidence: `build/evidence/hardware-r2-delivery.json`.

## Earlier hardware handoff — September 17

At the user's request, `dist/WOWX-hardware-20260917/WOWX` contains the latest
native executable with the two v10 terrain-water packs and twenty converted
global WMO packs. The merged index retains all four terrain tiles: 24 regions,
153 files / 1,060,926,510 bytes. The ZIP is 439,887,638 bytes; a separate XISO is
also provided. This is a development hardware candidate; combined live native
gameplay and physical hardware acceptance remain open. Every file in the old
September 16 kit still matches its original manifest. See
[the current hardware instructions](HARDWARE-20260917.md).

All 24 world packs pass the host production loader. Xboxdawn's saved position
loads and resolves its floor without errors. The native executable is unchanged
from the terrain-liquid capture, SHA256
`26b72603b6400cd31dbc155afa0282fb588c3841f7e3adaf1e5c64f5e46a7e40`.
Every game-file hash inside the ZIP matches the staged manifest; ZIP CRC and
XISO structure checks pass. Earlier offline xemu memory/timing evidence must not
be attributed to a physical Xbox or to this merged live-game package.

The user-authorized local account password change passed an authentication-only
handshake. WOWXTEST and all five characters remain; character rows were unchanged
by the reset. Private backups and the updated credential store stay under
`server/`, outside packages/source control. The package contains no password.
Workspace realmd/mangosd were started once; the existing LAN forwarding,
Xbox-scoped firewall rule and advertised endpoint 192.168.50.156:8086 are ready.
No native emulator or world login was started for this packaging task.

**Hardware reservation:** preserve this new package, the older kit, credentials,
saved progress and LAN realm address during the user's test. Do not log into the
shared game account or replace console files during autonomous development.
Future xemu authentication fixtures must be regenerated from the updated private
credential store after the account is released; older frozen discs retain their
original private test-auth data. Independent source/offline work may continue.

## Latest feature checkpoint

Latest independent batch: `build/candidates/20260917-draw-submission`. Larger
NV2A index packets and separated submission/drain telemetry pass host checks and
a combined native run. Two moving world/NPC/player/appearance cycles:8,267 main
rows,8,266 exact companions,28.89MiB minimum free; guest frames33/34/34/36ms.
Index submissions fall57.14% for the exact geometry drawn. Submission is3/5/5/10ms
and final drain1/3/5/17ms; these are guest wall times, not hardware timestamps.
346 multi-frame selections,32 appearance commits/64 cancellations, three staged
index publications and all shared quotas pass; no avatar gaps or asset failures.

Actual images retain loading gaps after explicit relocation and low animation
poses intersecting terrain; a full standing pose is intact. These are remaining
visual/animation issues, not full PC parity acceptance. The original terrain-water
43ms drawing spike used different packs and remains a profiling follow-up.
The first emulator launch failed before telemetry with host0xc000041d; unchanged
retry passed. Owned xemu was quit, with the previously seen post-quit GLib message
retained; no emulator remains. All three hardware kits, shared account and LAN
server remain untouched. See [DRAW-SUBMISSION.md](DRAW-SUBMISSION.md).

Previous independent batch: `build/candidates/20260917-selection`. Spatial selection
now examines at most 2,048 entries per frame and publishes a complete candidate
list. Movement cannot repeatedly restart the scan; index mutation/large relocation
invalidates stale work. Host consumers and ten telemetry test groups pass.
Three offline terrain-water cycles pass: 6,898 main / 6,897 exact companions,
29.23 MiB minimum free and 33/34/34/48 ms guest frames. Non-payload selection
frames take at most 4 ms of stream work; first index-publication stream work
falls to 0 ms, with a remaining 43 ms drawing spike. See SELECTION.md.
The three delivered hardware kits, shared account and LAN server were untouched;
no emulator remains. Full-game and physical release gates remain open.

Previous independent batch: `build/candidates/20260917-index-publication`. Joined
world tables copy in 32 KiB chunks, at most 256 KiB per frame, then publish on a
later frame. Cancellation/source replacement and temporary allocation accounting
are covered. Three offline ten-case terrain-water cycles pass: 6,149 main / 6,148
exact companions, 29.27 MiB minimum free, 33/34/34/56 ms guest frames. Copies take
at most 2 ms and the three publications 0/1/0 ms versus 18–19 ms previously.
Selection still takes 7–8 ms and first-publication drawing 46 ms. No full-world
or hardware performance pass follows from this localized improvement.
See INDEX-PUBLICATION.md. Native actor/player integration and physical release
gates remain open; the user's latest hardware package request is handled separately.

Previous independent batch: `build/candidates/20260917-stream-clock`, cooperative
streaming time slices and WXS3 telemetry. Host checks, native build and two
offline terrain-water cycles pass with30.02 MiB minimum free and1,432 world
time-yield frames. Guest frames33/34/34/81 ms (p50/p95/p99/max): no frame-time
improvement is established. The worst boundary is localized to19 ms index
publication,7 ms selection with no reads, and52 ms drawing. Joined-index
allocation/copy, selection and draw work remain synchronous. See STREAM-CLOCK.md.
Neither delivered hardware package changed. No emulator remains; native combined
NPC/player/gameplay and physical hardware acceptance remain open.

Latest scoped feature batch: `build/candidates/20260917-terrain-liquids`.
Original terrain water now uses the shared bounded liquid renderer: 115 MCLQ
grids / 3,835 cells across two prepared Azeroth tiles, with exact original
visible heights and masks. Ten offline camera cases pass one 64 MiB xemu run:
30.02 MiB minimum free; guest frame p50/p95/p99/max 33/34/34/69 ms. Boundary
index/payload and draw work still cause short stalls. Actual lake rendering is
visible; river/shore appearance, swimming and full-map coverage remain open.
See TERRAIN-LIQUIDS.md. That feature batch left the earlier hardware kit and shared account untouched; the subsequent authorized hardware handoff is recorded above.

Previous scoped feature batch: `build/candidates/20260917-liquids`.
Original WMO liquid grids now produce animated water/magma surfaces in the native
client. Twenty packs add 386 bounded patches while preserving exact earlier
geometry/materials. Fourteen original-data camera cases passed one 64 MiB xemu
run: 29.59 MiB minimum free; guest frame p50/p95/p99/max 33/34/34/55 ms.
All 30 original texture frames were submitted on three liquid maps. The earlier
missing-label-glyph report was not corroborated on rereading the saved images:
all 35 non-space glyph boxes contain gold strokes. An indexed-triangle experiment
showed no demonstrated improvement and was preserved separately, then reverted.
Full UI visual acceptance remains open. Swimming, terrain liquids, exact water appearance and
physical hardware remain unverified. See LIQUIDS.md and the final checkpoint.

**The full Vanilla port is incomplete.** The native Xbox client connects to vMaNGOS,
renders Northshire, server creatures and the test player's animated avatar, moves, targets, fights, gains XP, opens
loot, collects items, accepts/completes a quest, equips items, uses Heroic Strike
and reconnects with saved progress.
Controller character selection, creation and a name keyboard pass a native
create/enter/switch-back test. A controller quest journal now reads server quest
text and displays live objectives, completion flags and bag item counts.
A controller spellbook now lists visible learned abilities, identifies passives,
and supports confirmed assignment/clearing of controller action bindings.
The Black-button utility wheel opens bags, spellbook, quests and settings. A fresh
character now passes a continuous two-quest loop with ten kills, spell hits,
loot, walking return, rewards and saved reconnect. Avatar animation transitions
retain a complete body, and the bounded texture font reduces text-rendering cost.
Back now opens a bounded zone/continent map with explored areas, player position,
zoom and pan; native controller replay passes. A clean native screenshot now
verifies the explored Northshire map, player marker and red corpse marker.
Death, spirit release, Spirit Healer resurrection and saved reconnect pass in a
separate scenario. Corpse-run recovery also passes: ghost reconnect, a 198-yard
collision-constrained return, timed reclaim and alive reconnect. The full
combined combat/quest/death walkthrough remains unverified.

## Acceptance matrix

| Milestone | State | Evidence / remaining work |
|---|---|---|
| Boot and feasibility | Demonstrated in xemu | Native XBE/XISO, 64 MiB, textured animation, input replay and memory/timing measurements. Physical controller transport remains unverified. |
| Real world rendering | Partial | General batch converter, four verified terrain packs on both continents, bounded region streaming, buildings/props, NPCs and animated player with live clothing/equipment components. Third-person camera and native tile crossing pass local tests. Complete appearance/NPC equipment coverage, water and effects remain. |
| Playable quest loop | Partial | Fresh continuous quests 783 and 7, ten kills, Heroic Strike hits, loot, walking return, rewards and saved reconnect pass. Separate creation, gear, vendor, hearthstone, Spirit Healer and corpse-run tests also pass. A combined death/quest session and the remaining controller UI are still open. |
| Vanilla coverage | Not met | Full race/class/continent/profession/travel/social/pet/instance/PvP coverage remains. |
| Stability and packaging | Partial | A continuous 61-minute walking/UI/reconnect test passes in 64 MiB xemu. Mixed combat/crowded-scene stress, release packaging and hardware validation remain. |

## Actual xemu evidence

xemu 0.8.136, Vulkan host renderer, native scale, 640x480 and **67,108,864 bytes of
RAM**. Dedicated configuration, copied EEPROM and disposable HDD writes.
Frame intervals use the guest clock, not physical Xbox measurements.

| Local capture in build/evidence | Samples | Minimum free KiB | p50 / p95 / maximum ms | Outcome |
|---|---:|---:|---:|---|
| final-replay | 5,636 | 44,740 | 34 / 34 / 50 | Historical pre-creature controls/movement/save/reconnect pass |
| creature-optimized | 3,446 | 35,064 | 34 / 35 / 99 | Before gameplay and corrected model scale |
| native-quest | 3,393 | 34,676 | 34 / 50 / 99 | Quest 783 accepted and persisted |
| native-combat | 4,653 | 36,444 | 34 / 51 / 156 | Kill and 50 XP persisted; empty drop failed loot acceptance |
| native-combat-loot | 5,753 | 35,268 | 34 / 50 / 165 | Kill, two items collected, XP increased 50 to 100, reconnect passed |
| native-turnin | 4,501 | 35,704 | 34 / 52 / 234 | Quest 783 completed for 40 XP; follow-up quest 7 accepted |
| native-inventory | 3,752 | 35,696 | 34 / 50 / 144 | Shield moved to backpack and re-equipped; server confirmed slot 16 |
| native-kobold-diagnostic | 17,701 | 34,128 | 32 / 37 / 305 | Eight kills, nine Heroic Strike hits, three items; failed after an observer teleport |
| native-tile-boundary | 2,839 | 35,076 | 33 / 34 / 102 | Both packs loaded; fixture was blocked on steep terrain, crossing failed |
| native-tile-crossing | 4,052 | 35,124 | 33 / 34 / 102 | Walked from (-9048,-160) to (-9080.21,-160), crossed 48/49 and reconnected at the saved position |
| native-humanoid-fixed | 2,599 | 35,356 | 33 / 39 / 98 | McBride body, clothing and shoulder armor rendered; online neutral capture |
| native-kobold-turnin | 4,050 | 35,232 | 33 / 37 / 188 | Quest 7 reward: 170 XP, 25 copper, logged out/reconnected, server persistence verified |
| native-expanded-npcs | 2,879 | 30,652 | 50 / 59 / 164 | Expanded to 65 displays; four nearby instances still have missing models |
| native-hearthstone | 5,267 | 29,436 | 33 / 38 / 189 | Failed on captured SMSG_NEW_WORLD, opcode 0x3e |
| native-hearthstone-fixed | 5,098 | 29,300 | 35 / 42 / 147 | Item 6948 cast 8690, transferred to home within connection 1, reconnected in connection 2; zero missing nearby models and zero failures |
| native-death | 12,209 | 32,716 | 33 / 34 / 578 | Died, released and resurrected; reconnect authentication failed |
| native-death-fixed | 11,818 | 30,616 | 33 / 34 / 411 | Dead/ghost/alive states, Spirit Healer prompt, saved reconnect passed; zero missing nearby models or failures |
| native-vendor | 640 | 29,596 | 33 / 45 / 267 | Merchant list rendered; replay assumed incorrect item order and failed before buying |
| native-vendor-fixed | 5,711 | 29,660 | 33 / 34 / 150 | Purchase and sale changed money from 58 to 34; replay waited for a nonexistent successful-sale packet and failed |
| native-vendor-verified | 2,335 | 29,664 | 33 / 35 / 153 | Bought five water, sold one, reconnected with expected inventory and money; zero missing models or failures |
| native-playable | 1,321 | 29,792 | 33 / 34 / 140 | Final normal build, replay disabled, online with persisted progress; neutral capture only |
| native-player-foundation | 527 | 34,692 | 33 / 34 / 34 | Offline: local server processes had stopped; not a passing connection test |
| native-appearance-state | 1,614 | 34,688 | 33 / 34 / 34 | Authentication passed; cold first-map load exceeded the old login timeout |
| native-appearance-state-fixed | 1,651 | 36,848 | 33 / 34 / 48 | Online at the user's newer saved interior position, XP 768; neutral capture, 11 missing nearby NPC models and visible floor artifacts |
| native-live-appearance | 1,224 | 34,576 | 33 / 34 / 162 | Cold boot to saved interior; all 19 live equipment slots resolved through server item queries |
| native-appearance-replay-v10 | 1,930 | 34,572 | 33 / 34 / 162 | Controller UI unequip/re-equip/reconnect passed; entry/model/type preserved, no movement or XP/money change; avatar drawing remains pending |
| native-projected-z | 987 | 34,568 | 33 / 34 / 160 | Full interior floor gaps removed by correcting depth interpolation; boot-to-world capture, normal controls, no asset/region failures |
| native-appearance-normal | 959 | 34,568 | 33 / 34 / 164 | Earlier normal build, replay disabled, floor correction and resolved live equipment verified; boot/loading included |
| native-avatar-first | 1,263 | 34,224 | 33 / 34 / 260 | Native Human body, clothing, sword and shield; 14 draw batches, 1,005,828 avatar bytes, animated vertices, final missing-avatar count zero |
| native-avatar-gear | 1,929 | 34,220 | 33 / 34 / 247 | Shield draw removed/restored through controller UI and retained after reconnect; body and sword remain; progress unchanged |
| native-avatar-camera | 1,158 | 32,924 | 33 / 47 / 263 | Four-heading and pitch sweep, collision pull-in, restored view/position; expanded 92-display pack, zero missing nearby NPCs |
| native-avatar-camera-pruned | 1,156 | 32,920 | 33 / 42 / 254 | Same sweep with triangle rejection before ray tests; all camera/avatar checks pass. Moving-view p95 remains 37 ms, so no demonstrated sustained speedup |
| native-avatar-normal | 919 | 32,924 | 33 / 40 / 248 | Earlier normal build, no replay, avatar and nearby NPCs rendered; final missing counts zero; boot/loading included |
| native-character-create | 2,660 | 32,880 | 33 / 44 / 475 | Controller roster/keyboard/creation, new character entry, return to original with progress intact; no asset/region/session failures |
| native-character-screens | 4,923 | 32,720 | 33 / 36 / 192 | Paced roster/form/keyboard inspection, empty draft cancelled, original character restored; actual roster/keyboard screenshots |
| native-starter-quest | 3,042 | 26,612 | 37 / 61 / 394 | Continuous fresh-character delivery quest 783, followup 7 accepted, 60.175 yards walked, saved reconnect |
| native-camp-quest | 10,238 | 26,848 | 33 / 53 / 415 | Walked Abbey to camp, eight kills, three loot items and level two; failed a blocked target approach |
| native-camp-quest-journal | 4,830 | 33,104 | 33 / 42 / 428 | Journal at 8/10, two more kills and loot; return walk hit a tree and failed |
| native-camp-return | 5,121 | 26,900 | 33 / 60 / 602 | Continued at 10/10, journal objectives/description, walked 213.90 yards back, received 170 XP/25 copper and reconnected; return-only checker passes |
| native-journal-normal | 1,782 | 32,788 | 33 / 34 / 299 | Current normal build, replay/return-route disabled, boot/loading included, original saved character intact; avatar and source/output receipts pass |
| native-character-normal | 2,099 | 32,908 | 33 / 34 / 51 | Earlier normal build, replay disabled, 70-second neutral online capture; complete Human avatar and saved progress |
| native-visible-animation | 2,210 | 30,620 | 35 / 50 / 256 | Same camera and expanded pack, skinning only visible templates; 30 fps still not consistently met |

These captures had zero asset-load failures and more than the required 8 MiB
headroom. **30 fps is not consistently met**: 50 ms intervals and longer loading
frames remain. These historical captures do not prove crowded-scene stability. The later
61-minute walking/UI/reconnect result is documented below.

The first humanoid build stalled before telemetry because Windows clang's dependency
paths did not match MSYS make's object paths. That failed capture is retained as
`native-humanoid.csv`; it is not a passing result. Public-header changes now rebuild
all project modules, preventing mixed structure layouts. The rebuilt native run passed.
The NPC performance comparison used the same camera/pack, but moving server NPCs and
concurrent host work were not controlled. Do not treat it as a hardware benchmark.

The kobold diagnostic used revised vblank pacing, so it is not a controlled
performance comparison with earlier scenes. A nearby rabbit's observer teleport
(opcode 0xC5) exposed an incorrect session termination. The captured packet and
all its truncations are now host regression tests. Later native sessions accept observer relocation.
An independent encrypted-session fixture now proves observer relocation is
accepted while malformed relocation is rejected; controller world transfers are handled separately.
The server confirms quest 7 objectives at 10/10 across the combat runs, and the
character subsequently turned in quest 7 for 170 XP and 25 copper, reaching level 2 with 658 XP.

The tile-crossing capture used two resident packs (39,660 combined entries), zero
asset/tile-load failures, and completed the automatic walking/logout/reconnect
scenario. Actual screenshots are `native-tile-crossed.png` and
`native-tile-crossing-passed.png`. Four simultaneous packs, long-distance eviction,
other continents in the native client and WMO-only instances remain unvalidated.

The hearthstone retry reset only the offline test character position and spell 8690
cooldown. Quest/XP/inventory progress remained intact. Its `.check.json` verifies
item use, transfer without relogging, reconnect, zero packet/asset failures and
measured headroom. The earlier failed capture remains available.

The successful combat run dealt 45 damage, received 9 and collected two Chipped
Claws. `build/combat-loot-server-inventory.log` confirms item 7074 in saved inventory.
`native-quest-details.png` is an actual Xbox quest dialog. CSV/JSON captures and
server logs remain in ignored local build/evidence and server/logs directories.

Combat automation supplies controller samples to normal movement and interaction
code. It never sends gameplay packets or changes position directly. Offline
fixtures near wolves/questgivers isolate tests; they do not prove a continuous
walk-through. Replay does not verify a physical pad or USB transport.
The latest normal build reports an attached SDL controller, but brief automated
keyboard presses did not appear in the guest input records. Interactive keyboard
and physical gamepad operation remain unverified. This is retained as an input
investigation, not evidence of a confirmed client or emulator defect.

## Engine and gameplay boundaries

- Pinned WoWee ADT, terrain, BLP, M2, WMO, SRP, VanillaCrypt, Packet and animation
  samplers are reused. Native platform interfaces contain no Vulkan types.
- world.wxp: 2,555 batches, including 609 collision batches; 454 unique mip chains;
  48,772,396 bytes. This remains the legacy fallback pack.
- Generic catalog: 44 available WDTs and 2,429 terrain tiles. Four converted tiles
  total 422,359,042 bytes, including Northshire, its southern/eastern neighbors and
  an Orc starting-area tile. Whole-world conversion does not require zone-specific
  implementation. WMO-only instance roots still need an export path.
- actors.wxp: 92 display variants, 778 batches, 109 mip chains, 32,144,728 bytes.
  Humanoids use authored NPC body bakes, hair/geoset selection, clothing and rigid
  bone attachments for heads/shoulders. Helmet visibility follows the 5875 masks.
  Complete NPC handheld equipment and full visual fidelity remain.
  Idle/walk/run/attack/corpse poses derive from supplied assets. Combat pose choice
  remains approximate; attack synchronization, death transitions and per-actor blending remain.
- World cache: 32 MiB, 256 visual plus 64 collision slots, three loads/frame.
  Actor cache: 8 MiB, nearest 32 supported creatures, 128 desired batches, two loads/frame.
  Identical animation-pose ranges share memory; previous clips remain until replacement
  clips are complete. The fixed 320 slots and 8 MiB budget still bound transitions.
  Only visible templates are skinned; offscreen templates remain cached for camera turns.
  Both check physical headroom. Audio, compressed textures, LOD and spatial indexing remain.
- Player: a 4 MiB scene budget, up to 32 body/equipment components, two loads/frame,
  shared skeleton poses and separate bounded clothing atlas/mipmap buffers. The
  current Human's 13 selected components produce 14 batches and occupy about
  0.96 MiB combined. Live gear and hide flags select components/geosets; unknown
  looks or item displays are reported. The staged appearance/item catalog is
  intentionally small; complete coverage, sheathing and weapon grips remain.
- A read-only server catalog exports nearby spawn displays and alternate genders
  using current event eligibility. An initial unfiltered catalog failed on inactive
  Winter Reveler display 15713, whose Humanoid display lacks appearance data.
  The active-event catalog prepares successfully and removes the saved location's
  missing NPCs. Seasonal/event override and scripted/summoned coverage remain.
- Transactional 512-entity store, bounded decompression and 5875 ground splines.
  The 128-record main-thread snapshot prioritizes self, selected target and owned
  corpse, then nearest sampled positions. Crowded insertion-order and capacity
  cases pass host tests; the revised selector is running in the native merchant tests.
  Flying/falling/cyclic interpolation is approximate. Runtime uses absolute server
  scale once; model templates retain raw coordinates.
- Target/name/health, auto attack, loot, quest/reward/gossip packets and inventory
  equipment/backpack/four-bag snapshots are implemented. Name lookup is bounded.
  Merchant stock, bundle buying and one-item selling pass native controller tests
  with confirmation and saved reconnect. Repairs, buyback and quantity selection remain.
- Controller character roster and creation use valid Vanilla race/class choices,
  default appearance, a 2-12-letter on-screen name keyboard, a confirmation and
  rejected-name recovery. Native replay created GUID 2 `Xboxnight`, entered its
  Northshire spawn, then restored GUID 1 `Xboxer`. Database evidence independently
  confirms level 2, XP 768, ten copper, all eleven inventory records and the same
  original position. Appearance customization/preview and account login remain.
- Full account login, spellbook/talents, complete vendor services, radial/cursor/general text entry, party
  controls and ground targeting remain. Same-map controller teleports and stale
  movement rejection pass host fixtures. The full world-transfer acknowledgement
  passes native hearthstone testing; cross-map transfers are host-tested only.
  Death, release, Spirit Healer resurrection and reconnect pass natively. Corpse
  query/reclaim controls are implemented but corpse-run recovery remains unverified.
- Initial collision supports floors, steps and wall rays. Full swept collision,
  falling, swimming, moving platforms and broad traversal checks remain.

The first death run reached dead, ghost and living states, then failed authentication
on reconnect. The server reported an incorrect proof despite unchanged credentials.
The adapter now follows the pinned server's natural-width BigNumber inputs for
SRP proofs and world-session keys; deterministic short-key/proof/salt/public-value
fixtures pass. The original failed capture is retained as `native-death.csv`. The corrected native
run reconnected alive at 79/79 health with level 2, 658 XP, 58 copper, ten inventory
records and shield 2362. `native-spirit-healer-prompt.png` and
`native-death-passed.png` are actual xemu screenshots.

Dense scenes previously fragmented the debug HUD after command-buffer wrap.
The 1 MiB buffer now drains conservatively and resets before rectangle-based
text rendering. The death and merchant screenshots show legible text; this is
not an hour-long GPU stress test.

The merchant retry began with four water and 34 copper retained from the prior
trade. It bought one five-water bundle for 25 copper and sold one water for one
copper. Native reconnect and the database both show eight water, ten copper,
level 2, 658 XP and shield 2362 still equipped. `native-vendor-buy-prompt.png` and
`native-vendor-passed.png` are actual xemu captures. No money or inventory was
injected for the retries.

## Validation

- 285 host runtime/camera checks; 609 entity/decompression/motion/inventory/selection checks;
  1,338 gameplay packet/text checks; 19 DBC/WDT checks; 12 live appearance checks.
- 346 character-enumeration checks and 10 atlas blend/boundary checks. Appearance
  fields and all equipment slots are retained; every character-packet truncation
  is rejected.
- 358 character creation/keyboard/controller checks and ten encrypted character
  session scenarios cover selection, empty-account creation, name rejection/retry,
  cancellation, keepalive and malformed responses. All 61 existing encrypted
  world scenarios still pass after integrating the lobby.
- Player preparation: all 16 default race/sex pairs, an equipped Human and an
  alternate Human pass runtime pack validation. Eight malformed profiles are
  rejected without overwriting an existing output. The modular path separately
  passes all 26 cases through the avatar runtime. These host cases do not prove
  native visual correctness for all races; native evidence covers the test Human.
- 137 avatar metadata, compositor, component selection and allocation checks pass.
  Native gear replay checks the body's animated vertex changes and actual shield
  draw removal/reappearance. Camera replay restores heading/pitch and position.
- The humanoid refactor rebuilds the current NPC pack byte-for-byte unchanged
  (SHA-256 `4b410553a36886d08ce7f5e590413d0ea779ba02b64f7326ff10d053f86287ed`).
- 18 independent authentication scenarios and 61 independent encrypted world scenarios,
  including near-teleport acknowledgement, wrong GUID, nonfinite coordinates, trailing
  data and every truncation. Malformed teleports leave the published position unchanged.
  The new encrypted appearance scenario verifies equip, explicit removal,
  replacement, zero-valued appearance changes and resolved item metadata.
- Runtime pack verification loads every batch, checking vertices/poses, indices,
  skin weights and mip boundaries. It detected and helped fix nonfinite model normals.
- Launches now archive source/output hashes immediately after building under
  build/evidence/receipts. Older evidence and receipts describe earlier builds.
- Server: 158 DBCs, 2,429 maps, 6,082 vmaps, initial Northshire mmap only.
  Installed-output verification passed. Fresh extraction workflow and full-world
  pathfinding generation remain unverified.

## Next work

Extend native streaming/eviction coverage, rerun repeated combat with the observer
teleport fix and movement detours, extend player appearance/catalog selection, and complete a
continuous quest/death/reconnect loop. Continue full Vanilla and release testing.

The overnight modular player is now rendered in-game with the saved Human's gear.
A server launcher now recovers stopped local processes
without resetting state. World entry uses a separate 60-second timeout for cold
map loads. New interior evidence revealed floor artifacts and missing nearby NPC
models beyond the earlier Northshire scenes. Depth-range and solid-color shader
experiments did not establish a fix and were reverted. Their evidence is retained.
The investigation did fix shader-only rebuilds: `main.obj` now explicitly depends
on both generated shader includes, in addition to public-header dependencies.

The earlier restored normal textured build was verified in `native-overnight-normal`:
1,501 idle interior samples over 50 seconds, minimum 36,848 KiB free and
33/34/34 ms p50/p95/max, with an active world session and no asset/region failures.
The screenshot still shows floor artifacts, and telemetry reports 11 missing
nearby NPC instances. This capture does not establish travel/combat performance
or stability acceptance. XBE/XISO hashes match the accompanying build receipt.
Current saved progress is level 2, 768 XP, 10 copper and 79 health; older evidence
above describes earlier character state. Overnight continuation is recorded in
`docs/OVERNIGHT.md`.

The saved interior's floor artifact is now fixed in `native-projected-z.png`.
The floor rendered correctly alone; other building batches exposed a mismatch
between nxdk's perspective-depth default and this shader's already projected Z.
The renderer now disables the extra depth correction each frame while keeping
perspective-correct textures. The complete scene is restored, with no diagnostic
geometry filters. This verifies the saved camera; broader views and physical
Xbox rendering remain unverified. The later active spawn catalog removes the
missing nearby NPCs in the saved location; it does not establish full-world coverage.


## Current quest-journal checkpoint

Start -> A opens the controller journal; D-pad selects, A reads, X switches
objectives/description, left/right pages, B returns. The fixed 20-entry metadata
cache uses 83,604 bytes; 372 parser/cache/UI checks and 65 encrypted world
scenarios pass. Query truncations, trailing bytes and nonfinite map points are
rejected transactionally. Timed countdowns, reputation objectives, reward-item
inspection and abandonment remain pending. The live character status message
now stays within the SDK's 16 text rows.

Actual `native-quest-journal-objectives.png` shows the completed 10/10 objective
with a clean title. `native-quest-journal-description-first.png` is the earlier
readable description capture with an observed trailing-header bug, subsequently
fixed. `native-camp-return-passed.png` is the return/reconnect scene. The return
checker compares the final settled reward state before logout: quest-complete
and player/inventory updates arrive in separate packets. The first comparison
used the immediate quest-complete frame and correctly exposed that timing gap.

Xboxnight now has level2, 324 XP, 54 copper, 79 HP, eight inventory records and
both quests 783 and 7 rewarded, saved inside the Abbey. Xboxer remains at its
prior inn position with level2, 768 XP, ten copper and eleven inventory records.
No earned state was reset. The complete quest was tested across interrupted
runs; a single continuous combat/quest/death run remains unproven.

## GPU texture compression

WXP v5 and the NV2A renderer now support DXT1/DXT5 world textures and mip chains.
Existing RGBA v4 packs remain readable. Host repacking retains exact vertex,
index, collision and placement data; animated avatar/NPC packs stay in their
current format. The pinned host-only stb_dxt encoder and notices are vendored.

All five staged world packs shrink from **471,131,438 to 253,173,554 bytes**
(46.3%). Their unique texture payloads shrink from 253,838,920 to 35,881,036
bytes. Original RGBA packs remain in `build/rgba-baseline`; independently checked
compressed packs and receipts are in `build/dxt-world-final`. The four indexed
map packs shrink from 422,359,042 to 238,570,978 bytes. These measurements do not
establish full-world stock-disk feasibility; repeated geometry is still large.

The first normal capture, `native-dxt-normal`, has 1,346 samples, 38,072 KiB
minimum free memory and 33/34/42/298 ms p50/p95/p99/max. This is 5,284 KiB more
headroom than the preceding journal normal capture, with the same saved character
and scene; startup/capture windows differ. No asset or region errors occurred.
Actual screenshots show the textured interior and animated equipped player.

`native-dxt-camera` passes the full camera-sweep checker: heading and pitch,
collision, no translation, restored view and complete avatar. Its 1,156 samples
retain 38,072 KiB minimum free; the moving-view interval has 33/37/65 ms
p50/p95/max. Startup-inclusive timings are 33/40/50/309 ms. These are emulator
measurements from injected input, not physical controller or Xbox acceptance.

All eleven CTest suites pass, including 340 runtime boundary/allocation checks
and 187 independently decoded compressed-texture checks. Twenty-two optimizer
integration checks cover source truncation, output protection, legacy packs,
non-square textures and rejection of character packs. Native builds retain only
the previous upstream compiler/linker warnings. The XISO is now 289,669,120 bytes;
the XBE is 1,761,280 bytes. Full Vanilla content, UI and performance gates remain.

The final replay-disabled build, `native-dxt-final-normal`, has 1,550 samples,
38,068 KiB minimum free and 33/34/42/302 ms p50/p95/p99/max, with no asset, region
or packet failures. Normal/appearance checks and all 127 source/14 output hashes
pass. Its screenshot, telemetry and receipt are retained in `build/evidence`.

## Controller spellbook and persistent bindings

Start -> White opens the spellbook. D-pad browses/pages; A chooses an active
ability. The binding view accepts the trigger/button combination or left/right
navigation, shows the current action and requires confirmation. X in that view
reviews clearing the control. B cancels. It follows the saved controller mapping
and supported warrior stance bars, and invalidates a stale confirmation.

The catalog was built from the supplied archives: 22,357 Vanilla records,
2,459,286 bytes on disc. Only its 44,714-byte ID index and a fixed 57,344-byte
learned-record cache plus 1,024-byte display index are resident. One metadata
record loads per frame. All catalog records pass the runtime loader. Hidden
abilities and trade child spells are excluded from the top-level list; active
abilities sort before passives. The initial visual capture exposed internal
entries, leading to these filters. Its screenshot is retained for comparison.

All twelve host test suites pass; 1,016 spellbook checks cover metadata bounds,
low-memory failure, sorting/filtering, passive assignment, confirmation changes,
action encoding and transactional truncated action lists. All 68 encrypted
world scenarios pass, including set/clear/restore and malformed binding packets.
The server gives no successful edit acknowledgement; local state updates after
the send, and reconnect provides the authoritative action list.

`native-spellbook` completed assignment, reconnect, clearing and a second
reconnect: 4,599 samples, 37,816 KiB minimum free, 33/35/41/257 ms
p50/p95/p99/max. It preserved position, XP, level, money and inventory. The first
host checker compared the first online frame before player/inventory packets;
it now compares the initialized stage-2 baseline before any edit. That first
failed report and raw capture are retained. The filtered follow-up also passes
the complete flow. Final-build measurements are recorded in the overnight handoff.

Spell descriptions, icons, cooldown display, talents, profession recipe sublists,
all class/form bars and full spell targeting remain. Native tests use injected
controller samples; physical controller and original Xbox validation remain open.

The final assignment/reconnect/clear/reconnect capture, `native-spellbook-final`,
passes with 4,025 samples, 37,820 KiB minimum free and 33/35/45/313 ms
p50/p95/p99/max. Its actual screenshots show the ten visible warrior abilities
and confirmation view. Original progress, position and the empty test binding
were restored and verified after reconnect.

The normal replay-disabled build, `native-spellbook-normal`, passes normal,
avatar and all 134 source/15 output hash checks. Its 1,536 samples retain
38,012 KiB minimum free with 33/34/37/245 ms p50/p95/p99/max, including startup.
No asset, region, spellbook or packet failures occurred. These are emulator
measurements, not physical Xbox performance. XBE: 1,765,376 bytes; XISO:
292,093,952 bytes. Screenshot, telemetry and receipts are in `build/evidence`.

## Automatic avatar profile selection

The client now selects prepared avatar files from all seven server appearance
fields when entering a character. One profile remains resident; old GPU/cache
allocations are released before replacement. Exact metadata identity is checked,
and failed profiles are attempted once per appearance/connection. Missing assets
cannot leave the previous character's model on screen.

Sixteen default race/sex profiles with the existing five-item test catalog are
prepared and runtime-verified: 32,643,410 bytes total, outside source control.
The helper writes short deterministic filenames and refuses existing output
directories. All twelve host suites pass, including 383 avatar checks covering
switches, missing/misnamed profiles, retry bounds and allocation failure.

The first native `native-avatar-switch` capture created Xboxdawn, a female Human
warrior, through controller input, entered her world, then returned to Xboxer.
Character/appearance checkers pass: three profile loads, male/female/male drawing,
no selection errors, and original position/progress preserved. This run retained
37,976 KiB minimum free across 3,470 samples, with 33/37/48/629 ms
p50/p95/p99/max. The largest stall occurred during entry into the second scene;
smooth transitions and full-game 30 fps remain unproven. Actual creation and
female-avatar screenshots are retained. Input was injected, not a physical pad.

The other fourteen race/sex profiles currently have host validation only. Their
starting zones, full equipment catalogs, cosmetics and class mechanics remain
separate coverage work. The final normal-build measurements are in the overnight
handoff; this is still an incomplete development port.

The final replay-disabled `native-avatar-select-normal` capture passes normal,
avatar and 135 source/47 output hash checks: 1,576 samples, 38,004 KiB minimum
free, 33/34/36/352 ms p50/p95/p99/max, including startup. It loads the correct
profile once with no asset, profile, region or packet failures. The XBE is
1,769,472 bytes and the XISO is 324,796,416 bytes. The second switch capture also
passes, with 34,600 KiB minimum free and 34/40/66/590 ms timings; more NPCs and
cache data were resident. These remain emulator measurements, not hardware tests.

## Live controller action prompts

Holding LT, RT or both now shows the current layer's eight assignments. The HUD
uses the same saved controller mapping and supported stance resolver as gameplay;
it displays cached spell/item names, empty slots and unavailable action types.
It responds to live assignments and clearing. Health/target text moves above the
panel, which is suppressed by menus, dialogs, inventory, spellbook and death UI.
The fixed attack hint has been replaced by a trigger prompt. Cooldown/resource
availability, icons and unsupported class form mappings still need implementation.

`native-actionbar` passes all three layers against a separately captured server
action bar, menu/inventory suppression, no unintended casts, unchanged progress
and position. It has 3,063 samples, 38,004 KiB minimum free and
33/35/40/357 ms p50/p95/p99/max. The actual right-trigger screenshot includes the
item binding. Its initial source receipt check detected a checker-script update
made after the build; native inputs/outputs were unchanged, and that failed report
is retained. Later builds include the updated checkers.

`native-actionbar-spellbook` passes assignment, panel update, reconnect, clearing,
panel update and another reconnect: 4,926 samples, 37,812 KiB minimum free and
33/34/36/357 ms. Displayed binding telemetry checks both updates; the original
empty binding, progress and position were restored. Source/output hashes matched
136 files/47 outputs at capture. Tests inject controller samples, so physical
controller and original Xbox verification remain separate.

All twelve host suites pass, including 1,079 spellbook/action checks. The final
normal build, `native-actionbar-final-normal`, passes normal/avatar and all
136 source/47 output identity checks. Its 1,560 samples retain 38,008 KiB minimum
free, with 33/34/36/343 ms p50/p95/p99/max. No replay, asset or packet faults are
present. XBE: 1,769,472 bytes; XISO: 324,796,416 bytes. Screenshots and raw evidence
remain under `build/evidence`. Full Vanilla and physical hardware gates remain.

## Black-button utility wheel

Hold Black for 350 ms to choose Bags, Spellbook, Quests or Settings with the
left stick/D-pad. Release Black or press A to open; B cancels. A short tap still
toggles bags. Input is captured through the closing frame, including sticks and
trigger actions; disconnected/unavailable states cancel without activation.

All thirteen host suites pass, including 60 utility state/timing/cancellation
checks. `native-utility` passes all destinations, quick tap, held-button capture,
modifier-button confirmation, cancellation, unchanged position/camera/progress
and no unintended cast. Its 3,210 samples retain 38,008 KiB minimum free with
33/36/41/348 ms p50/p95/p99/max. The actual wheel screenshot shows the four
destinations and highlighted selection. All 141 source/47 output hashes match
the capture receipt. Native samples are injected, not physical-controller proof.


The utility action-panel regression also passes all layers, menu/bag suppression,
unchanged bindings and no accidental actions:3,035 samples,min38,008KiB,
33/35/39/369ms p50/p95/p99/max. A real LT screenshot shows Attack/HeroicStrike.
The normal utility build passes normal/avatar and141-source/47-output receipts:
1,527 samples,min38,008KiB,33/34/36/347ms, no replay or faults. Saved progress is
unchanged. These are xemu guest timings; physical hardware remains untested.


## Continuous quest test and collision investigation

The first combined quest run, `native-quest-loop`, completed the delivery quest,
accepted/read the camp quest, killed ten kobolds, looted three items and accepted
14 Heroic Strike casts. Xboxdawn reached level two with143 XP. The automatic
return then hit a camp obstacle; the checker correctly fails reward/reconnect.
Its13,516 samples retain 32,300KiB minimum free with33/54/103/597ms
p50/p95/p99/max. No asset, region, profile or packet errors occurred. This is
partial evidence, not a completed continuous loop. Earned progress is retained.

The replay's half-yard breadcrumbs cut a tight obstacle corner. The revised
bounded trail stores every actual movement sample (up to4,096) and uses tighter
arrival/steering tolerances. Replaying844 recorded positions against the actual
host collision code returns to the camp anchor with zero blocked moves or
failures. The subsequent fresh native run also passes, as recorded below.

Walking collision now rejects triangles outside conservative floor/body-ray
bounds before expensive intersection tests. An8,000-query comparison against the
previous code at spawn, Abbey, camp and inn returns identical floor heights,
allowed/blocked moves and blocker identities (6,770 allowed,1,230 blocked).
Host CPU time was1,377ms before and564ms after; this is not an Xbox performance
measurement. All14 host suites pass, including4,242 trail boundary checks.
Native recovery and before/after walking checks follow separately.


The separate recovery `native-dawn-return` passes its return-only checker and
avatar checks:5,388 samples,min 32,740KiB,33/47/90/631ms p50/p95/p99/max.
It walked231.30 yards, turned in quest7 and reconnected. Server readback confirms
XboxdawnGUID3 at level2,313 XP,49 copper and10 inventory entries in the Abbey.
This continues earlier earned objectives and is not a fresh continuous test.
The146-source/48-output receipt passed at build; the later tighter breadcrumb
steering change belongs to subsequent builds. This run uses a supplied route.

## Fresh continuous quest loop â€” 04:44 PDT, September 16

`native-rise-quest` passes all 17 quest-loop checks and the avatar checks. The
new female Human warrior, Xboxrise (GUID 4), accepted/completed A Threat Within,
accepted and read Kobold Camp Cleanup, killed ten kobolds, landed fifteen
Heroic Strike hits, looted four items, returned on foot and completed the quest.
The run walked 717.81 yards without teleports or server state edits. Logout and
reconnect preserved level 2, 308 XP, 51 copper, inventory and Abbey position.
Read-only server queries confirm both quest rewards and all ten objectives.

The capture contains 25,465 samples with 32,456 KiB minimum free memory and no
asset, region, appearance-selection or packet faults. The source/output receipt
matched 146 sources and 48 outputs at launch. Actual xemu screenshots and raw
client/server evidence are under `build/evidence/native-rise-*`.

Timing remains a material limitation: 33/131/208/599 ms p50/p95/p99/max. The very
dense return replay repeatedly switches between moving and idle, exposing
animation reload stalls. This passes the tested quest path, not the 30 fps goal,
all Vanilla coverage, physical-controller verification or an hour-long session.
Phase counters and a repeatable walking route are being validated next.

## Animation continuity and frame profiling â€” 04:56 PDT

The native phase counters pass their sum/boundary checks. On the repeated
365.61-yard Abbey/camp walk, movement collision averaged 0.85 ms, actor/avatar
work 6.70 ms and presentation 14.27 ms before the animation change. These are
guest wall times including waits, not isolated CPU or physical Xbox timings.

Avatar transitions now keep every component of the previous complete sequence
until the new one is ready. Idle/run data can remain in the existing 4 MiB cache,
without prefetch or increasing the two-load-per-frame allowance. Optional reuse
is discarded when memory is tight; allocation guards and byte budgets remain.

All 14 host suites pass, including 539 avatar checks covering complete fallback,
repeated idle/run switching, a third sequence, low-memory eviction and allocation
failure recovery. All 16 prepared race/sex profiles pass real-asset host loading
and idle/run/attack/death transitions. This does not establish native coverage of
all races, classes, equipment or appearances.

`native-cache-walk` passes walking, progress/reconnect, avatar continuity and
profiling checks. Across the same 2,568 walking frames, unavailable-avatar frames
fell from 177 to zero and total asset loads from 1,879 to 1,468. Maximum avatar
allocation during walking was 1,166,508 bytes. Actor-phase mean fell from 6.70 to
5.94 ms; NPC activity and host work are uncontrolled, so this is observational.
The complete 4,842-sample capture retains 32,908 KiB minimum free with
33/43/72/526 ms p50/p95/p99/max. No asset/region/profile/packet failures occurred.
Both comparison builds matched 147 source and 48 output hashes at launch.
Raw results, comparison JSON and an actual walking screenshot are preserved.

The complete equipment regression, `native-cache-gear-full`, also passes:
1,925 samples, 37,764 KiB minimum free, 33/37/54/367 ms p50/p95/p99/max.
Shield removal/re-equip, corresponding draw changes and saved reconnect all pass.
An earlier capture that missed the initial state is retained as failed evidence.

The final normal checkpoint, `native-cache-normal`, passes 11 neutral-session
checks plus avatar and profiling checks. It contains 2,580 samples, at least
60 seconds of received telemetry, 37,952 KiB minimum free and 33/34/35/337 ms
p50/p95/p99/max. Replay and scenario fixtures are disabled; original Xboxer
progress, gear, position and view match the earlier normal baseline. No faults
were recorded. The 148-source/48-output receipt matched at launch. A subsequent
host-checker tuple/list fix is explicitly recorded in validation-change.json;
all native source and all output artifacts still match that launch receipt.

Current output sizes: `default.xbe` 1,773,568 bytes and `wowx.iso` 324,796,416 bytes.
The normal build is running in the dedicated 64 MiB xemu configuration. Full-game
coverage, sustained 30 fps, physical controls and physical hardware remain open.

## Text rendering â€” September 16, 05:25 PDT

The UI now draws from a bounded texture atlas instead of issuing many rectangle
clears for each glyph. It retains the existing font/grid and allocates 154 KiB
once. A legacy fallback uses the same bounded text layout. All 15 host suites
pass, including 161 font layout, scrolling, atlas and vertex-boundary checks.

New presentation sub-timers show the walking baseline spent 11.10 ms waiting for
the 3D scene and 2.82 ms submitting/completing text on average. The texture font
reduces that text interval to 0.51 ms on the same 2,568-frame walking route; scene
wait remains 10.99 ms. The complete walking replay still passes route, saved
reconnect, continuous avatar and timing checks. Its 4,816 samples retain
32,920 KiB minimum free, with 33/39/64/565 ms p50/p95/p99/max. These are xemu
observations with uncontrolled host/NPC activity, not hardware benchmarks.

Both textured and forced-legacy fonts pass the complete controller utility
replay, opening bags, spellbook, journal and settings without leaked input or
changed progress. Each has exactly 942 wheel frames with 105 glyphs. Mean text
submission/completion is 0.62 ms textured versus 2.91 ms legacy. Actual screenshots
of both wheels and native menu rendering are saved. The textured utility run has
3,991 samples, 37,792 KiB minimum free and 33/34/35/346 ms p50/p95/p99/max; legacy
has 3,795 samples, 37,948 KiB minimum free and 33/35/40/396 ms. Both launch receipts
match 155 sources and 49 outputs. No font or asset errors occur.

The converter now has a read-only `--map-info` inspection mode. The supplied
Vanilla WorldMapArea table has 51 records and eight fields; newer optional fields
must not be assumed from upstream layouts. This supports upcoming Back-button
map work; a native map screen is not implemented yet.

The final `native-font-normal` checkpoint passes normal-session, font, avatar,
phase and receipt checks: 2,688 samples, 37,788 KiB minimum free, and
33/34/35/342 ms p50/p95/p99/max. All five development fixtures are disabled and
saved character state is unchanged. The complete 155-source/49-output receipt
matches current source and outputs. Normal xemu remains running with texture
fonts. XBE size is 1,781,760 bytes; XISO remains 324,796,416 bytes.


## Controller world map â€” September 16, 05:52 PDT

- Added a generic exporter for all 51 Vanilla WorldMapArea records and their
  WorldMapOverlay images. The actual source table has eight core fields.
  Base image layout is 4x3 256-pixel tiles; the 1002x668 coordinate rectangle is
  reduced to 501x334 in a 512-square texture. Small source overlay edge tiles
  (encountered in Silithus) retain transparent padding, with no out-of-bounds read.
- Native Back map uses server exploration bits (64 fields beginning at 0x457),
  a player marker, 1x/2x zoom, pan, zone paging and continent/current-zone selection.
  It consumes all gameplay input, including held modifiers on close. Settings
  retains Back logout/reconnect. Current map selection uses the smallest matching
  coordinate rectangle; overlapping-zone selection still needs wider field testing.
- One texture plus four vertices requests 1,048,672 GPU bytes; measured allocation
  includes page alignment. Loading uses a bounded row buffer, at most 128 overlay
  records, an 8 MiB free-memory guard, allocation rollback, strict file lengths,
  finite bounds and contiguous offset validation. Closing releases the texture.
  New exploration fields add 256 bytes per entity to the bounded entity stores.
- All 16 host suites pass, including map file/input/low-memory/failure tests and
  exploration delta/transaction tests. The native loader's host harness also
  decoded all 51 prepared maps with no/all explored bits: 155 checks passed.
- `native-map` passes all 16 map replay checks plus avatar, timing and font checks:
  6,332 samples, 36,424 KiB minimum free, 33/34/35/351 ms p50/p95/p99/max.
  1,982 map frames; six intentional loads; Elwynn, Eastern Kingdoms and Alterac
  selected; three explored Elwynn overlays; marker at approximately 46.5%,62.2%.
  No movement/cast leak, saved progress unchanged through logout/reconnect, and
  map allocation returns to zero on close. Timing is the emulator guest clock.
- Telemetry WXTL adds 14 map counters to WXTK: 228 words / 912 bytes. Older wire
  formats remain readable. Receipts include MAPS.WMI and all 51 Z*.WMP files.
- Initial native receipt matched 161 sources and 101 outputs at launch. Afterwards
  the host checker corrected its expected vertical pan direction (stick up means
  smaller map v); main.c gained an explicit zero initializer to silence a warning.
  A fresh verification build is underway with both changes and a new receipt.
- Visual review is currently obstructed by a Windows update restart prompt, which
  the UI tool identifies as PickerHost but exposes with no targetable window.
  No OS security/update policy was altered. `shutdown /a` reported no shutdown
  in progress, so a 06:00 restart has NOT been confirmed deferred. Native automated
  tests continue independently. This is not a claim of finished visual review.
- Map quest/party POIs, cursor/world overview/dungeon floors remain open, alongside
  broader Vanilla and hardware gates. Full conversion is not full gameplay.


### Latest map verification and visual limitation

The fresh build's main capture contains 4,149 samples (minimum36,424KiB,
33/34/35/367ms) and all1,982 map frames, six loads and saved reconnect. Its
200-second host timer ended14 neutral replay frames before replay_active cleared.
A600-sample tail from the same process confirms finalclosed/released/live state.
`native-map-verified-full.csv` preserves those two measured segments with an
explicit83,281ms guest-clock gap in `.segments.json`; it passes all16mapchecks
and avatarchecks but is NOT an uninterrupted capture. The earliernative-map
run is the uninterrupted completepassingtest. Originalsegmentfiles are retained.

The actual `native-map-continent-obstructed.png` shows the continent image and
controller prompts rendering correctly around the Windows update dialog. The
center/player marker is obscured; clean zone/overlay screenshots remain due.
A normal-control rebuild/capture is now running to verify released map memory,
unchanged Xboxer progress and the final source/output receipt.


### 05:59 final normal checkpoint

`native-map-normal` passes all12 normal checks plus avatar, phase timings, font
and exact161-source/101-output receipt checks. The2,349-sample capture covers
more than60seconds of received telemetry: minimum37,452KiB free and
33/34/35/365ms p50/p95/p99/max. Map is closed with zero texture allocation,
all five development fixtures are cleared, and Xboxer remains level2/XP768/
10copper/79HP at the inn. Allfour saved characters match the prior readback.
XBE:1,789,952bytes. XISO:405,405,696bytes. Owned normal xemu PID21712 remains
running; no build/recorder sessions remain. A private character-database dump is
saved at server/backups/characters-20260916-0559.sql. It was taken online with
single-transaction semantics; this is not a tested full-server restore package.
The scheduled Windows06:00 restart remains unconfirmed deferred.

## Corpse recovery verified (06:28 PDT)

The `native-corpse` run passed all 16 acceptance checks plus avatar, font and
phase-timing checks. It contains 9,033 samples, with at least 32,620 KiB free.
Guest frame intervals were 33/35/52/597 ms (p50/p95/p99/max). The long frame is
still a measured streaming/loading stall; this does not establish sustained
30 fps across the game or original-hardware performance.

The freshly created Xboxspirit (GUID 5) took 60 damage from server combat,
released at the graveyard, received a 30-second reclaim timer, opened the corpse
map, logged out/reconnected as a ghost, walked 198.35 yards through collision,
reclaimed its owned body and reconnected alive. XP, level, money and inventory
count were preserved. The other four saved characters were unchanged.

Actual screenshots are `build/evidence/native-corpse-map.png` and
`build/evidence/native-corpse-passed.png`. They are unobstructed: the former
verifies Northshire exploration imagery, yellow player marker and red body
marker. The separately rendered corpse model and ghost visual effects remain
unfinished. Full combined quest/death stress coverage is also still open.

The corpse build receipt matched 167 sources and 101 outputs at launch. New
sustained-test tooling was added afterward, so that receipt is historical.
A 300-second walking/UI/reconnect smoke test is currently running before the
one-hour session. Normal controls will be restored after those tests. See the
latest `OVERNIGHT.md` checkpoint for current process and capture details.

### Sustained-test smoke passed; hour run pending

`native-soak-smoke` passed all 16 checks, with two complete laps and 731.22 yards
of walking. Every lap exercised the map, utility wheel, inventory, spellbook,
quest journal and settings, then retained progress after reconnecting. All
menus closed and the map texture was freed. The 10,325-sample capture spans
331,927 ms, with minimum 31,500 KiB free and 33/41/78/538 ms frame intervals
(p50/p95/p99/max). Avatar, font and phase-timing checks also pass. All 17 host
suites pass. The clean `native-soak-continent.png` verifies continent imagery
and player placement without the earlier modal obstruction.

The one-hour version has been launched with a continuous recorder. It is pending,
not a completed stability gate. It covers walking/UI/reconnect, not the required
crowded combat, forced network loss or physical-hardware workloads. Normal input
will be restored after that run; current process details are in OVERNIGHT.md.

## 07:10 endurance interruption and monitored retry

The first `native-soak-hour` capture is interrupted, not a one-hour pass. xemu
PID22124 was found absent at approximately07:01; its last captured sample was
at06:59:38, after1,044,335ms of the sustained scenario. It contains31,178samples,
six completed laps and2,539.64yards walked, with minimum31,300KiB free. No client
asset, region, map, avatar-selection or packet failures were reported. The
capture fails completion, duration, minimum-lap and returned-position checks.
The reason for xemu's exit is unknown: no matching Windows application error,
crash dump or xemu log error was found. No exit code was retained by that run.
OriginalCSV and emulator logs are preserved with interruption metadata.

Xboxnight's earned progress is intact, and its position was saved inside the
Abbey entrance. The retry uses the already-saved Xboxdawn at the original Abbey
fixture start, without resetting either character.

The recorder now optionally observes the actual xemu process handle, writes its
exit code/lifecycle metadata and ends after60seconds without valid telemetry.
Fifteen real process lifetime/identity checks pass; a separate explicitly
synthetic four-packet integration fixture verifies exit-code7 is captured and
validJSON is written. This synthetic fixture is not game evidence. The launcher
now reports state-driven fixtures correctly, and SOAK.BIN has an explicit XISO
build dependency.

`native-soak-retry` is now running and pending. Its launch receipt matches
171sources/102outputs. At18,564ms it is walking with XboxdawnGUID3, free32,684KiB,
no client failures and a3,600,000ms duration target. Automatic follow-up continues
through09:00PDT to allow completion and a verified normal-build restoration.
The prepared renderer optimization remains unapplied.


## One-hour endurance passed (08:12 PDT)

`native-soak-retry` passes all 17 endurance checks, plus avatar, font and phase
counter checks. The uninterrupted capture contains 107,864 samples and ends
normally with `terminal_scenario`, with no emulator exit. It covers 3,661,998 ms
(61.03 minutes), 23 complete walking/UI/saved-reconnect laps and 8,409.05 yards.
Each lap visits the zone/continent map, utility wheel, bags, spellbook, journal
and settings. Every lap releases the map texture; the final avatar is complete,
menus are closed and the saved starting position/orientation is restored.

Minimum free memory is 30,684 KiB (29.96 MiB), above the 8 MiB requirement.
End-of-lap free memory is 32,272 KiB initially and 31,916 KiB finally. After lap 7,
it varies within 31,772-31,928 KiB as resident scene contents change, without a
continuing downward trend across this workload. Longer/more varied workloads
remain necessary; this is not a universal leak-free claim.

Guest frame intervals are 33/40/68/1,084 ms (p50/p95/p99/max). The longest frame
is initial world-entry loading: 529 ms in streaming and 552 ms in actor/avatar
work. Walking also has 389-399 ms frames dominated by streaming. Sustained
30 fps is not established; xemu measurements are not original-hardware timings.
Detailed preceding-frame phases are in `native-soak-retry-long-frames.json`.

The launch receipt still matched all 171 sources and 102 outputs before normal
restoration. Actual screenshots are `native-soak-retry-walking.png` and
`native-soak-retry-passed.png`. A read-only server snapshot confirms all five
characters' earned progress is intact; Xboxdawn retains level 2, XP 313,
49 copper and 79 HP. Original interrupted-hour evidence remains preserved.

This pass uses client-injected controller samples. It covers walking, menus and
saved reconnect, not crowded combat, forced network loss, a combined quest/death
stress session or physical controllers/hardware. Normal-build restoration and
its separate neutral capture are underway. The full Vanilla port is incomplete.


### 08:17 verified normal checkpoint

`native-endurance-normal` passes all 13 normal checks plus avatar, font, phase
counter and exact 171-source/102-output receipt checks. Its 3,927 samples span
131,457 ms of received telemetry; the 180-second host recording includes launch
and boot time and ends normally at its duration limit. Minimum free memory is
37,444 KiB; guest intervals are 33/34/35/373 ms (p50/p95/p99/max).

All six development fixtures are cleared, the map has no allocation, and Xboxer
retains level 2, XP 768, 10 copper, 79 HP, 11 inventory items, equipment and its
saved inn position/view. A fresh read-only server snapshot confirms all five
characters match the end-of-endurance snapshot. Physical controller use remains
unverified; this is a normal-input neutral capture.

Actual screenshot: `build/evidence/native-endurance-normal.png`.
Current deliverables: `build/xbox/default.xbe` (1,798,144 bytes) and
`build/wowx.iso` (405,405,696 bytes), with private local game assets/test setup.
Dedicated xemu PID 10736 remains running; both recorders have exited. The private
online character dump at `server/backups/characters-20260916-0814.sql` is 75,192
bytes; restore has not been tested. The overnight heartbeat is now PAUSED after
this verified checkpoint. No claim of full Vanilla or hardware completion.

### 09:50 original Vanilla interface foundation

The renewed full-parity objective is tracked in `docs/PARITY.md`; the existing
development heartbeat is active again, every 30 minutes with no overnight cutoff.
The 08:17 checkpoint is preserved under `build/checkpoints/20260916-0817`, with
all 171 source bytes verified against its receipt.

Character roster, creation and controller keyboard now use the supplied Vanilla
FRIZQT__/MORPHEUS fonts, WoW logo, red buttons and frame border. A fixed 512x512
atlas and 2,048-quad queue request 1,376,256 bytes including GPU vertices. The
converter reads local archives; private artwork remains outside source control.
Solid-color sampling and horizontal border rotation were corrected after actual
xemu inspection. Host tests cover malformed packs, clipping, UV orientation,
queue limits, memory headroom and allocation rollback: 174 checks plus eight
against the real prepared pack pass.

`native-vanilla-ui-fixed` passes UI/avatar/profile/text checks and preserves
Xboxer's saved progress, position and equipment. Its 6,962 samples have a minimum
35,892 KiB free; maximum submission is 367 quads with zero UI/asset failures.
Guest frame intervals are 33/34/35/410 ms (p50/p95/p99/max), not hardware timing.
The repeated visual run's actual roster, creation and keyboard screenshots are
saved as `native-vanilla-ui-visual-{roster,create,keyboard}.png` under evidence.
That run's final checks and normal-build restoration are still in progress.

These are original-art controller screens, not completed PC layouts. Account
entry, realm selection, character previews and appearance customization remain
unfinished. The actual title M2 was inspected: 7,970 vertices and 57 bones, with
portal, sky, cloud and effect materials that still need supported rendering.

### 10:14 native account/realm flow passed

`native-login-flow` passes all 16 account-flow checks plus avatar, UI allocation,
font, profiling and exact 187-source/104-output receipt checks. The injected
controller replay opens/cancels the account keyboard, handles a rejected password,
authenticates, displays the live realm list, cancels it, logs in again, selects
the realm and character, and enters Xboxer's saved world. The 300-second host
capture contains 7,586 samples, ends normally and measures a minimum 35,744 KiB
free. Guest intervals: 33/34/37/373 ms (p50/p95/p99/max). No asset/UI failures.
All five saved server character records match the earlier endurance snapshot.

Realm parsing now covers all 255 count entries and 65,535 packet bytes, including
empty/offline lists and the correct Vanilla build-flag layout. Host validation:
242 realm boundaries/categories, 326 controller/password-masking checks, 24 auth
scenarios, all 20 CTest suites, 68 encrypted world cases and 10 character cases.
The three category assertions were added after the full-suite run and pass in
the targeted realm run. Authentication cancellation is also tested against a
stalled fake peer. The private fixture's credentials never enter telemetry.

Default startup now opens the account menu with an empty password. Existing
gameplay fixtures and explicit `-AutoLogin` retain the development world shortcut.
Actual account/keyboard/realm/world screenshots were inspected. The account
input-box height has since been corrected so its borders render; neutral world
and clean manual-menu verification of that final build are underway.

The original animated title background, PC screen layouts, model previews,
remembered-account settings, DNS and the remaining matrix are still unfinished.
This passes a controller login workflow, not the full 1:1 port.

### 10:32 final login checkpoint

The corrected `native-login-normal-retry` completed its full recorder duration:
3,986 samples, min 35,744 KiB free, guest intervals 33/35/38/359 ms. All normal,
avatar, UI, profile, font and 187-source/104-output receipt checks pass. Xboxer's
saved progress, position, view and equipment are unchanged. All replay fixtures
were disabled. `native-login-menu-retry` also passed 3,600 manual-menu samples.

The restored default `native-login-menu-final` completed with no emulator exit:
2,797 samples, min 39,292 KiB; guest intervals 33/34/34/35 ms (p50/p95/p99/max). Manual menu, original-art UI, profile and exact receipt checks pass.
Actual screenshot inspected: correct field borders, empty password, no auto-login.

The earlier `native-login-normal` was stopped by the agent eight seconds early;
its failed lifecycle remains explicit. The ensuing first menu recorder could not
bind the occupied telemetry port. Both attempts are retained and excluded from
stability acceptance. Replacement recordings above completed independently.

Checkpoint `build/checkpoints/20260916-1032/` preserves the verified XBE, XISO,
receipt and all 187 source files with archived bytes checked against that receipt.
Stock-hardware/controller verification, complete PC layouts, the authored title
scene and the remaining parity matrix remain open. Continue development.

### 10:33 final login checkpoint

The corrected `native-login-normal-retry` completed its full recorder duration:
3,986 samples, min 35,744 KiB free, guest intervals 33/35/38/359 ms. All normal,
avatar, UI, profile, font and 187-source/104-output receipt checks pass. Xboxer's
saved progress, position, view and equipment are unchanged. All replay fixtures
were disabled. `native-login-menu-retry` also passed 3,600 manual-menu samples.

The restored default `native-login-menu-final` completed with no emulator exit:
2,797 samples, min 39,292 KiB; guest intervals 33/34/34/35 ms (p50/p95/p99/max).
Manual menu, original-art UI, profile and exact receipt checks pass. Actual
screenshot inspected: correct field borders, empty password, no auto-login.

The earlier `native-login-normal` was stopped by the agent eight seconds early;
its failed lifecycle remains explicit. The ensuing first menu recorder could not
bind the occupied telemetry port. Both attempts are retained and excluded from
stability acceptance. Replacement recordings above completed independently.

Checkpoint `build/checkpoints/20260916-1032/` preserves the verified XBE, XISO,
receipt and all 187 source files with archived bytes checked against that receipt.
Stock-hardware/controller verification, complete PC layouts, the authored title
scene and the remaining parity matrix remain open. Continue development.

### 10:59 original title model and native transition

The Xbox renderer now loads the supplied `UI_MainMenu.m2` scene through a bounded
private `TITLE.WXB` pack:7,970vertices,57bones,22batches and20mesh textures. It
retains79independently looping bone/alpha/weight tracks instead of resampling
all animation into one short clip. Requested CPU+GPU storage is4,142,740bytes,
with an8MiB total scene cap and an additional8MiB free-memory requirement.
Real alpha/additive materials, original texture wrap modes and camera framing
are active. Missing particles/lighting/fog and exact PC widgets remain explicit.

The first screenshot exposed over-wide projection. Corrected Classic M2 FOV
conversion produces the preserved `native-title-framing.png` and later animation
frame. The completed capture has3,031samples,35,212KiB minimum free memory and
33/34/34/38ms guest frame intervals. Menu/UI/backdrop/profile and195source/
105output receipts pass. This verifies native rendering, not exact PC fidelity.

`native-title-login` completes330seconds normally with8,708samples. It passes
all16login checks, title allocation/release, avatar, UI, text, profiling and
receipt checks. Rejection, cancellation, retry, roster and saved-world entry
work with the title active. Scene allocation becomes zero at the roster/world;
all five saved character records match the prior checkpoint. Minimum35,212KiB;
guest intervals33/34/37/476ms. The longest stalls are at world entry, where actor
loading remains a performance task. Full-game smoothness is not established.

Host validation:215boundary/interpolation/skinning/allocation checks, a multicycle
actual-asset finite test,9timestamp comparisons against pinned WoWee/GLM (maximum
error0.000001431), and all21CTest suites after rebuilding all host targets pass.
An initial text-check invocation used the wrong --legacy backend argument; its
failure is retained as native-title-login-text-wrong-mode.*. The correct check
of the recorded default GPU text backend passes; no renderer change was needed.

Read-only particle preparation found a pinned-loader Vanilla field-width bug:
raw emitter types at0x2a are uint16values1/2, but the later-layout byte read at
0x29 reports0. Evidence and an inspector are retained for the next effects work;
no fake substitute particles have been added. Neutral world verification and
restoration of the clean manual-menu build are in progress.

### 11:05 title checkpoint preserved

The restored default `native-title-final` capture completed normally: 3,084
samples, at least 35,216 KiB free, no title/UI/asset failures and no automatic
login attempts. Backdrop, manual menu, UI, profiling and exact build-identity
checks pass. The actual title screenshot is `native-title-final.png`.

The separate normal world run passed all 13 neutral-session checks, preserving
Xboxer's progress, equipment, position and view: 2,997 samples, 35,724 KiB minimum
free, guest intervals 33/35/37/345 ms. All replay fixtures were disabled and the
title was never allocated on that explicit development auto-login path.

`build/checkpoints/20260916-1105/` preserves default.xbe (1,835,008 bytes), the
private XISO (408,616,960 bytes), receipt, and a source archive. All 195 archived
source hashes and both archived binary hashes match the tested receipt. The
source archive includes current documentation and notices, with game content
and credentials excluded. The clean manual-menu xemu is left running.

Full PC parity remains in progress. Title particles, authored lighting/fog,
original widget workflows, character previews and the remaining gameplay matrix
are open. Original Xbox hardware and physical-controller tests remain separate
release gates. Autonomous development remains active.

### User workflow adjustment

The user requested more implementation between test cycles. Development now uses
larger related feature batches, quick targeted checks during coding, and combined
xemu validation at meaningful milestones. Repeated per-change recordings and
archives are no longer the default. Release, memory and hardware gates remain.

### 11:43 title effects/lighting batch complete

Native title scenes now include28 bounded emitters,4 authored lights, animated
material colors,25 prepared textures and the shared camera/projection foundation
for character previews. The local Vanilla256 adapter corrects the pinned parser's
uint16 emitter-type field without changing upstream. TITLE.WXB is2,594,616bytes;
requested scene memory is5,540,606bytes, with a2048-particle fixed pool.

Exact pose/light caching removes redundant skinning and lighting while preserving
frame-by-frame animation. Host forced-recomputation comparisons pass. Initial
native title timing was67ms median; after these optimizations the final title
measures33/34/34/35ms p50/p95/p99/max, with6ms median scene-update work.
This is xemu timing, not physical-Xbox performance.

The combined native-effects-final run completed5,315samples and passed effects,
login/error/cancel/realm/roster/world, profiling and all198-source/105-output
identity checks. Peak1,391particles; no pool drops or runtime failures; at least
33,916KiB free. World samples measure33/34/36/384ms: loading stalls remain.
All five saved characters match the read-only database baseline, and Xboxer's
progress/equipment/position match the prior normal-session capture.

The first effects run exposed a one-frame title reload during the lobby/world
handoff. Its failing checks are retained. The final build gates title allocation
out of the world-connection phase and loads it exactly once in the combined run.
All scene/effect buffers release before world entry. Follow-up runs addressed
these measured performance/lifetime failures; no broad suite or endurance cycle
was repeated. The new targeted boundary/adapter/cache checks pass.

Manual launch restored MENU with the exact accepted XBE and198source hashes.
No additional manual-menu recording was run; its actual screenshot was inspected:
build/evidence/native-effects-manual.png. Current project xemu is49428, no recorder.
The private checkpoint isbuild/checkpoints/20260916-1143; binaries, receipt, source/docs archive
and verification are retained. XBE1,847,296bytes; XISO409,075,712bytes.

Human and Dwarf original selection scenes prepared successfully in
build/ui-prepared/previews (host readback/reference comparison only; not native
character previews). Orc/NightElf need additional emitter flags/type support;
Scourge has an animation-key case; Tauren fails current format/budget validation.
See build/evidence/preview-preparation.json and individual converter logs.

Full PC parity remains incomplete: advanced particle motion/sprite timing,
fog/camera tracks, original widget workflows and live character previews, the
in-world HUD, missing gameplay systems and broad world coverage remain open.
Input was replayed internally; physical controllers/hardware remain unverified.

### 12:12 character-preview batch complete

The roster now renders the selected character with server appearance, equipment
and hide flags. Creation renders all 16 default race/sex models. Right stick
rotates and zooms; D-pad retains form/list navigation. Original Human and Dwarf
scenes use authored camera/depth and animated attachment-0 stand positions;
Gnomes share the Dwarf scene. WXB v3 carries this placement while v1/v2 remain
readable. One preview profile and one background stay resident and release
before world entry. The previous world avatar also releases on lobby entry.

The single combined native-preview run completed 6,781 telemetry samples;
4,047 fully drawn preview samples covered all 16 identities. All 12 preview and
4 profiling checks pass, plus all 202 source/107 output identities. No runtime
failures or missing equipment in the exercised roster. All five saved database
characters remain identical to preview-server-before.tsv; Xboxer returned with
XP 768, money 10 and saved position. No character was created or deleted.

Ready preview timing is 33/34/47/68 ms p50/p95/p99/max. Including asset-loading
transitions it is 33/34/50/906 ms. Returned world timing is 33/36/48/370 ms.
Minimum free memory is 32,012 KiB in the stock 64 MiB xemu configuration.
Synchronous scene/profile reads still stall selection; these figures are emulator
wall times, not physical Xbox performance. Internal pad replay does not validate
physical controller transport.

Targeted host validation passes: 392 backdrop boundary/allocation checks, 77
preview snapshot/input/real-asset/lifetime checks, and the existing character
controller suite. No broad suite or endurance run was repeated. Actual inspected
screenshots: native-preview-roster.png, native-preview-create-human.png and
native-preview-create-troll.png. Title/UI artwork is original, but full PC parity
is incomplete: Human scene edge/near-plane gaps and caption overlap are visible;
creation starter outfits/customization and four racial backgrounds remain open.

The next implementation batch should combine starter outfits/customization,
remaining racial scene support and framing/clipping, with incremental bounded
loading to remove the measured 906 ms selection stalls. Do useful implementation
before the next combined acceptance run. Do not port zones individually.

Raw scene diagnostics are in preview-remaining-scenes.json: Orc adds emitter
flag 0x8; NightElf uses 0x8, 0x100 and 0x1000; Scourge has a zero-duration global
sequence; Tauren still needs precise format/budget failure diagnostics. These
files identify unsupported cases; they are not permission to discard effects.
Private raw models are in build/ui-source; never commit those assets.

Manual launch restored LOGIN.BIN=MENU with the exact accepted XBE and 202 source
hashes. The project xemu PID is 69264, no recorder is active. Current receipt:
native-preview-manual-receipt.json. Acceptance receipt: native-preview-receipt.json.
Launch fixtures/entropy and therefore ISO differ; sources/XBE are identical.
XBE: 1,851,392 bytes; XISO: 414,842,880 bytes. Checkpoint: build/checkpoints/20260916-1212.
Keep wowx-overnight-development ACTIVE, with no old morning cutoff. The user
continues to authorize autonomous development toward complete PC functionality.

## 2026-09-16 â€” physical Xbox test handoff

The user requested an actual-hardware walkthrough during the next preview batch.
Console: modded, XBMC, FTP 192.168.50.85. PC Ethernet: 192.168.50.156/24,
Private profile; NordLynx also present. No FTP password requested or transfer
performed. No console/BIOS/dashboard changes made.

Prepared dist/WOWX-hardware-20260916/WOWX: 108 files, 414,119,256 bytes. Every
asset and the XBE match checkpoint 20260916-1212; all copy hashes verified.
Overrides are manual MENU, no injected input/scenario, LAN auth address 3725,
blank packaged password, fresh 4112-byte entropy. Password remains in the private
server/local-credentials.json. Source manifest/notices/licenses and a hardware
guide accompany the package. This is a limited playable client, not full parity.

scripts/prepare-hardware.py reproduces staging without touching build/xbox or the
checkpoint. scripts/hardware-network.ps1 configures/restores scoped Windows TCP
forwarding 3725/8086 and the realm's advertised IP. Its PowerShell parse and Check
mode passed; local server endpoints reachable. Enable/Restore require Windows
administrator rights and were NOT executed by the agent. User is instructed to
run Enable in elevated PowerShell, wait 25 seconds, FTP and launch default.xbe
from XBMC. It saves prior realm settings in server/hardware-network.json. Do not
reset/reconfigure the server or stop it during this physical playtest. If that
state file exists, respect the hardware LAN configuration. No hardware success
has been reported yet. Existing telemetry still targets 10.0.2.2, so this exact
checkpoint uses on-screen diagnostics/photos and server logs for hardware proof.
Guide: docs/HARDWARE-TEST.md. Owned xemu was already closed by the time of setup;
server launcher started/reused realmd PID 791 and mangosd PID 852 in WSL.

Unfinished preview batch stays separate from the hardware package:
- New fixed OUTFT table reader/exporter (OUTFITS.WXO, 82 rows, 8216 bytes), draft
  equipment integration and modular per-race/sex starter item catalogs.
- All 16 avatar profiles cooked and host-verified into the NEW directory
  build/avatars-outfits-20260916. Not staged or native-tested yet.
- Corrected raw color table offset to 0x54, added pinned first-color crosscheck;
  increased backdrop bone limit 128 -> 256 for the 244-bone Tauren scene.
- Zero-duration globals reduced to a stationary key, matching pinned sampling.
  Scourge converted successfully before the RGB fix; regenerate all scenes/title.
- Raw Orc/NightElf emitter flags remain unsupported; external flag descriptions
  conflict. Do not silently discard them or call their semantics proven.
- Incremental loading, class outfit coverage tests, RGB/cap host checks, full
  combined native verification and new checkpoint remain to do. Current new
  sources are NOT the executable in the physical-test package.

Resume useful offline implementation while awaiting hardware observations; keep
this stable transferred package and any active physical server session intact.

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

Candidate archive: build/candidates/20260916-preview-outfits (210 source hashes, 110 output identities, both archived binaries verified). VALIDATION.txt explicitly marks native acceptance pending. Ongoing physical-test package is unchanged.


## September 16 evening candidate: all racial preview backgrounds

The remaining Orc/Troll and NightElf backgrounds now export and load in the
native C preview path. This is a development candidate, not a new accepted
hardware checkpoint. The user's physical-test folder and shared server/account
were left untouched; no xemu session or native replay was launched.

The build-5875 loader uses different particle file/runtime flag numbers. Local
inspection established emitter-local versus released motion, opt-in bone scale,
sphere-only Z-up and bone-oriented XY quads. Implemented those behaviors with
fixed storage and no frame allocations. Unlit bit-8 combinations used by the
scenes are accepted; standalone bit 8 and other unknown cases stay rejected.
The evidence and remaining limitations are in docs/PARTICLE-FLAGS.md. Existing
WXB layouts and the 2048-particle capacity remain unchanged.

- Orc: 11 emitters, 4 lights, 2,718,824 resident CPU/GPU bytes.
- NightElf: 12 emitters, 4 lights, 4,815,812 resident CPU/GPU bytes.
- Both geometry exports match pinned WoWee/GLM at nine timestamps (maximum
  error 0.000031). No emitters or ribbons were omitted from these scenes.
- Host simulation advances all seven prepared scenes for 30 seconds each;
  every emitter appears, with finite output and zero pool drops. Title peak
  1385; Orc 344; NightElf 147; Scourge 36; Tauren 42. Human/Dwarf have no emitters.
  These are simulated host results, not wall-time/native performance figures.
- Targeted boundary/behavior checks: 550 backdrop and 12 adapter checks pass.
  Actual-asset preview coverage: 514 checks, all 80 starter outfits and all eight
  racial selections, including their authored stand positions and release.
- Native nxdk compile/XBE/XISO succeeds; only the known linker merge warning.
  XBE 1,855,488 bytes; XISO 434,634,752 bytes. Identity receipt verifies 210 source
  hashes and 112 output identities. Normal MENU/live-input fixtures retained.

Evidence: particle-cook-{Orc,NightElf}.log, particle-scenes-host-simulation.log,
particle-scenes-preview-host.log, particle-scenes-host.log,
particle-scenes-native-build.log, particle-scenes-identity.json and
particle-scenes-untested-receipt.json, all in build/evidence.
Candidate archive: build/candidates/20260916-preview-scenes. Native acceptance
is pending. The next combined 600-second replay uses check-preview.py --outfits
--all-scenes and check-backdrop.py --world-transition, with fresh read-only
server baselines. Do not run it against the shared account during the user's
physical hardware test. No hardware success has been reported.

Remaining work includes near-plane/framing issues, appearance customization,
precise emission shape/timing/sprite behavior and full lighting/fog/sorting,
plus the full-game gaps in PARITY.md. New native loading timings, screenshots
and physical controller results remain unmeasured; the prior 906 ms selection
stall is still the latest native measurement. No full-parity claim is made.


## September 16 character-management candidate

Implemented controller character deletion and recoverable world-login errors.
This candidate includes the earlier starter-outfit/racial-scene work; native
acceptance remains pending. The verified checkpoint 20260916-1212 and the user's
physical-test folder are unchanged. No shared server process, account, character,
network setting, credential or private source asset was modified by this batch.

- White opens a deletion dialog for the selected GUID/name. The on-screen
  keyboard requires DELETE; Done returns to the dialog and a separate A press
  submits. B cancels. Reordering preserves the target; disappearance or a changed
  name invalidates the confirmation. Preview selection follows that GUID.
- Real CMSG_CHAR_DELETE sends the full 64-bit GUID. Terminal Vanilla replies
  0x39/0x3a/0x3b refresh the roster; success additionally requires the GUID absent.
  An incomplete reply, disconnect, timeout or inconsistent refresh reports
  "Deletion not confirmed" and never automatically retries the request.
- SMSG_CHARACTER_LOGIN_FAILED codes 0x3e through 0x44 now re-enumerate the roster
  and show the specific error, waiting for a new selection. Unrelated character
  replies cannot be consumed as another operation's expected reply.
- Host results: 352 character-data checks, 395 controller/packet checks and 517
  actual-asset preview checks pass (all 80 outfits remain covered). Forty
  encrypted synthetic character scenarios pass, including the 15-second delete
  deadline, malformed/duplicate/out-of-order replies, last-character deletion,
  refusal, transfer lock and all seven recoverable login errors. Two existing
  automatic-world-entry fixture regressions also pass. These fixtures bind only
  loopback, use a public fixture key and do not read the real account config.
- Native nxdk build passes: default.xbe 1,859,584 bytes; XISO 434,700,288 bytes.
  Warnings are two pre-existing font.c indentation warnings plus the known
  linker .edata merge warning. Normal MENU/live-input staging is retained.
- No new xemu run, screenshot, frame-time, measured free-memory or physical
  controller result is claimed. Fixed pools and the >=8 MiB allocation guards
  remain; new-candidate headroom still requires native measurement. The latest
  accepted preview minimum remains 32,012 KiB, with a 906 ms worst transition;
  those measurements describe the older checkpoint, not this candidate.

Evidence: build/evidence/character-management-{host-summary.json,protocol.log,
world-regression.log,native-build.log,identity.json,untested-receipt.json}.
Archive: build/candidates/20260916-character-management. Do not replace the
physical-test package with this unaccepted candidate. Native deletion acceptance
must use a separate disposable account/character, never any of the five saved
characters. The combined preview replay does not create or delete characters.

Next: continue reusable appearance customization and full UI/gameplay coverage;
accept the combined preview/loading changes when the shared account is available,
then verify the new management screens and deletion on an isolated disposable
account. Appearance sliders, near-plane/framing gaps, exact particles/lighting,
audio and the many full-game rows in PARITY.md remain open.


## September 16 shared appearance candidate

Controller creation now has a functional Y -> Appearance screen for skin, face,
hair style, hair color and facial feature. Choices come from the supplied Vanilla
DBC data, repair dependent choices, update the preview and populate the real
creation packet. The same renderer supports nondefault roster/world appearances.
This is an implemented host-validated candidate, not a native acceptance claim.

One geometry/animation/equipment pack stays open for each race/sex. WXA v2 adds
hair/extra bindings, while WXL supplies deduplicated body/detail/hair/extra layers
and geosets. All sixteen catalogs total 49,841,432 bytes, 3,603 section rows and
280 geoset mappings. WXA v1 remains readable. No appearance Cartesian product is
baked into separate models. Added GPU storage is at most two 87,380-byte buffers;
the fixed per-avatar index is 20 KiB, the animation cache remains 4 MiB and the
>=8 MiB free-memory allocation guards remain. Native headroom is unmeasured for
this candidate; host byte accounting must not be presented as console memory.

Resolved real source-data cases: unavailable rows are excluded from creation;
texture-only hairstyles without a hair-geoset row use the existing bald/scalp
fallback; female Tauren's unused hair-texture references do not prevent export.
Optional missing overlays are logged. Thirty-two unreachable section keys are
retained and audited separately, because they cannot form server-valid player
looks. See CHARACTER-APPEARANCE.md for the full per-race choice matrix and limits.

Host acceptance: all 16 catalogs match the raw Vanilla tables; 3,229 reference
appearances match actual native-C body/hair/extra pixels, using the independent
full-256-square host compositor as reference. All geometry packs remain open as
looks change. GPU allocation failures and low-memory guards release resources.
127 layer-boundary, 539 avatar-regression, 407 controller/packet and 517 combined
actual-asset preview checks pass; all 80 starter outfits remain covered. These
are host/controller-event tests, not physical pad or PC screenshot comparisons.

Native XBE/XISO compile passes: 1,867,776 / 484,966,400 bytes. The full rebuild
reports existing pinned-SRP unused-variable and font indentation warnings; the
final incremental link has only the known .edata merge warning. Normal MENU/live
input retained. Archive: build/candidates/20260916-appearance. Build identity:
looks-untested-receipt.json / looks-identity.json. Main evidence:
looks-host-summary.json, looks-dbc-coverage.json, looks-runtime-*.log,
looks-preview-host.log and looks-native-final-build.log in build/evidence.

All 108 hardware-test files match their original manifest. No xemu was running
when staging/building; no emulator, shared server, account or saved-character
operation was performed. No new screenshot, frame timing or measured native
headroom is claimed. The accepted 20260916-1212 checkpoint still has minimum
32,012 KiB and worst selection transition 906 ms; these belong to that old build.

Next: add a combined native customization replay/telemetry, then accept the
pending front-end candidates once the shared account is available. Native saved
nondefault creation/deletion needs a separate disposable account and preserved
existing characters. Continue the original in-world HUD/menu and gameplay work
independently. Exact PC layout/racial captions/randomization, all equipment and
animation coverage, near-plane gaps, effects/audio and the rest of PARITY.md
remain open. Do not promote this candidate to the user's physical-test package.


## September 16 Randomize and combined customization candidate

Implemented X: Randomize on the appearance screen, preserving the draft name,
race, class and sex. The sampler uses compatible Vanilla skin/face and
hair/color/facial records, handles sparse IDs, avoids repeating the current look
when an alternative exists, and commits choices/PRNG state only on success.
It uses bounded catalog scans and stack arrays, with no added persistent/GPU
allocation. This closes the missing randomization operation; exact racial
captions, layout, portrait/framing and the rest of the UI remain open.

The combined PreviewReplay now covers all 80 legal starter outfits, five
appearance changes per race/sex (80), 16 randomizations, rotation/zoom and name
keyboard cancellation, then returns to the original character. It sends no
creation/deletion commands. Its 952 records / 18,742 frames fit the unchanged
1,024-record bound. An isolated output option prepares fixtures without altering
normal-input staging; arguments and trace bounds are checked before writing.

WXTU telemetry adds the actual rendered look, facial feature, composition
revision and full-size CPU atlas hash, plus UI category/randomization count.
The hash is computed once per changed composition before mip building reuses the
CPU canvas. The checker requires drawn samples for each expected look, increasing
revisions and completed randomizations for every identity. It retains saved-world
return, all-outfit/background and >=8 MiB headroom gates. Historical packet
versions remain readable; mismatched lengths/magic and non-finite values fail.

Host validation: 4,096 valid/nonidentical randomized choices across sixteen real
catalogs; 901 boundary/dependency checks; 473 controller/packet checks; the full
combined input trace passes without create/delete commands. Seven synthetic
telemetry/checker cases reject stale, missing, unstable and incomplete evidence.
319 Human male reference compositions also verify the new atlas hash against the
prepared texture buffer on the host, with allocation-failure cleanup. No GPU or
physical input execution is implied by these host checks. Fixture output isolation
and invalid-argument preservation were checked separately.

Native nxdk build passes: XBE 1,867,776 bytes; XISO 484,966,400 bytes. The final
incremental build has only the known .edata merge warning; the full build had
existing font indentation warnings and an aggregate initializer warning fixed
before the final build. Native appearance composition cost, screenshots and new
free-memory measurements remain pending. The older accepted checkpoint's minimum
32,012 KiB and worst transition 906 ms still describe only that older build.

Evidence: build/evidence/customization-{host-summary.json,plan.json,replay.log,
telemetry.log,render-hash.log,native-final-build.log,untested-receipt.json,
identity.json}; per-catalog choice logs share the customization- prefix.
Archive: build/candidates/20260916-customization. Normal MENU/live input remains
staged. No xemu was active when building; no emulator, server, shared-account,
saved-character or hardware-network action occurred. The stable hardware-test kit
and all prior checkpoints/candidates are retained. This is not native acceptance
or full-port completion, and must not replace the user's verified physical kit.

Next: advance the original in-world player/target HUD and menus independently.
For the pending combined native front-end gate, use the 900-second combined
capture and customization-plan checker described in PLAYTEST.md once the shared
account is available. Capture a fresh saved-state baseline and archive the actual
replay-mounted disc receipt. Use a separate disposable account for nondefault
create/save/reconnect and deletion tests; never sacrifice the saved characters.
Synchronous appearance I/O, near-plane/framing, effects/audio, full PC interface
and the wider gameplay/streaming compatibility matrix still require work.


## September 16 player/target HUD candidate

Implemented original-art player and target frames driven by the existing Vanilla
entity snapshots: names/levels, health, active resource, XP, death/ghost states and
target clearing. Resource selection follows the server's power-type byte and
supports the five Vanilla slots, with rage displayed in points. Unknown types,
zero maxima, absent units, large counters and long names are bounded. This is HUD
data/display coverage; other class combat and full unit-frame functionality remain
unfinished. Portrait circles are empty; no unrelated portrait is substituted.

The private WXU1 v2 atlas adds the supplied frame, status-bar and skull artwork.
The loader still accepts v1. The atlas remains 512x512 with the existing 2,048-quad
pool and 1,376,256-byte graphics allocation. HUD snapshots are fixed CPU storage;
no per-frame allocations were added. The 8 MiB guards remain. Newly measured
native headroom/timing is still pending; the old checkpoint's minimum 32,012 KiB
and worst transition 906 ms must not be attributed to this build.

Host verification covers real UPDATE_VALUES packets through the production parser
for all five resources, switching types, unknown/zero values, death/ghost and
target removal, plus actual-atlas draw bounds: 995 checks, 117 sample quads. Another
182 UI checks cover strict v2 metadata, allocation failures, mirrored/partial
sampling, clipping and bounded names. Actual legacy/current atlas reads pass.
Eight synthetic telemetry tests cover historical versions and new WXTV fields.
The labelled hud-host-preview.png is a software rendering of real C draw commands
with synthetic unit data, not an xemu screenshot or GPU acceptance result.

Native build passes: default.xbe 1,871,872 bytes; XISO 484,966,400 bytes. Final link
has only the known .edata merge warning; full build also has existing pinned-SRP
unused-variable and font indentation warnings. WXTV adds submitted HUD values to
a 1,288-byte packet. check-hud.py checks live value agreement, submissions, memory,
and optionally two selected targets followed by clearing.

The combined fixture now adds read-only target cycling/clearing after the saved
world return. It has 961 records / 18,994 frames, within existing bounds; all 80
outfits, 80 selector changes, 16 randomizations and draft cancellation still pass
host controller replay. The host replay does not simulate networked targeting or
GPU drawing. Use hud-customization-plan.json with this trace, not the prior plan.
One 900-second future xemu capture can cover front-end/customization/HUD gates.

Evidence: build/evidence/hud-{host-summary.json,unit-render.log,ui-boundaries.log,
ui-legacy.log,ui-current.log,telemetry.log,combined-replay.log,customization-plan.json,
host-preview.png,source-provenance.json,native-final-build.log,untested-receipt.json,
identity.json}. Assets: build/hud-prepared-20260916; isolated fixture:
build/hud-replay-20260916. Archive: build/candidates/20260916-hud.
Normal MENU/live input remains staged. No xemu, server, shared account, saved
character, credential or hardware-network operation occurred. The verified
checkpoint, earlier candidates and stable hardware-test package are retained.

Next: implement real unit portraits and continue the original in-game menu/action
bar foundation and its missing operations. Exact HUD layout, faction/difficulty,
auras, player/group targeting, target-of-target, and the broad gameplay/streaming
matrix remain open. Accept pending native work when the shared account is
available, with actual screenshots and source/disc identity. Preserve every saved
character; use a separate disposable account for creation/deletion persistence.
This candidate is not a replacement for the stable physical-test package.


## September 16 resident-model portrait candidate

Implemented real player/creature portraits inside the original HUD frames. The
pass reuses the current resident geometry, animation, composed appearance and
item textures. Circular depth/stencil clears use 37 rectangles per portrait;
there is no additional GPU allocation or frame heap allocation. A fixed 22,540-
byte CPU catalog holds 108 records in a 4,768-byte private PORTRAIT.WPT. The
selected creature retains one of the existing 32 actor slots and is skinned even
outside the world camera. Unknown/unprepared displays remain unavailable.

The converter reuses WoWee's M2 cameras and head-bone approach. All 92 prepared
creatures supply an authored portrait camera; all 16 player models supply head
key-bone 6. Initial native screenshots exposed low player framing, especially on
hunched models. The corrected catalog transforms the head pivot by the standing
pose and tightens player camera distance. Actual corrected Dwarf evidence is
build/evidence/portrait-focused-dwarf.png. Precise PC framing, non-idle/dead poses,
all gear/appearance variants and other-player portraits still require work.

An isolated, credential-free fixture initializes UDP telemetry only; it never
starts the authentication worker. Both fixture discs/source archives are retained
under build/portrait-test-20260916 and build/portrait-focus-test-20260916. They use
native rendering scale, 64 MiB and disposable HDD writes, and no shared server or
saved-character operation occurred. Both project-owned emulators were stopped
through QMP after their captures finished. No emulator remains intentionally active.

The corrected 300-second capture has 7,190 samples, draws all 16 player models and
92 prepared creatures, reports zero asset/UI/catalog failures, and retains at
least 42,912 KiB free. Guest-clock frame p50/p95/p99/max: 33/34/76/250 ms. Stable
identity samples: 33/34/45/242 ms. CPU portrait submission p95/p99/max: 1/1/2 ms.
These measurements exclude terrain and full-world memory, are not physical Xbox
performance, and do not establish consistent 30 fps. This fixture has synthetic
unit data and no controller replay or physical input verification. It does not
accept live-world HUD integration, target/equipment synchronization or full parity.

The initial 330-second capture is also retained: 7,932 samples, all 108 models,
42,836 KiB minimum, 33/34/82/329 ms frame timing. Its geometry/coverage checks pass,
but screenshots exposed the framing issue above. Do not treat it as final visual
acceptance or compare these uncontrolled runs as a hardware benchmark.

Host checks: 9,337 camera/catalog/truncation/finite-value/circular-mask checks,
including every real camera, pass. Nine telemetry cases cover historical versions
and WXTW. WXTW is 1,328 bytes and adds catalog/keys/draws/mask/submission/fixture
metrics. The offline checker is separate from live check-hud.py --portraits;
synthetic fixtures must fail the live-world gate. See docs/PORTRAITS.md.

Normal XBE/XISO build: 1,880,064 / 484,966,400 bytes; final build reports only the
known .edata merge warning. MENU/live input remains staged with no fixture marker.
The normal binary adds defensive invalid-display-ID guards after the isolated
capture's source snapshot; this normal build is not a verified world checkpoint.
Archive: build/candidates/20260916-portraits, with its own source/output receipt.
No verification.json is issued. The accepted 20260916-1212 checkpoint, all earlier
candidates, stable hardware kit, credentials and saved characters are preserved.

Next: original in-game menu and action-bar operations, then missing gameplay UI.
The pending combined front-end/customization/live-HUD gate still needs the shared
account to be available; use the saved-state baseline and actual mounted receipt,
and extend the HUD check with --portraits. Keep the full compatibility matrix;
this portrait implementation does not complete the port or replace the physical
hardware test package.


## September 16 original action-icon candidate

Implemented original spell/item icons for the existing eight actions in each
LT/RT/combined-trigger layer. Names, empty bindings, mapped item displays and
pressed-control highlights use the existing binding/stance logic. The common UI
renderer now preserves ordered artwork/icon/text batches, bounded to 128 batches
inside its unchanged 2,048-quad / 1,376,256-byte graphics allocation.

The converter indexes Vanilla Spell, SpellIcon and ItemDisplayInfo directly:
51,962 mappings / 2,522 distinct 32x32 images in a 10,745,832-byte ICONS.WIC. It
normalizes legacy .tga references to .blp. 47 source paths are absent from the
supplied archives and remain listed in the private .missing.txt; their mapped
icons use the original question mark. Do not claim those icons are verified.
The reusable runtime keeps a 415,696-byte CPU index plus 262,144-byte GPU atlas
(677,840 bytes total), protects requested cells during eviction and reads/uploads
at most one 4 KiB image per frame. Current gameplay prewarms all 24 bindings.

Host checks pass: 1,899 icon/cache/action-UI checks, 188 shared UI checks, 993 HUD
checks and 10 telemetry cases. These include malformed asset dimensions/indices,
allocation failures, eviction protection, one-read budget, warm-cache stability,
UV clipping, texture ordering and batch exhaustion. WXTX telemetry is 1,368 bytes
and appends icon counts/allocation/loading/draws and UI batch/upload metrics.

The isolated 110-second native fixture uses synthetic spell/action/item state,
real catalogs and the production renderer. It never starts the auth worker and
has no credentials. Capture native-action-icons: 1,567 received samples, all
three layers, correct slot bindings and 6/8/8 drawn icons, no failures and stable
cache uploads. Minimum free memory 43,592 KiB; received frame p50/p95/p99/max
33/34/34/35 ms. Upload submission p95/p99/max 0/1/1 ms. Every received sample was
already warm; cold-cache upload pacing is covered by the host test, not this
capture. These are guest-clock timings in a small offline scene, not full-world
or physical Xbox performance. UI state was injected without controller replay.
Actual screenshot: build/evidence/native-action-icons-right.png.

Fixture source ZIP, disc receipt, EEPROM/config, 64 MiB QMP result and logs are
preserved in build/icons-test-20260916. Its project-owned emulator was stopped
through QMP after capture completion. No server/shared-account/saved-character,
credential or hardware-network action occurred. Stable hardware package remains
unchanged. Earlier verified checkpoints and both new portrait captures remain.

Normal build is MENU/live input with no offline marker: default.xbe 1,884,160 bytes,
XISO 495,714,304 bytes; final build has only the known .edata merge warning. Archive:
build/candidates/20260916-action-icons. The normal XBE matches the isolated fixture
XBE, but this is still not a live-world checkpoint; no verification.json is issued.

Remaining: cooldowns, range/resource/usability, charges/counts, original complete
bar/menu layout, tooltips, macros, pets and other class/form gameplay. Existing
spellbook text UI remains. Continue substantial gameplay/UI implementation, then
run the combined pending front-end/live-HUD/action regression when the shared
account is available. Preserve a fresh saved-state baseline, source/disc identity
and >=8 MiB measured full-world headroom. Full PC parity/release gates are not met.
See docs/ACTION-ICONS.md for formats, implementation scope and reproduction.


## September 16 action cooldown candidate

Added a separate bounded cooldown subsystem and radial/timer feedback on original
spell/item icons. It restores initial timers on login, handles Vanilla's flagless
SMSG_SPELL_COOLDOWN, per-spell clears and delayed cooldown events, and starts base
spell/category timers on validated SMSG_SPELL_GO. Server item-query overrides
resolve through item caster GUIDs; held cooldowns remain visibly held until the
server event. Other-unit packets do not change player state. Disconnect resets
character-specific timers; map transfers retain them. The UI samples the clock
inside the network-state lock. No per-frame allocation or added GPU buffer.

COOLDOWN.WCD indexes all 22,357 supplied Vanilla Spell.dbc rows in 447,140 resident
bytes (447,156 on disc). Fixed timer/item metadata state is 45,076 CPU bytes; it
stays outside WxGame's existing large stack copy. Radial shading uses a 2 KiB
lookup and bounded quads. Overflow/missing metadata are measured, not silently
accepted as complete. Global cooldowns, talent/aura modifiers, ranged-speed rules,
full usability and all-class combat remain open; the base prediction cannot yet
represent every modified spell accurately. See docs/COOLDOWNS.md.

Host: 1,429 cooldown/catalog/boundary checks, 2,024 icon/UI checks (including exact
radial areas), 11 telemetry cases and 72 encrypted world-protocol scenarios pass.
The encrypted local fixture proves initial/lockout/clear state reaches the
published action view and rejects a newer flags-byte layout. No real account,
shared server or saved character was involved.

Native capture build/evidence/native-cooldowns.csv: 1,850 received samples during
one 115-second launch/capture allowance, all eight injected packet phases and all
three action layers. Every received timer matches the deterministic sequence;
no catalog/UI/cooldown failures or overflow. Minimum free memory 43,048 KiB,
frame p50/p95/p99/max 33/34/34/35 ms, maximum 585 UI quads. QMP confirms 67,108,864
bytes RAM. Actual screenshot: build/evidence/native-cooldowns-charge.png. These
are guest-clock timings in an offline scene; the timer sequence uses 33 ms per
fixture frame. This is injected state, not controller replay, live combat or
physical Xbox acceptance. Full-world measured headroom still needs its own gate.

The fixture at build/cooldowns-test-20260916 has its exact source ZIP, disc/output
receipt, dedicated config/EEPROM and disposable HDD launch. Source/output hashes
were checked after capture. Only its owned PID 43164 was quit through QMP after
completion. Normal staging remains MENU/live input without PTTEST.BIN. The stable
hardware kit and verified checkpoints remain untouched; no verification.json is
issued for an offline candidate.

Next: extend cooldown behavior through GCD and spell modifiers alongside
resource/range/usability and item counts, then missing original in-game menus/UI.
Keep the pending combined front-end/customization/live-HUD/action regression
until the shared account is available; use a fresh saved-state baseline and
source/disc identity. Do not treat this fixture as the release or parity gate.


Normal candidate archive: build/candidates/20260916-cooldowns. Final XBE is
1,896,448 bytes; XISO is 496,173,056 bytes. The final rebuild removes two new
compiler warnings (explicit zero initializer and indentation). Its four loaded
XBE sections are byte-identical to the captured executable; all six changed
bytes belong to XBE/PE/certificate timestamp fields. Both hashes and the section
comparison are retained in build/evidence/cooldowns-xbe-comparison.json. Final
incremental build has only the known .edata linker warning. These identities do
not turn the offline fixture into a live-world or hardware checkpoint.


## September 16 global cooldown and modifier candidate

Implemented cast-start GCD, preparation cancellation, completion without timer
restart, other-caster isolation, zero-duration GCD hints preserving lockouts,
server flat/percentage SET modifiers over all 64 family bits, player-family
matching, haste eligibility/clamping, ranged attack-time recovery and wand
category flags. GCD-only sweeps omit numerical countdown text; longer recovery
still takes precedence. Player timing fields publish before the next received
spell packet. No added GPU allocation or per-frame allocation.

WXCD v2 indexes all 22,357 supplied spells with family/attribute/category data:
1,251,992 resident bytes, 1,252,008 disc bytes. v1 remains readable with zeroed
extension metadata. Fixed cooldown state is 60,216 bytes, including 16 GCD
categories and signed modifier totals. Multiple overlapping nonzero modifier
bit totals remain ambiguous: their contributing aura masks are absent from the
wire packet. The client counts this case and retains base timing. Resolving
those auras, charged modifiers and all-class live behavior remains required.
See docs/GLOBAL-COOLDOWNS.md for exact scope, formats and source references.

Host: 4,511 GCD/modifier/lifecycle/bounds checks, 1,445 base cooldown checks,
2,026 icon/UI checks, 12 telemetry cases and 77 encrypted protocol scenarios pass.
The final zero-base ranged recovery correction also passed the five affected
encrypted GCD scenarios. One initial protocol fixture omitted roster appearance
fields; restoring the test data fixed it without weakening the appearance check.

Native capture build/evidence/native-global-cooldowns.csv: 1,781 samples during
one 115-second launch/capture allowance, all eight injected phases and exact
published timers/context/counters. No failures, missing metadata or overflow.
Minimum free memory 42,228 KiB; guest frame p50/p95/p99/max 33/34/34/35 ms; maximum
UI quads 850. QMP confirms 67,108,864 bytes RAM. Actual native screenshot:
build/evidence/native-global-cooldowns.png. Synthetic packets/context use a
33 ms/frame deterministic clock. This is neither physical controller verification
nor live combat; emulator timing is not a hardware performance result.

The isolated fixture build/gcd-test-20260916 retains its source ZIP, receipt,
mounted output identities and dedicated config. All 247 source hashes and 60
mounted outputs verified after capture. Only owned xemu PID 29860 was quit once
capture completed. No server/shared-account/character/credential/network changes.
The hardware test reservation remains; do not initiate shared account replays
until released. Stable hardware kit dist/WOWX-hardware-20260916/WOWX is unchanged.

Normal candidate archive: build/candidates/20260916-global-cooldowns; XBE
1,904,640 bytes, XISO 497,025,024 bytes, 247 source hashes/131 outputs. MENU/live
input, no PTTEST marker. Only existing font indentation/.edata build warnings.
No verification.json: this is not a full-world checkpoint. WXTZ telemetry is
1,472-byte UDP payload, at the 1,500-byte IPv4 MTU with headers; do not grow it
further without splitting/redesigning telemetry.

Next substantial batch: resource/range/usability and count feedback using real
Vanilla spell/item data, then missing original in-game menus. Continue unresolved
modifier/aura coverage. Preserve pending combined front-end/customization/live
HUD/actions regression for when the shared account is available, with a fresh
saved-state baseline and exact source/disc identities. Full-world >=8 MiB free
headroom, complete parity, sustained performance and physical hardware gates
remain unmet. Autonomous continuation remains active with no morning cutoff.


## September 16 action feedback and cast/channel candidate

Implemented resource shortages, conservative unit-target range hints, unlearned,
passive/dead/form restrictions, item quantities and remaining charges. These are
advisory UI snapshots, not client-side command rejection. Real Vanilla Spell and
SpellRange data now feed WXS v2 (22,357 records, 4,426,702 disc bytes); v1 stays
readable with unknown extended metadata. Known/name storage is fixed at 105,064
Xbox bytes plus the 44,714-byte ID index; one record is read per frame.

The production parser tracks cast starts/completion/failure, pushback, finite and
indefinite channels and channel shortening. B cancels the active cast/channel
with the corresponding Vanilla command. Item instance charges survive entity
snapshots; cached item prototypes identify charge slots. Shared START/GO decoding
eliminates divergent cast/cooldown target parsing. Fixed cooldown state is now
65,336 bytes; cast state 28 bytes; inventory 8,792 bytes. No new GPU allocation.

All 81 encrypted world scenarios pass, including real command-wire cancellation
and malformed channel/delay rejection. Targeted host checks: 205 cast lifecycle,
40 feedback, 1,097 catalog/controller, 4,666 icon/UI and 13 telemetry cases. All
22,357 supplied spell records validate. Earlier 4,511 global and 1,445 base
cooldown checks also pass this batch.

One combined offline native capture: build/evidence/native-action-feedback.csv
and its .actions.csv companion, all 12 injected phases. 1,736 main samples / 1,735
companion samples; minimum free 42,096 KiB; guest frame p50/p95/p99/max
33/34/34/35 ms; at most 535 UI quads. QMP confirms 67,108,864 bytes RAM. No parser,
asset, UI or modifier failures. Actual screenshot: native-action-feedback.png.
The screenshot's cast bar overlaps the explanatory fixture notice; that notice
is not part of the live UI. Resource/item scenarios are explicitly synthetic,
including fixture-only item quantities/charges. This is not class combat, physical
controller verification or hardware performance, and does not satisfy the pending
combined front-end/live-world acceptance gate.

The credential-free disc/source ZIP remains in build/actions-test-20260916.
All 258 source and 60 mounted output hashes verified after capture. Only owned
xemu PID 51456 was quit after completion. Candidate archive:
build/candidates/20260916-action-feedback (XBE 1,921,024 bytes; XISO 498,991,104
bytes). No verification.json. Stable hardware package, server, shared account,
saved characters and credentials remain untouched.

Remaining: per-skill cost scaling, overlapping/charged aura modifier identity,
exact movement leeway, facing/LOS/faction/reagents/combo and all class-specific
availability. Cast bar is a native functional layout, not final original artwork.
Original workflow parity, live integrated acceptance and release gates remain
open. Approved implementation roadmap is docs/ROADMAP.md. Next: shared fog and
bounded world streaming, while retaining the account reservation. Third-party
addons are deferred; original native core UI workflows remain required.


## September 16 fog foundation candidate

Initial shared outdoor fog is implemented: 90-145 world units, matched background,
scaled world-depth vertex output and NV2A final color blending preserving texture
alpha. UI/font/map and framed portraits/previews reset fog state. No extra render
target/GPU allocation. Host: 1,714 bounds/transition checks. Native build succeeds;
compiled shader is 21 instructions with unchanged constant bindings.

The resident Northshire fixture completed: build/evidence/native-fog.csv, 2,292
samples, three matched camera pairs alternating fog off/on, 302 preloaded entries,
no streaming during measurement and no asset/UI failures. Minimum free 40,144 KiB.
All six phases have guest frame p50/p95/p99 33/34/34 ms. In the heaviest view
(142 draws / 16,352 triangles), draw p50 changed from 1 to 2 ms; p95 remained 2 ms,
p99 from 2 to 3 ms. These are coarse guest-clock measurements from static resident
views, not sustained travel/combat or physical Xbox performance. QMP confirms
67,108,864 bytes RAM. All 264 source / 59 mounted output hashes verified.

VISUAL ACCEPTANCE IS PENDING. The user pressed physical Escape during Computer
Use; desktop interactions stopped, and no fog screenshot was captured. Numeric
capture success cannot establish correct fog appearance, alpha cutouts or UI state
restoration. Only owned PID 51304 was quit after lifecycle duration_limit. Fixture
image/config/source ZIP/receipt remain in build/fog-test-20260916. Candidate:
build/candidates/20260916-fog-foundation, no verification.json. It is not a
replacement for the preserved action candidate or the stable hardware kit.

Normal world currently selects only the outdoor fallback. The API distinguishes
indoor/underwater environments and disables their generic fallback, but automatic
classification/authored lighting is not implemented. WoWee DBC light-volume/band
conversion, zone/time transitions, world effects and native indoor/underwater
verification remain. See docs/FOG.md for the contract and source reference.

Next substantial work: complete fog visual/zone-data integration alongside bounded
world streaming stages, prefetch and residency hysteresis. pack.c still performs
whole-entry reads/validation synchronously despite its three-entry quota; the
previous travel stalls are not fixed by this fog batch. Preserve the shared account
reservation and pending combined login/customization/world/actions acceptance.
No current emulator remains from these captures. Full Vanilla/release gates remain
unmet; original hardware validation is still required.

## September 16 staged world streaming and fog visual checkpoint

Implemented incremental world payload reads/validation, atomic resident publication,
cancellation/rollback and detach-safe pending identities. Terrain-pack index opening
is also staged; prefetch/retention uses separate distances, required collision can
preempt optional visual work, and fixed resident lookup reduces selection overhead.
Pending CPU/GPU allocations and actual retained index capacity are accounted for.
Normal UI distinguishes loading terrain from an asset error. Details: STREAMING.md.

Host: 3,562 staged loader/index/malformed/allocation checks, 343 runtime/collision,
539 avatar checks, strict streaming companion decoder and 13 existing telemetry
tests pass. Real-pack one-tick-per-frame boundary/Abbey/camp traversal completes
in 3,373 ticks with no blocked steps or asset failures. A straight Y=-132.493
route failed at the same point with both the changed and archived original loader;
that pre-existing route limitation is retained, not counted as passing.

One combined native capture: build/evidence/native-streaming.csv and .stream.csv;
7,057 main / 7,056 aligned companion samples, two complete route cycles, zero
blocked steps/traversal/asset/region failures. Guest frame p50/p95/p99/max:
33/34/34/35 ms. Minimum actual free: 36,340 KiB. QMP confirms 67,108,864 bytes RAM,
native render scale and disposable HDD writes. Region-update p99/max: 4/9 ms;
world payload streaming plus animation p95/p99/max: 9/13/21 ms. All configured
read/validation quotas respected. These offline world-only results exclude NPC,
avatar, combat and server workloads, and are not physical Xbox performance.
Telemetry began at fixture frame 240; subsequent unload/reopen transitions supplied
index timing. Initial cold-boot loading latency is not measured by this capture.

Actual screenshots: native-streaming-boundary.png, -abbey.png, -camp.png and the
matched -fog-off.png / -fog-on.png. The pair shows distant trees fading into the
horizon with near surfaces, foliage cutouts and overlay text intact. This resolves
the earlier missing fallback-fog visual evidence for those views; authored zone/
time/indoor/underwater behavior and all-material/avatar/portrait acceptance remain.
Movement is a deterministic offline collision route with an explicit relocation
between boundary and Abbey routes. It is neither injected pad nor physical input.

Fixture/source ZIP/receipt: build/stream-test-20260916. All 273 recorded source
and 63 mounted-output hashes verified after capture. The analysis checker was
added after that receipt, and is included in the normal candidate's 274-source
archive. Only owned xemu PID 20424 was quit after duration_limit and identity checks.
No emulator remains from this run. Normal candidate archive:
build/candidates/20260916-staged-world-streaming, XBE 1,933,312 bytes, XISO
498,991,104 bytes, 131 normal output identities, no offline marker. No
verification.json: combined live acceptance and the release gates are still open.

Stable hardware kit dist/WOWX-hardware-20260916/WOWX, checkpoints, private assets,
server, reserved account, saved characters and credentials remain protected.
Next engineering priority: apply bounded payload jobs to NPC/avatar animation
loading while retaining complete prior poses during clip changes, then authored
Vanilla zone/time fog data. Large index commit/copy, avatar composition/profile
opening, crowded animation, all-map coverage and physical hardware remain work.
Autonomous continuation remains active with no morning cutoff.

## September 16 bounded NPC/player payload checkpoint and fog-scale follow-up

Runtime NPC/player payloads now share the incremental reader/validator, with two
commits per call and per-scene limits of 64 KiB reads, eight operations and 64 KiB
validation. Partial batches remain invisible. Complete NPC fallback clips and the
player's complete body/equipment pose survive transitions. Rapid player idle/run
requests finish the pending partner instead of starving it. Allocation rejection
stops the call at its first failure and preserves the last complete pose. Scene
budgets and the actual 8 MiB headroom guard remain unchanged. ACTOR-STREAMING.md
records the contract and remaining work.

Host: 3,644 staged loader checks, 348 runtime/collision/pose checks, 539 avatar
checks, two strict companion decoder suites and 13 existing telemetry tests pass.
The actual equipped Human pack passes idle/run/attack/death and return loading.
Its host result has 12 components, 640,296 final payload bytes and 378,900
auxiliary bytes; host allocation stubs are not a native memory measurement.

Native retry capture: build/evidence/native-actor-streaming-retry.csv and its
.stream.csv/.actors.csv companions, 7,112 main / 7,111 aligned companion samples.
Two offline boundary/Abbey/camp cycles passed with eight synthetic NPCs from six
prepared templates and an equipped Human. Zero asset, region or traversal errors;
all eight complete NPC models retained after warmup; zero incomplete player gaps
across 964 synthetic clip requests, including rapid idle/run and attack/death.
NPC rapid changes can still cancel obsolete reads; request coalescing remains.

Minimum actual free memory: 32,968 KiB in QMP-confirmed 67,108,864-byte xemu at
native scale. Guest frame p50/p95/p99/max: 33/34/36/55 ms. Actor streaming plus
skinning p50/p95/p99/max: 5/15/20/34 ms; region maximum 9 ms; world payload plus
animation maximum 23 ms. All scene read/validation quotas were respected. These
are synthetic real-asset/rendering/collision results, not live server combat,
injected pad, physical controller, full-game 30 fps or physical Xbox acceptance.

The initial launch (native-actor-streaming) exited with host exception 0xc000041d
and zero telemetry; its log stops during host graphics initialization. Failed
lifecycle/logs and Windows event remain in the fixture's startup-failure folder.
The same unchanged disc succeeded on retry. Only retry PID 60792 was quit after
capture completion and identity checks. No active emulator remains from the run.

Fixture: build/actor-stream-test-20260916, exact source ZIP/receipt/disc/config.
All 275 recorded source and 63 mounted-output identities verified after capture.
Actual screenshots: native-actor-streaming-retry-run.png, -attack.png and
-complete.png. Accepted scoped candidate: build/candidates/20260916-actor-streaming,
XBE 1,937,408 bytes / XISO 498,991,104 bytes, 275 sources and 131 normal outputs.
No verification.json and no replacement of the preserved stable hardware kit.

After preserving that candidate, a coordinate review found an error for actors
with a scale other than one: their camera is divided by scale and view axes are
multiplied by scale, so shader Z is already world depth. The fog uniform now uses
one instead of applying scale a second time. Native build and 1,714 fog parameter
checks pass; independent arithmetic is recorded in fog-scale-audit.json. The
capture used scale-one units and is unchanged by this correction. SCALED-ACTOR
NATIVE VISUAL VERIFICATION REMAINS PENDING. Current normal build/source archive:
build/candidates/20260916-fog-scale-correction (same byte sizes, different XBE/hash).
Its receipt is current; the preceding native capture retains its historical source.

Next: authored Vanilla zone/time fog and scaled-actor verification in that batch;
profile-opening/composition and animation scheduling; combined live front-end/
customization/HUD/combat acceptance when the hardware account is released; general
map coverage and remaining original UI/gameplay systems. Full PARITY.md and release
gates remain unmet. Stable hardware kit, checkpoints, private assets, server,
reserved account, saved characters and credentials remain protected.


## September 16 authored Vanilla fog and world clock checkpoint

Candidate: build/candidates/20260916-authored-fog. XBE 1,945,600 bytes, normal
XISO 499,187,712 bytes. Current receipt: 285 source hashes / 132 normal outputs;
all matched after the run. Stable hardware kit and preceding checkpoints intact.

The client now samples Vanilla fog volumes/time bands: map defaults, local falloff,
midnight interpolation, signed haze starts, stable overlap ordering and smooth
changes. Northshire uses default profile 12. Converter reuses pinned WoWee
coordinate/band helpers and strict 1.12 layouts. LIGHT.WLF: 374 volumes / 26 maps /
395 profiles; 135,040 resident bytes, no per-frame allocations or disk reads.
Five source profiles have empty bands; fallbacks and DBC hashes are explicit in
build/evidence/lighting-assets.json. This is fog-data coverage, not all-map geometry
or gameplay. See FOG.md for distance adaptation below the existing 160-unit cull.

Exact eight-byte LOGIN_SETTIMESPEED is validated and published in a locked clock
snapshot, before or after LOGIN_VERIFY_WORLD. Minutes-per-second scaling, midnight,
tick wrap and logout reset are handled; absent clock means noon. Six independent
encrypted localhost scenarios pass, including early/late publication and malformed
disconnects. Shared server/account/characters were untouched. 28,120 host checks
pass, covering actual data, malformed files, reload rollback, both heap allocation
failures and headroom refusal. Strict WXL2 telemetry decoding passes.

Native: build/evidence/native-lighting.csv and .lighting.csv, 2,626 main / 2,625
companion samples, all eight phases, zero asset/UI failures, all three synthetic
real wolves rendered after warmup. QMP: 67,108,864 bytes, native scale 1, disposable
disk writes. Minimum free39,488 KiB (~38.6 MiB); guest frame p50/p95/p99/max
33/34/34/79ms. One79ms interval followed entry into the isolated scale comparison.
Fog sample/blend p50/p95/p99/max0/1/1/2 ms at millisecond resolution. Resident
Northshire, synthetic clock/conditions and .5/1/2 scales; not streaming, live combat,
persistence, physical input or Xbox performance. First received frame156 excludes
startup/preload. Native numeric gate passes; no full-port verification.json.

Actual JPGs: native-lighting-noon, -midnight, -dawn, -rain, -scale-off, -scale-on in
build/evidence. They show authored horizon/fog changes, readable overlay and all
three fogged scales. Scoped non-unit fog-depth visual acceptance is now complete.
Some screenshots follow the timed capture on the exact unchanged instance/disc.
Fixture float diagnostic labels are blank because the native formatter omits
floats; numerical fog ranges/catalog size are verified in WXL2 instead.

Fixture build/lighting-test-20260916 preserves source ZIP/disc/receipt/config/logs
and capture-identity.json. All285sources/60mounted outputs matched before/after.
Capture reached duration_limit; only owned PID3092 was quit after executable,
disc and QMP checks. No emulator remains; no hardware-kit replacement or real login.

Remaining: full ambient/diffuse/sun/sky lighting (nearby geometry still uses fixed
lighting under midnight fog), weather packets/rendering, automatic water/interior
classification, all-material/avatar/portrait/live integration; bounded profile
opening/composition and animation scheduling; generalized map coverage; combined
front-end/HUD/combat acceptance once the reserved account is released; physical
stock-console/controller gates. Full Vanilla parity remains substantially unfinished.


## September 16 authored world material and sky checkpoint (latest)

Candidate: build/candidates/20260916-material-sky. XBE 1,953,792 bytes; normal
XISO 499,515,392 bytes. All 288 source / 132 normal output identities matched after
acceptance. Stable hardware kit, earlier candidates, shared server/account, saved
characters, private assets and credentials remain intact. No shared login occurred.

The normal client now applies Vanilla ambient/diffuse RGB and time-derived light
direction to terrain, buildings and actors. Actor rotation preserves world-space
lighting; scale does not multiply intensity. Framed previews/portraits retain their
own lighting. A camera-relative sky colour gradient blends into the fog horizon,
using the existing UI buffer/white sample with no extra surface or allocation.
Explicit shader uniforms replace compiler-literal patching. Fixture numeric labels
are readable again. Detailed asset and rendering contracts are in FOG.md.

WXL1 v2: 374 volumes / 395 profiles, 451,040 resident bytes under 512 KiB. Backward
loading for v1 uses explicit palette fallbacks and accounts for the expanded memory.
29 profiles have some empty bands (199 bands total); the asset report names them.
Five are entirely empty. Smog/sun colours are retained; celestial objects, clouds,
stars, weather rendering and automatic indoor/water classification are unfinished.
Normal requests are still clear/outdoor. No complete sky or all-material parity claim.

Targeted host validation: 74,690 lighting/clock checks with actual private assets,
2,226 UI/sky checks, strict WXL2/3 decoder checks and six encrypted local clock
scenarios. The latter are now part of the default 87-scenario protocol suite; only
the affected six were rerun for this batch. New WXL3 companion is 136 bytes; main
WXTZ remains 1,472 bytes. Cg emits 23 instructions with explicit c0..c8 uniforms.

One combined native run: build/evidence/native-material-sky.csv (3,052 frames),
.lighting.csv (3,051 samples), eight phases, all three scaled wolves and an
independently lit wolf portrait after warmup. Zero asset/UI errors. Minimum free
39,184 KiB (38.27 MiB). Guest frame p50/p95/p99/max 33/34/34/35 ms; lighting plus
sky build/submission/wait 1/2/3/4 ms. QMP confirms 67,108,864 bytes, native scale 1,
disposable disk writes. Captured frames 154..3205 exclude startup/preload. This
resident scene does not establish streaming, combat, crowded-scene or hardware
performance. Conditions/time/entities are synthetic; no injected or physical input.

Actual JPGs: native-material-sky-noon, -midnight, -dawn, -rain, -inspection. Nearby
world materials now darken at midnight; dawn changes their tint, distant geometry
meets the sky/fog, foliage cutouts remain visible and portrait/UI lighting stays
independent. Matched day/night screenshots followed the timed capture on the same
unchanged disc. Underwater/indoor synthetic requests suppress the sky as intended;
live environmental selection and all-avatar/material combinations remain pending.

Frozen fixture: build/material-sky-test-20260916 (source.zip, disc, receipt, config,
logs, capture-identity.json). All 288 source / 60 mounted outputs matched before
and after capture. It ended at duration_limit; only owned PID74524 was quit after
executable/disc/QMP verification. No emulator remains. Normal disc has no fixture
marker. No full-port verification.json; stable hardware package was not replaced.

Next: authoritative weather/environment selection with reusable WoWee/map data;
bounded avatar profile opening/composition and NPC idle/run coalescing/animation
scheduling; all-map asset pipeline. The combined login/customization/HUD/combat
acceptance remains pending while the account is reserved for hardware testing.
Original UI, all nine classes, professions/social/travel/instances/PvP/audio and
the full PARITY.md and physical stock-console/controller release gates remain.
Continue substantial feature batches autonomously; the old morning cutoff does
not apply and the full feature goal is unchanged.


## September 17 weather-state and atmosphere checkpoint (latest)

Candidate: build/candidates/20260916-weather-state (batch started September 16).
XBE 1,957,888 bytes; normal XISO 499,515,392 bytes. All 294 source / 132 normal
output identities matched after acceptance. Earlier candidates, stable hardware
kit, reserved server/account, saved characters, private assets and credentials
remain intact. No shared login, server restart or account mutation occurred.

The normal client now validates the exact 13-byte Vanilla SMSG_WEATHER layout,
including sound ID and instant-change flag. Locked snapshots publish early/late
packets. Disconnect/logout and map-transfer start clear stale state; updates
between transfer start and NEW_WORLD survive. Malformed packets fail without
publishing partial state. This corrects an inaccurate 8-byte desktop WoWee comment
using the pinned vMaNGOS serializer and Vanilla protocol reference (WEATHER.md).

Weather grade blends clear and overcast DBC fog/material/sky profiles. Smooth
changes use bounded exponential convergence; instant changes apply the target
on the same frame. Clearing reaches zero exactly. Explicit indoor/underwater
requests take priority. No new frame allocation, I/O, texture or surface. Rain,
snow and storm currently share the overcast lighting approximation; distinct
precipitation, clouds, sound playback and automatic interior/water classification
remain unfinished. Retaining sound IDs does not implement weather audio.

Targeted host validation: 23,163 parser/presentation/real-catalog checks, three
telemetry decoder tests and 12 encrypted localhost weather scenarios passed.
The default protocol suite now contains 99 scenarios; only the 12 affected cases
were rerun for this batch. Fixture credentials are public synthetic data. WXW1
is a strict 68-byte companion; WXTZ remains 1,472 bytes. Native build passed.

One combined native capture: build/evidence/native-weather-state.csv, 3,681 main
samples and 3,680 each weather/lighting companions. All ten phases passed,
including smooth rain/storm/clearing, instant snow, explicit indoor/underwater
overrides, reset and malformed rejection. Three scaled wolves plus independent
portrait remained present after warmup; zero asset/UI/weather failures. Minimum
free 39,180 KiB (38.26 MiB). Guest frame p50/p95/p99/max 33/34/34/35 ms; lighting
plus sky work 1/2/3/5 ms. Native checker: native-weather-state.weather-check.json.
QMP confirms 67,108,864 bytes, native scale 1, disposable disk writes. Frames
153..3833 exclude startup/preload. These resident-scene timings do not establish
streaming, crowded combat, live-server behavior or physical Xbox performance.
No controller input was injected; physical controller verification is separate.

Actual screenshots: native-weather-state-clear.jpg, -instant-snow.jpg and
-inspection.jpg. Matched clear/instant-snow images followed the timed capture
on the same unchanged disc; companion JSON identifies the observed phases.
They show lighting/fog changes with readable UI and independent portrait lighting,
not falling precipitation. Inspection shows the explicit indoor override. One
55-second wait missed the heavy-rain screenshot window; no heavy-rain screenshot
is claimed. Numerical rain transitions are covered by the full captured trace.

Frozen fixture: build/weather-state-test-20260916, source.zip, receipt, disc,
config, logs and capture-identity.json. All 294 source / 60 mounted outputs matched
before and after capture. Recording ended at duration_limit; only owned PID6688
was quit after executable/disc/QMP ownership verification. No emulator remains.
xemu logged a GLib g_source_destroy assertion during shutdown after the accepted
capture; retained stderr records it. It did not invalidate recorded guest samples.
The normal disc contains no fixture marker. No full-port verification.json was
created and the stable hardware package was not replaced.

Next substantial batch: bound avatar profile opening/appearance composition and
coalesce NPC animation requests, preserving visible complete assets until a staged
replacement is ready; measure full route/appearance costs in one combined run.
Then automatic environment selection and the reusable all-map asset pipeline.
The combined login/customization/HUD/combat gate remains pending while the shared
account is reserved for hardware testing. All nine classes, original UI workflows,
professions/social/travel/instances/PvP/audio and full PARITY.md stock-console
release gates remain. Full Vanilla parity is not complete. Overnight continuation
is ACTIVE every ten minutes, no former morning cutoff, no routine confirmations.


## September 17 actor cadence checkpoint (latest feature acceptance)

Candidate: build/candidates/20260917-actor-cadence. Native XBE 1,957,888 bytes,
normal XISO 499,515,392 bytes. All 294 source / 132 normal output identities and
294 source /63 mounted fixture outputs matched. Earlier candidates and the stable
hardware kit remain intact. Reserved account/server, characters and credentials
were not used or changed. Source ZIP and raw capture identities are preserved.

NPC idle/run requests now finish a pending partner for the same template instead
of repeatedly cancelling it. Optional whole clips are evicted before requested
allocations need their space; the last complete fallback stays protected. Shared
budgets and the 8 MiB guard remain. Distant idle/run skinning uses 66/100 ms buckets
beyond 35/80 camera units. Selected units and other clips retain full-rate samples;
the fastest visible instance controls a shared template. Gameplay updates are
unchanged. Adjacent parts sharing pose data reuse one matrix palette; the player
and previews retain full-rate animation. No extra matrix-cache allocation.

Targeted host checks: 3,829 stream, 360 runtime, 539 avatar, three telemetry tests,
and the real equipped Human through idle/run/attack/death and back passed. Tests
cover large multipart requests, cancellation/coalescing, atomic fallback, optional
clip eviction, memory pressure, cadence boundaries/priorities/time wrap and palette
reuse. WXN2 adds per-frame work counters in a 196-byte companion; older WXN1 still
decodes with unavailable counters explicitly -1. Native build passed.

One combined run, build/evidence/native-actor-cadence.csv: 6,500 main and 6,499
world/actor companions, frames222..6721. One complete boundary/Abbey/camp cycle
plus part of the next; eight complete NPC fallbacks after warmup, zero incomplete
player gaps, no asset/region/traversal failures across 862 clip requests. All four
clips observed. NPC pending cancellations stayed zero. Work quotas passed.
Distant work skipped on 2,467 frames, up to five batches/frame; the player used
one interpolated palette for twelve animated components. Actual screenshots
-inspection.jpg and -route.jpg were captured during the timed run. The latter has
phase/frame JSON; synthetic near/far units and their positions are not live gameplay.

Memory: minimum 32,456 KiB (31.70 MiB), QMP-confirmed 67,108,864 bytes, native
scale 1 and disposable disk writes. Guest frame p50/p95/p99/max:33/34/42/72 ms.
Actor streaming+skinning:5/7/22/40 ms; region max10 ms; world payload+animation
max26 ms. Peak NPC payload2,473,076 bytes, avatar payload+auxiliary1,203,140.
Native counters include static structure growth and retained optional clips.

The performance gate remains OPEN. The earlier actor fixture recorded a55 ms
maximum; this run uses different near/far placements and later rendering code,
so no overall speedup is claimed. The worst72 ms interval follows a frame with
both NPC and avatar reading64 KiB each, plus13 ms world work; other large spikes
also coincide with simultaneous clip/world streaming. Correlation is saved in
actor-cadence-stall-analysis.json. Per-scene quotas do not yet bound total work
across scenes. This is the next measured optimization target, alongside synchronous
avatar profile opening/composition. Startup/preload, live server combat, crowded
scenes, physical controller and stock-console performance remain outside acceptance.

Fixture build/actor-cadence-test-20260917 has source.zip, receipt, ISO, dedicated
config, logs and capture-identity.json. Capture ended at duration_limit; only owned
PID13676 was quit after executable/disc/QMP verification. No emulator remains.
xemu again logged a GLib g_source_destroy assertion during shutdown after the
accepted capture; the log is retained. No full-port verification.json and no
hardware-kit replacement. The normal disc has no fixture marker.

Next: coordinate world/NPC/avatar streaming work so simultaneous queues cannot
consume three full per-scene quotas in one frame; preserve required collision and
complete models while providing fair progress. Then stage avatar profile opening
and atomic appearance composition. Reuse pack_open.h but preserve full pack header
and contiguous animation-header reads; new profiles must never render the previous
character's geometry. Same-profile atlas changes need staged textures/families
committed together after required geometry is resident. Verify cancellation, memory
rejection and rapid customization/equipment changes in the combined feature batch.

General environment classification/all-map assets, live combined frontend/HUD/
combat (reserved account), all original systems and the full stock-console parity
and release gates remain. Overnight automation continues every10 minutes without
an old9AM cutoff. Full Vanilla parity is not complete.


## September 17 shared streaming budget checkpoint (latest feature acceptance)

Candidate: build/candidates/20260917-shared-stream-budget. XBE1,961,984 bytes;
normal XISO499,515,392. All295 sources / 132 normal outputs and295 sources /63
mounted fixture outputs matched. Earlier accepted candidates, stable hardware
kit, reserved server/account, saved characters and private assets/credentials are
preserved. No shared login or server change occurred.

The render thread now coordinates index/world/NPC/avatar queues under one frame
budget:64 KiB reads,16 read operations,64 KiB indexed-entry/payload validation and
three payload-allocation attempts. Active queues receive guaranteed shares;
absent queues donate work, and unused earlier shares flow forward. Fully resident
queues do not reserve I/O. New requests can defer one frame, then advertise demand
and receive progress. Collision remains first within world work; complete models
stay visible during replacement. Repeated calls cannot renew the frame budget.
The offline verifier rejects use inside a quota scope rather than looping forever.

WXS2 is212 bytes with total and per-queue work; WXS1 remains compatible with -1
for unavailable counters. The global index record includes the32-byte begin header.
Host checks passed:5,144 streaming,360 runtime,539 avatar and four decoder tests.
These cover three competing large payloads plus a staged index, guaranteed progress,
borrowing, deferred activation, repeated calls, cancellation, zero quota and memory
pressure. Native build passed. Details and exclusions are in STREAMING.md.

One combined native run: native-shared-stream-budget.csv,6,797 main and6,796 each
stream/actor companions, frames254..7050. Two complete boundary/Abbey/camp cycles;
eight complete NPC fallbacks after warmup, zero incomplete-player gaps and zero
asset/region/traversal failures across863 clip requests. Both NPC and avatar job
cancellations remained zero. Three obsolete world jobs were cancelled as part of
normal selection/preemption, without a traversal wait or failure.

Global observed maxima:65,536 read bytes,14 read operations,65,536 validation
bytes and three payload-allocation attempts per frame. Complete identity coverage
for every nonterminal main sample passes in shared-stream-budget-coverage.json.
This supplemental check is essential: strict companion decoding rejects invalid
quota values, so acceptance cannot rely only on the general checker's90% alignment
threshold. All6,796 expected identities are present; no rejected/missing frame is
hidden by that tolerance. Carry this stronger coverage gate into the next checker
source batch, or record explicit rejected-companion diagnostics.

QMP confirmed67,108,864 bytes, native scale 1, disposable disk writes. Minimum free
32,452 KiB(31.69 MiB). Guest frame p50/p95/p99/max33/34/37/49 ms; actor streaming+
skinning5/9/19/24 ms; region max10 ms; world payload+animation1/7/11/18 ms. The same
near/far fixture's matched frame254..6721 interval contains6,468 samples per run:
previous33/34/42/72 ms, now33/34/37/49 ms. Intervals over50 ms:15 to0; over40 ms:97
to17. Actor maximum40 to24 ms, with p95 rising7 to9 ms as work is spread over frames.
Comparison JSON records scope: loading waits can shift exact positions/phases;
these are xemu measurements, not full-game or physical performance claims.

The performance release gate remains open:49 ms is still over the33.3 ms target
frame budget. Profile opening and atlas composition, icons/title/preview/UI loads,
index allocation/commit/compaction, eviction, selection, skinning and GPU waits are
not all bounded by this coordinator. Startup/preload and live gameplay are outside
this capture. It does not replace crowded combat, physical controller, or actual
stock-Xbox validation. The full Vanilla gameplay/UI goal remains unchanged.

Actual -inspection.jpg and -route.jpg were captured during the run; route JSON
records the last observed telemetry identity before capture. Fixture folder
build/shared-stream-budget-test-20260917 preserves source.zip, disc, receipt,
config, logs and capture-identity.json. Capture ended at duration_limit; only owned
PID48052 was quit after executable/disc/QMP ownership checks. No emulator remains.
xemu again reported the post-QMP GLib g_source_destroy shutdown assertion; stderr
is retained. No full-port verification.json and no hardware-package replacement.

Next substantial feature batch: staged avatar profile opening and atomic appearance
composition, with quotas for metadata/layers/mip generation and correct cancellation.
Preserve contiguous animation-header reads and the full pack header when adapting
pack_open.h. New profiles must never render the preceding character's geometry;
same-profile texture/family changes commit only when required geometry is resident.
Keep original asset reference hashes and add rapid customization/equipment coverage.
Integrate strict full telemetry coverage as noted above. Then continue environment
classification, general map/asset coverage and full gameplay/UI parity. The combined
live frontend/HUD/combat gate remains pending while the account is reserved for
hardware. Overnight automation continues without the obsolete morning cutoff.


## 2026-09-17 - staged and atomic appearance composition accepted

Candidate: `build/candidates/20260917-staged-appearance`.
XBE 1,966,080 bytes, SHA256
`951da70fae81d77db2e04233073479b1d64ab35786808a1314dba164b285a659`.
Normal XISO 499,515,392 bytes, SHA256
`508d2bd1342e5cc8745a3d3c4938bed7e53b489ff0d96c3473d0c27e716954db`.
All 296 sources / 132 normal outputs and 296 sources /63 mounted fixture outputs
matched. Earlier candidates and their source/disc/capture identities, stable
hardware kit, reserved account/server, saved characters, assets and credentials
remain preserved. No shared login, server mutation or hardware transfer occurred.

Equipment and same-profile skin/face/hair changes now build in an unpublished
texture set. Reads and pixel work are staged; the complete old appearance keeps
animating until textures and all required geometry publish together. Rapid changes
cancel obsolete work. Removed equipment remains resident until replacement is
complete. Memory pressure or a rejected layer cannot publish half-written pixels
or a partial body. Failed profile selection still hides the preceding character.
Implementation, exact limits and follow-up design: docs/STAGED-APPEARANCE.md.

Reads: 32 KiB per call, 64 KiB/eight calls per update, charged to the shared AVATAR
lane. Compositing/hash/mip/copy/fill: 8,192 pixel work units per update. One extra
2-4 texture set adds 174,760-349,520 accounted bytes, guarded by free memory and
reused across changes. Planning/index lookups, profile opening and initial atlas
setup remain synchronous. These are work quotas, not fixed millisecond guarantees.

Host: 1,030 avatar, 5,144 streaming, 360 runtime and 517 preview checks, 80 legal
starter outfits, and six telemetry tests passed. All 3,229 original appearance
references across 16 race/sex profiles passed 361,104 checks, including unchanged
body/hair/extra hashes. A separate preserved host oracle compared every 87,380
bytes of 112 nonuniform mip chains against the unchanged synchronous mip helper,
with no remaining GPU allocations. The largest reference needed 40 staged updates,
including cold geometry. These are host results, not all-race native acceptance.

One combined WXPF0011 native capture: `native-staged-appearance.csv`, 6,848 main
and 6,847 each stream/actor companions, frames 194..7041, two complete boundary/
Abbey/camp cycles. Twenty-seven published appearances including the initial one
(26 changes in the captured interval), 50 cancelled compositions, three distinct
body hashes. Eight complete NPC fallbacks, zero player gaps and zero composition,
asset, region or traversal failures across 863 animation requests. One obsolete
avatar payload job was cancelled without dropping the published appearance.
Assertions checked published pointers, dispersed GPU texels, family keys and
complete geometry. This is synthetic state over real assets, not gameplay/input.

QMP: 67,108,864 guest bytes, native scale 1, disposable -snapshot writes. Minimum
free 32,136 KiB (31.38 MiB). Guest frame p50/p95/p99/max 33/34/36/50 ms; actor
streaming/composition/skinning 5/12/15/24 ms. World payload/animation 1/7/9/17 ms;
region maximum 18 ms. Shared maxima: 65,536 read bytes, 14 operations, 65,536
validation bytes, three allocation batches. Composition maxima: 65,536 bytes,
seven reads, 8,192 pixel work units. Peak avatar payload + auxiliary: 1,556,044 B.

WXS2/new WXN3 companions cover every nonterminal main time/frame identity: no
missing, duplicate or foreign rows. The --frame-budget checker now enforces this;
strict-decoder rejection or dropped UDP cannot hide behind 90% alignment tolerance.
Legacy WXN1/WXN2 remain readable. --output allows rechecks to preserve older reports.
The prior shared-budget capture's complete file manifest was reverified unchanged.

Actual -inspection.jpg and -route.jpg show different hair appearances on the hill
and Abbey exit. Route JSON is the last flushed identity before the screenshot.
`build/staged-appearance-test-20260917` retains source.zip, receipt, disc, config,
logs and capture-identity.json. Capture ended at duration_limit. Only owned PID
4852 was quit after executable/disc/QMP ownership checks; no emulator remains.
xemu again logged a post-QMP GLib g_source_destroy shutdown assertion; guest
samples had no failures. No full-port verification.json or hardware replacement.

The 50 ms maximum still exceeds the 33.3 ms target. Added appearance workload means
this is not a controlled speedup comparison with the earlier route. Startup/profile
opening before frame 194 is excluded. Full-world 30 fps, crowded combat, combined
frontend/live gameplay, physical controller and stock hardware gates remain open.
The full Vanilla gameplay/UI goal is unchanged.

Next substantial batch: staged profile pack/metadata/looks opening, cancellation,
full header preservation, contiguous animation-header reads and a bounded family
index. Exercise cold/rapid roster/race/sex changes; keep wrong-character geometry
hidden and preserve atomic same-profile publication. Then continue environment
classification, general map/assets and remaining gameplay/UI coverage. The shared
account remains reserved for hardware, so combined live frontend/HUD/combat
acceptance stays pending. Overnight development continues without the old cutoff.

## Checkpoint - 2026-09-17 staged character-profile loading

Accepted scoped feature batch: `build/candidates/20260917-profile-loading`.
Full Vanilla parity remains incomplete. This batch implements production loading
and cancellation across character profiles; it does not complete character-screen
workflows or live gameplay acceptance.

XBE 1,974,272 bytes, SHA256
`e41f2023a5c134748230f0a281e894b5aaa1321c02035a0faea4a7ecf269a160`.
Normal XISO 499,515,392 bytes, SHA256
`a2e6ac12af04bf9a939d4ceeea14721dab6c8cbdaf5de50af3b314a639056427`.
The 302-source/ 132-output normal receipt and 302-source/59-output mounted fixture
receipt matched after capture, before the recorder-only correction below.

Profile pack headers/tables, animation headers, appearance metadata, looks catalog,
bindings and item references now load through cancellable stages. The validated
table transfers to an empty scene with its complete original header. Contiguous
animation headers use a bounded bulk read. A 544-byte family membership table
replaces repeated full-entry scans. Production skips the redundant synchronous
initial-atlas composition; the staged compositor publishes the first complete body.
Changing race/sex cancels obsolete jobs and suppresses old-character geometry.
Same-profile edits retain the previously accepted atomic appearance behavior.

Each pump caps reads at 32 KiB/call, 64 KiB/eight calls and validation at 64 KiB,
within the shared AVATAR lane. Allocation batches require quota and the 8 MiB
free-memory guard. Looks validation handles at most 32 rows/pump, charging duplicate
comparisons. Pending index buffers and frontend avatar resources are accounted.
Preview selection/composition opens an AVATAR scope when no world scope is active;
title/backdrop/icons still need coordinated frontend budgeting. Synchronous tooling
drains the same validators outside a frame scope and preserves its initial-atlas
contract. Details: docs/PROFILE-LOADING.md.

Host: 1,467 avatar, 5,144 stream, 360 runtime, 901 looks and 517 preview checks;
all 80 legal starter outfits. All 3,229 original reference appearances across 16
profiles passed 364,481 checks. Equipped Human geometry and four clips passed.
After the recorder fix, all 11 telemetry decoder/recorder tests passed, including
an actual mocked-socket-to-CSV regression. Native XBE and both images built.

One combined WXPF0012 native run: 6,832 main and 6,831 WXQ1 companion samples,
frames 241..7072. Every nonterminal main identity has exactly one companion;
zero missing, foreign or duplicate identities. All 16 race/sex profiles rendered
with starter outfits. Counters reached four complete cycles, 96 cancelled pending
selections, four deliberately missing-profile failures and successful recoveries.
No unexpected selection/composition/asset failures, stale character identity or
partial-profile draws. Four cycles and counters include the short pre-capture
interval; all sixteen visible profiles are independently present in the capture.

QMP confirmed 67,108,864 guest bytes, native scale 1 and disposable -snapshot
writes. Minimum free 41,660 KiB (40.68 MiB). Guest frame p50/p95/p99/max:
33/34/34/35 ms. Profile loading/composition work: 1/9/10/17 ms. Shared maxima:
65,536 read bytes, eight read calls, 65,536 validation bytes and two allocation
batches. Maximum accounted avatar bytes: 1,840,492. Contiguous animation indices
used one read in these prepared profiles. This is isolated character rendering,
not world traversal or crowded gameplay, and is not a controlled speedup comparison
with the previous 50 ms world maximum. Frames before 241 and OS-cold disk latency
are not covered. Files are reopened after each released profile.

Actual `native-profile-loading-pending.jpg` shows loading with no incomplete body;
`native-profile-loading-nightelf.jpg` shows the equipped Night Elf. Fixture and
source/disc identities are preserved in `build/profile-loading-test-20260917`.
The run ended at duration_limit. Only owned PID 3128 was quit after executable,
disc and QMP ownership checks. No emulator remains. xemu again logged its post-QMP
GLib g_source_destroy assertion; captured guest samples had no unexpected failures.
The fixture uses synthetic selection over real assets, without controller input,
input replay, authentication, server mutations or physical hardware validation.

### Evidence correction, fully preserved

The recorder accidentally reused PROFILE_FIELDS for its existing ten performance
columns. Native WXQ1 values were decoded correctly, but the companion CSV received
the wrong header. Original captures remain unchanged. A derived
`native-profile-loading-recovered.profiles.csv` replaces only the header; every
data byte is identical. Each of its 6,831 rows was reassembled into a WXQ1 packet
and passed the strict decoder. The accompanying main CSV is a byte-identical copy.
`profile-loading-header-recovery.json` records hashes and the transformation;
`native-profile-loading-recovered.profiles-check.json` records passing acceptance.

After source/disc post-verification and emulator shutdown, only
tools/record_telemetry.py and tests/profile_telemetry_tests.py changed: a distinct
PROFILE_LOAD_FIELDS name and the regression. All 132 normal output identities,
including XBE/XISO, remained identical. Both source snapshots and receipts are
preserved: candidate source.zip is final tooling; capture-source.zip is the code
used during capture. `profile-loading-source-delta.json` proves this tools-only
delta. No corrected-recorder native run is claimed or needed for unchanged binaries.

### Next substantial work and gates

Continue reusable map/dependency preparation, including global-WMO maps, automatic
environment classification and remaining world-startup/frontend loading work.
The existing catalog has 23 terrain maps/2,429 tiles and 21 global-world-model maps;
developer/unreleased maps must be distinguished from required playable content.
Four terrain packs remain prepared; catalog enumeration does not mean map support.
Use pinned WoWee parsers and shared systems rather than per-zone implementations.

Combined live frontend/customization/HUD/combat acceptance remains pending while
the shared account is reserved for hardware. Full-world timing, all classes,
original UI workflows, full maps, audio/effects, mixed endurance and stock hardware
remain release gates. Stable hardware kit, 20260916-1212 checkpoint, previous
candidates, saved characters, private assets and credentials remain preserved.
No hardware transfer, server restart, full-port verification.json or stable-package
replacement occurred. Overnight development continues with no old morning cutoff.

## Checkpoint - 2026-09-17 global world-model pipeline and routing

Scoped candidate: build/candidates/20260917-global-world. Full Vanilla parity
remains incomplete. The reusable converter and native streamer now accept global
WMO maps alongside terrain maps; complete instance gameplay is not implemented.
Implementation, report format and open limitations: docs/GLOBAL-WORLD.md.

Normal XBE: 1,974,272 bytes, SHA256
f3f7e32c7cc272a092599034f21d665f5217a09aef56326934e83b778aac54c0.
Normal XISO: 499,515,392 bytes, SHA256
0fd7a39805a773b45c6c952a52dc2debba3df186277efc1f5749881e0f483975.
All 309 sources / 132 normal outputs and 309 sources /14 mounted fixture outputs
matched after the capture. Source/disc identities stayed frozen throughout.
The normal disc retains its existing terrain index; five global packs are on the
separate fixture disc. No stable hardware package was replaced.

Strict WDT validation now covers map-wide root paths/placements. The pinned WoWee
WDT/WMO/M2/BLP interpretation feeds short G###.WXP files. Zero-position instance
origins and rotation order/signs match WoWee's world loader. Version-2 region
indices identify global maps explicitly, retain v1 compatibility and reject
mixed/duplicate map identities. Native map changes cancel pending work and detach
old geometry before loading the selected global pack through existing quotas.

The catalog validates all 44 WDTs: 23 terrain maps /2,429 tiles and 21 global-model
maps, including developer/unreleased content. The second conversion batch prepared
20 global packs (415,761,298 bytes) and verified every payload through the runtime
loader. Dependency/unsupported-feature reports are hashed alongside each pack.
The largest file is 59,448,740 bytes; largest index 8,343 entries.
Map 450, HordePVPBarracks, still fails finite-vertex validation. Its failure remains
in the batch report and it is excluded from the index.

Four legitimate geometry-free particle doodads initially caused whole-map failure.
They are now recorded as unsupported effects, with their source dependencies,
instead of being called malformed meshes. Rendering those effects remains required.
The first conversion's erroneous zero-origin terrain offset is also preserved as
rejected evidence; corrected transforms were checked against pinned WoWee.
Do not use build/global-worldpacks-20260917. Corrected assets and reports are in
build/global-worldpacks-v2-20260917. Nothing was staged into the normal world index.

Host: 153 DBC/WDT, 405 runtime/collision/allocation and 16 scene/transform checks,
plus malformed mixed-index tests and 12 telemetry tests. New region tests cover
terrain/global/missing-map transitions and global selection across grid boundaries.
The original terrain-only ground probe returned no floor on all five global maps;
that failed probe is preserved. The corrected height-aware, frame-budgeted --point
probe passed all five maps with valid original collision and no allocation leaks.
Its simulated memory figures are not console measurements.

One combined WXPF0013 capture: native-global-world.csv, 6,866 main and 6,865
stream companions, frames 210..7075. Complete nonterminal identities: zero missing,
foreign or duplicate records. Six completed map cycles (counter includes the short
pre-capture interval), with every map independently represented in the captured
data. Stockades (34), Wailing Caverns (43), Deeprun Tram (369), Ragefire Chasm (389)
and Molten Core (409) each reached 120 frames with resident selected geometry,
original collision floor and visible batches. No asset, region or fixture failures.
This is synthetic switching among diagnostic surfaces, not live entry or traversal.

64 MiB guest RAM confirmed by QMP, native scale 1, disposable -snapshot writes.
Minimum free 36,184 KiB (35.34 MiB). Guest frame p50/p95/p99/max 33/34/34/35 ms;
index work 0/0/5/10 ms; payload streaming 1/11/15/19 ms. Shared maxima: 65,536
read bytes, eight reads, 65,536 validation bytes, three allocation batches. Peak
scene residency 6,461,782 bytes; current/pending index each peaked at 373,504 bytes.
This fixture omits actors, combat, interface workflows, audio and travel. It does
not demonstrate full-world 30 fps or physical hardware performance.

Actual screenshots preserve partial cavern/Tram loading, resident Tram geometry,
and a later cavern view after telemetry ended. The Tram inspection point lies in
the underwater scene and exposes missing water/material presentation; these images
are evidence of limitations as well as geometry. Current 256-render/64-collision
residency caps can omit dense content. A passing routing/collision probe does not
prove complete geometry, authentic visuals, navigable entrances or instance logic.

The capture ended at duration_limit. Only owned xemu PID 48136 was quit after
executable/disc/QMP ownership checks; no emulator remains. xemu again logged its
post-QMP GLib g_source_destroy assertion. Earlier startup QMP refusal occurred
before the listener was ready; the later memory query passed. Native build first
failed under Windows PowerShell 5's missing RNG Fill, then on a nonexistent
telemetry struct member; both invocation/compiler logs are preserved. The final
PowerShell 7 build and native run passed.

Next substantial batch: improve general WMO material/geometry coverage and spatial
selection, investigate rejected map 450 geometry, and carry environment metadata
into automatic interior/water classification. Improve default loading presentation
and use verified entrance/route points before claiming navigable maps. Expand the
same pipeline across remaining terrain content while continuing original UI and
class/gameplay systems. Shared-account live frontend/HUD/combat acceptance remains
pending while the account is reserved for hardware.

All earlier candidates/captures, stable hardware kit, saved characters, private
assets and credentials remain preserved. No account login, server restart/mutation,
hardware transfer, physical controller test or full-port verification.json.
The full gameplay/UI goal and stock hardware gates remain unchanged. Overnight
autonomous development continues without the old morning cutoff.


## Checkpoint - 2026-09-17 original world material semantics

Scoped candidate: build/candidates/20260917-world-materials. Full Vanilla parity
remains incomplete. WORLD-MATERIALS.md describes the implementation and limits.

The converter/native renderer now preserve seven original blend modes, unlit and
unfogged materials, and M2 depth-test/depth-write flags. Opaque/cutout batches draw
before bounded, stable back-to-front transparent batches. Material fog preserves
alpha and approaches the neutral additive/multiply colour. State is restored for
following actors, UI and previews. WXP v6 adds no entry/vertex size increase;
legacy v4/v5 packs remain readable. Sorting uses 2,560 stack bytes with no new
render target. Existing avatar/NPC packs and normal terrain index are unchanged.

Twenty global packs were regenerated in build/world-material-packs-20260917,
415,761,298 bytes total. All 57,991 entries retain exact geometry and mip pixels;
5,397 material entries changed. Reports count 1,581 alpha-blended, 2,521
alpha-additive and four double-multiply batches, plus opaque/cutout geometry.
No real mode-3/mode-5 batches occur in these twenty packs; the synthetic chart
covers those paths. Unsupported shaders/effects remain explicitly reported.

Map 450 still fails. Investigation identifies original WMO group
kl_pvpbarracks_010.wmo, raw vertex 9601, with two NaN UV components. Four
nondegenerate triangles reference it. No geometry was discarded or source
coordinates fabricated. The raw group, precise diagnostic and triangle evidence
are retained privately. A documented repair or clean-source comparison remains.

Host checks: 369 material arithmetic/fog/order, 453 runtime/version/allocation,
1,467 avatar, 5,144 staged-streaming, 20 scene/adapter and 42 repacking checks;
11 telemetry tests. An initial repacker check rejected the new version because
its independent comparison still required v5; fixed to support v5/v6 without
allowing a material-format downgrade. Original failed log and passing rerun saved.

One combined native capture: native-world-materials.csv, 7,359 main samples and
7,358 each streaming/material companions, frames 239..7597. Both companions have
complete nonterminal identity coverage, no foreign/duplicate samples. Five cycles
completed (counter includes pre-capture time). All five original inspection maps
reached resident geometry/collision holds; synthetic map 999 rendered all seven
modes and all new state flags, with 905 fully held fog-on/off probe samples.
No asset/region/fixture failure or stream quota violation. This is synthetic map
switching and material submission, not input replay, gameplay or pixel-exact PC
comparison. No physical controller was tested.

64 MiB guest confirmed, native scale 1, disposable disk writes. Minimum free:
36,180 KiB (35.33 MiB). Guest frame p50/p95/p99/max: 33/34/34/135 ms. The only
interval over 40 ms followed frame 626 on the first Tram visit: draw 123 ms,
stream 10 ms, total work 134 ms, one double-multiply batch submitted. Later
visits did not reproduce that spike. First-use emulator pipeline creation is
only a hypothesis; the cause is not established and the spike is not excluded
from results. No full-world 30 fps or hardware performance claim.
Index work: 0/0/4/8 ms; payload work: 0/10/12/16 ms. Shared maxima remained
65,536 read bytes, eight read operations, 65,536 validation bytes and three
allocation batches. Peak scene 6,461,782 bytes; index/pending index 373,504 bytes.

Actual screenshots show the fog-off/fog-on material chart with an unaffected UI,
loading Molten Core and resident Tram geometry. The Tram view still has bright
light cards and missing underwater/environment presentation: M2 colour/opacity
tracks, complex shaders and local lighting/water are not solved by blend factors.
Material sorting is per draw call; intersecting triangles/cross-actor transparency,
face culling, complete geometry selection and original visual parity remain open.

Normal XBE: 1,978,368 bytes, SHA256
056eaa3ca6bb3e6d57e1bc620e7f3fc0bf33cddb587daff87be14f854dd83d4c.
Normal XISO: 499,515,392 bytes, SHA256
89b52aed0a28a1670f6d3cd73cb46de25e87e2aba69c8c1c74086cf55e647d8e.
All 316 sources / 132 normal outputs and 316 sources / 16 mounted fixture outputs
matched after capture. Native sources/disc stayed frozen throughout. The capture
ended at duration_limit; only owned PID 58028 was quit after executable/disc/QMP
ownership checks. No emulator remains. Its post-QMP GLib shutdown assertion is
preserved. Stable hardware kit, earlier candidates, credentials, private assets,
shared-account progress and server processes remain untouched.

Next substantial batch: reuse original M2 colour/opacity/texture-track evaluation
for world doodads and address the bright material cards; improve spatial selection
and WMO interior/water classification. Profile the measured first-use draw stall
without hiding it in a warm-only performance number. Investigate a documented
map-450 UV repair/clean source; continue general terrain conversion, full original
UI and all-class gameplay. Shared-account combined frontend/HUD/combat acceptance
and all physical-hardware gates remain pending. Overnight autonomous development
continues with no morning cutoff. No full-port verification.json or stable-package
replacement was made.


## Checkpoint - 2026-09-17 original material colour and opacity motion

Scoped candidate: build/candidates/20260917-material-motion. The full port remains
incomplete. MATERIAL-MOTION.md records the format, acceptance and remaining limits.

The converter now reuses original Vanilla colour/alpha/texture-weight data and
pinned WoWee texture-translation tracks. Exact step/linear keys keep independent
local/global periods; constant tracks fold into defaults. Native world rendering
samples these tracks, applies colour and opacity, handles authored U/V wrapping,
and restores material state for following actors, UI and previews. Fully hidden
batches are skipped. WXP v7 adds an optional 2,652-byte resident material block
without changing entry/vertex size; v4/v5/v6 remain readable. Allocation, bounded
read/validation, cancellation and rollback use the existing shared stream budget.
Pack optimization preserves the metadata and exact geometry. WXM2 telemetry
records submitted motion blocks and sampled material hashes.

Twenty packs in build/material-motion-packs-v2-20260917 total 419,394,538 bytes.
All 57,991 entries retain exact geometry and mip pixels. Added metadata comprises
1,370 blocks (3,633,240 bytes on disk), with 78 animated batches: 40 Blackfathom,
38 Dire Maul. None of these twenty maps has translated UV tracks or nonidentity
UV rotation/scale; UV translation has synthetic coverage only. Actor material
clip integration, skeletal doodad motion, multi-texture shaders, vertex colours,
local lighting, water and complete spatial selection remain unfinished.

The first conversion attempt rejected five maps for inert UV lookup slots when
the model had no texture-transform table. The pinned renderer treats the absent
table as a no-op; the converter now does too, while still rejecting invalid
bindings into a present table. The rejected attempt and corrected v2 packs are
preserved. Map 450 still fails on its original referenced NaN UV; no geometry was
dropped or invented. That diagnostic remains a separate content blocker.

Host checks passed: 5,071 material motion, 453 runtime, 1,467 avatar, 5,144 staged
streaming, 44 repacking and 11 Python telemetry tests. Native build passed. One
combined offline capture used original maps 34, 43, 369, 389, 409, 429 and the
separate synthetic chart 999. Dire Maul's diagnostic point was selected beside
an authored animated material and checked against original collision triangles;
this does not establish an entrance, navigation route or instance gameplay.

native-material-motion.csv: 7,062 main samples, frames 206..7267, and 7,061 each
streaming/material companions with complete nonterminal identity coverage; four
full cycles. All seven maps reached resident collision/render holds. No asset,
region or fixture errors, and no quota violations. The held synthetic chart
yielded 724 samples across fog-on/off, 14 submitted motion blocks and 659 distinct
uniform hashes. Selected Dire Maul holds yielded 164 authored-motion samples and
162 hashes. This verifies native submission and variation, not pixel-exact PC
visual parity. No account, controller input, combat or saved state was used.

Confirmed guest RAM: 67,108,864 bytes; native scale 1 and disposable disk writes.
Minimum free: 35,912 KiB (35.07 MiB). Guest frame p50/p95/p99/max: 33/34/34/43 ms.
The single interval above 40 ms followed frame 3282, while loading the synthetic
fog-on chart: draw 38 ms, streaming 2 ms, total work 42 ms. Cause remains unknown.
This is not a matched benchmark against the prior 135 ms first-use spike; neither
capture establishes full-world performance or physical Xbox timing.
Index work: 0/1/5/9 ms; payload work: 0/10/14/19 ms. Shared maxima: 65,536 read
bytes, eight reads, 65,536 validation bytes and three allocation batches. Peak
scene residency 6,464,434 bytes; current/pending index each 533,952 bytes.

Actual native screenshots show changed chart colour/opacity, fog state, Stockades,
an original Dire Maul brazier and the Tram's remaining excessive brightness and
missing water. The first chart image belongs to the telemetry interval; filenames
ending postcapture were taken afterward with the same frozen source/disc. The
motion chart's cutout column stays below the original alpha-test threshold and
is discarded; visible cutouts have separate v6-chart evidence. No pixel-exact
comparison or physical-controller verification is claimed.

Normal XBE: 1,982,464 bytes, SHA256
87ee6bd443bc08725fcc1df6740c3fb40805c7af95c78ccddd57c9d067c4da18.
Normal XISO: 499,515,392 bytes, SHA256
2336d5d4283440a1a860317ecf2bc1b6a48e161c60eb8fe0511219b746cef048.
All 319 sources /17 mounted outputs and 319 sources / 132 normal outputs matched
after telemetry; sources/disc stayed frozen through the subsequent screenshots.
The source ZIP, dependency receipts, accepted and rejected assets are preserved.
Capture ended at duration_limit. Only owned xemu PID 16132 was quit after exact
executable/disc/QMP listener checks; no emulator remains. Shared account/server,
private assets/credentials, saved characters, stable hardware kit and all earlier
checkpoints remain unchanged. No full-port verification.json or hardware package
promotion was made.

Next substantial work: retain original WMO environment/material/lighting metadata
in the general converter and bounded streamer, improve the bright Tram scene and
spatial selection, then extend the same pipeline across terrain content. Continue
original UI and all-class gameplay without reducing the goal. Actor material
clips, map-450 repair/clean-source comparison, shared-account combined frontend/
HUD/combat acceptance and every physical stock-hardware release gate remain open.
Autonomous overnight development continues without the old morning cutoff.


## Checkpoint - 2026-09-17 authored WMO vertex lighting

Scoped candidate: build/candidates/20260917-vertex-lighting. Full Vanilla parity
remains incomplete. VERTEX-LIGHTING.md contains the implementation and evidence.

Original WMO MOCV RGB now reaches the native shader. The converter follows pinned
WoWee's indoor MOHD ambient-floor and outdoor tint policy, quantized to RGBA8.
Interior batches bypass outdoor sunlight. WMO clamping is retained; MOCV alpha
does not become surface opacity. Missing colour streams and subsequent draws use
explicit constant white. WXP v8 adds optional four-byte-per-vertex colour arrays
before the existing motion block without changing geometry/header/entry size.
Only resident arrays allocate; the scene/pending/free-memory budgets include
them, reads use the shared quota, and publication waits for the full payload.
Cancellation, eviction, truncated files and partial allocations roll back safely.
Repacking preserves colour/motion data. Legacy v4..v7 remain readable.

Twenty packs in build/vertex-lighting-packs-20260917 total 426,894,490 bytes.
6,116 coloured batches, 6,374 baked-light batches, 1,874,988 colour vertices add
7,499,952 bytes on disk. All 57,991 entries retain exact geometry/indices, mip
pixels and v7 material-motion data. Map 450 remains rejected for its original
referenced NaN UV. No geometry was dropped or invented. The first comparison
summary confused colour bytes with total pack bytes; its geometry comparison
passed. The original field-error report is preserved, and the corrected report
uses distinct pack_bytes and colour-byte fields.

Host checks passed: 2,170 vertex-lighting/chunk-boundary/publication/rollback,
5,071 material-motion, 453 runtime, 1,467 avatar, 5,144 staged streaming; 50
repacking checks and 14 Python telemetry tests. Both host and native builds pass.

One combined offline native run: native-vertex-lighting.csv, 6,008 main samples,
frames 237..6244, and 6,007 each streaming/material companions with complete
nonterminal identity coverage. Three full cycles cover original maps 34, 43,
369, 389, 409, 429 plus synthetic chart 999. All reached resident render/collision
holds. No asset/region/fixture errors or quota violations. The held gradient chart
has 543 fog-on/off samples; original WMO baked/colour flags also reached draws.
Original Dire Maul material animation continues (123 selected held samples,
121 uniform hashes). These are offline diagnostic map switches, not travel,
controller input, gameplay, original-client pixel matching or hardware acceptance.

Confirmed guest RAM 67,108,864 bytes, native scale 1 and disposable disk writes.
Minimum free 35,680 KiB (34.84 MiB). Guest frame p50/p95/p99/max: 33/34/34/44 ms.
The sole interval over 40 ms followed frame 3294 on the first fog-on chart visit:
draw 40 ms, stream 0 ms, work 43 ms. Cause remains unproven; all cold samples are
retained. This is not a matched benchmark against prior batches, sustained
full-world 30 fps or physical Xbox performance.
Index work: 0/0/4/7 ms; payload work: 0/10/11/14 ms. Shared maximum use remained
65,536 read bytes, eight reads, 65,536 validation bytes, three allocation batches.
Peak scene residency 6,577,050 bytes; current/pending index each 533,952 bytes.

Actual screenshots verify RGB channel order, interpolation, lit/baked intensity,
material blending, fog and unchanged backing-board/UI colour. Dire Maul walls
now show authored blue lighting and the Tram bridge shows shaded tint. Overbright
Tram cards and its underwater scene remain visibly incomplete. The fog-on chart
and Tram images named postcapture were taken after telemetry with the identical
frozen source/disc; Dire Maul and fog-off chart belong to the capture interval.

Normal XBE: 1,982,464 bytes, SHA256
f571f915629a66400cd38b9f9ec120372101c1ecd768b0555a7f286f07dba225.
Normal XISO: 499,515,392 bytes, SHA256
3238a21abb6b248a18c29eb38374973bd90213c9f91859a514944cc9642157fc.
321 source identities and 17 mounted outputs matched after capture/screenshots;
321 sources and 132 normal outputs also matched. The source ZIP, receipts,
private dependencies and exact rejected-map diagnostic are preserved. Capture
ended at duration_limit. Only owned xemu PID 3852 was quit after executable/disc/
QMP checks; no emulator remains. Its post-QMP GLib shutdown assertion is retained.
Earlier checkpoints, stable hardware kit, normal terrain/actor asset index,
shared-account characters, private assets/credentials and server state remain
unchanged. No stable-package promotion or full-port verification.json was made.

Next substantial work: general WMO environment and underwater classification,
local lighting/complex materials/water, spatial selection and broader terrain
preparation, alongside original UI and all-class gameplay. WoWee's lighting
interpretation is not the original client's complete colour-alpha fixup; exact
visual parity remains open. Actor material clips and live combined frontend/HUD/
combat acceptance remain pending, with the shared account reserved for hardware.
All stock-console memory/performance/controller release gates remain open.
Autonomous overnight development continues without the old morning cutoff.


## 2026-09-17 — source-derived WMO environments accepted

Candidate: `build/candidates/20260917-environment`. WXP v9 world metadata now
classifies the collision-adjusted camera using original rigid group bounds and
MLIQ heights/holes. Interior boxes suppress outdoor sky/fog; water/ocean selects
the water palette and a documented fallback when authored lighting is absent.
Magma/slime/unknown types remain explicit unsupported presentation data. Metadata
loads atomically under the existing shared quotas and memory guard, independent
of the number of models drawn. Legacy packs report unknown coverage.

Twenty global packs retain all 57,991 entries' exact v8 geometry/index/pixel/colour/
motion bytes. 721 group volumes and 69 liquid grids add 562,268 metadata bytes.
The original largest grid axis is 139; a bounded 256-tile metadata limit fixed
four initial 64-tile-limit failures. Rejected initial outputs/logs remain.
Map 450 retains its original NaN-UV blocker. See ENVIRONMENTS.md for format,
validation, original-data interpretation and limitations.

One combined offline xemu capture passed: 9,179 main samples, frames 210–9388,
and 9,178 each stream/environment/lighting companions with complete nonterminal
identity coverage. All twelve cases completed; the cycle counter reached four.
No asset, region, classification, fixture or shared-quota errors occurred.

Confirmed 67,108,864 bytes guest RAM, native rendering scale 1 and disposable HDD
writes. Minimum free memory: 31,648 KiB (30.91 MiB). Guest frame p50/p95/p99/max:
33/34/34/193 ms. The sole interval over 40 ms followed frame 278, the first outdoor
fog draw in Stockades: draw 186 ms, stream 3 ms, work 192 ms. Cause is unproven;
cold GPU pipeline creation is only a hypothesis. All cold samples remain in the
report. The 30 fps/stall release gate is not met; these are different camera
workloads from prior material captures, not a matched performance comparison.

Environment queries measured 0/0/1/1 ms at p50/p95/p99/max. Maximum metadata
residency in this native fixture was 55,880 bytes. Region-index work measured
0/0/4/11 ms; payload work 0/11/17/20 ms. Peak scene residency was 10,154,346 bytes;
current/pending index capacity each reached 373,504 bytes. Shared maximum use was
65,536 read bytes, eight reads, 65,536 validation bytes and three allocation
batches. These are xemu guest times, not physical Xbox performance.

Actual screenshots show Stockades interior classification, an outside-box Wailing
Caverns diagnostic view, and blue fog below Blackfathom's original water grid.
The above-water Blackfathom image is post-capture with the identical source/disc.
It shows fog disabled and also clearly exposes missing liquid surfaces/nearby
geometry from this synthetic camera position. No claim of normal player-view
fidelity or valid walkable camera placement is made. UI remains readable through
the environment changes; no controller input was injected or physically tested.

Capture ended at duration_limit. Only owned xemu PID 12644 was quit after exact
executable, mounted-disc and QMP-owner checks. No emulator remains. Its post-QMP
GLib shutdown assertion is retained. 328 source hashes and 15 mounted outputs,
and 328 sources with 132 normal outputs, match the frozen receipts. The source
ZIP and all private asset/camera-case dependencies also match.

Normal XBE: 1,990,656 bytes, SHA256 `8fb98c37d631eb9c4bb7262a6b68b82eb31b714b71d29c1e198ae4c6fc440e4e`.
Normal XISO: 499,515,392 bytes, SHA256 `78c787b2004ed835656ab041e9598681bcad03fde483897450f35fdd7589f0d0`.

After native capture ended, the general tile converter also prepared map 0 tile
32/48 in `build/environment-terrain-20260917`: 169,821,468 bytes, 346 WMO group
volumes and 23 liquid grids (369 records, 69,252 metadata bytes). Full runtime
asset verification passed on the host. This candidate terrain pack has no native
route acceptance and does not replace the normal installed terrain index.

Host checks: 464 environment, 453 runtime, 1,467 avatar, 5,144 streaming,
1,715 fog, 20,831 lighting/clock, 55 repacking and 16 telemetry tests. Failed
initial include-path, liquid-type expectation and obsolete fog expectation logs
remain; corrections and final results are explicit in ENVIRONMENTS.md.

The stable hardware kit, prior candidates, normal terrain/actor asset index,
shared account/characters, private assets/credentials and server state remain
unchanged. No stable-package promotion or full-port verification was made.
Next: accept terrain WMO metadata on native routes, implement original water
surfaces and terrain liquids, improve camera/environment/portal precision and
cold-draw stalls; continue original UI and all-class gameplay while shared-account
live acceptance remains reserved for hardware. Exact local fog, swimming, water
effects, stock hardware/controller tests and full-game release gates remain open.
Autonomous work continues without the old morning cutoff.


## 2026-09-17 original WMO liquid surfaces checkpoint

Candidate: `build/candidates/20260917-liquids`. Full Vanilla parity is unfinished.

The general WMO converter now emits original MLIQ heights and hole masks as
bounded 16x16-cell surface patches, retaining placement IDs. WXP v10 carries
30-frame original lake/ocean/lava/slime texture sequences with aligned mip chains.
The existing loader publishes a whole sequence atomically and shares it among
patches. Playback selects a resident GPU address with no per-frame allocation or
upload. Water/slime opacity initially follows pinned WoWee policy; magma is opaque,
unlit and unfogged. A shared liquid pass follows actors/player and restores state
before the UI. Normal underwater lighting telemetry now uses the water condition.

Twenty global packs prepared: 436,983,970 bytes, 386 new surface patches,
47,327 enabled liquid cells / 94,654 triangles. All 57,991 earlier entries retain
exact IDs, geometry, vertex lighting, material motion and texture bytes; environment
metadata remains exact. Added bytes total 9,526,892, including ten per-pack shared
sequences. Global content here contains water and magma. Ocean/slime, terrain
liquids and compressed-sequence native playback remain unverified. Map 450 still
fails on the preserved original NaN UV, without silently changing its geometry.

One combined 64 MiB, native-scale xemu capture covered fourteen original-data
camera cases across Stockades, Blackfathom, Blackrock Depths and Dire Maul.
8,673 main / 8,672 stream, material, environment and lighting samples span frames
240..8912; two complete case cycles, no fixture/asset/region errors, exact companion
identity coverage. All 30 texture frames submitted on each of the three liquid
maps; maximum water/magma patch draws 11/9/1 on maps 48/230/429 respectively.

Minimum measured free memory: **30,296 KiB / 29.59 MiB**. Guest frame p50/p95/p99/max:
**33/34/34/55 ms**. The sole interval above 40 ms, frame 279, follows first Stockades
outdoor draw frame 278 (48 ms draw, 2 ms stream, 54 ms work). The cause remains unproven;
this is not a controlled comparison against earlier cold-draw stalls. Index work
p50/p95/p99/max 0/1/5/11 ms; payload 1/15/17/20 ms; environment query 0/0/1/1 ms.
Maximum scene residency 11,754,134 bytes, current/pending index 534,016 bytes,
metadata 134,828 bytes. Shared maxima 64 KiB read, eight reads, 64 KiB scan, three
allocation batches. These are xemu guest measurements, not physical hardware.

Actual screenshots show original water in Dire Maul and bright magma in Blackrock
Depths. A Blackfathom above-water view exposes remaining cutoff/geometry and
appearance limitations. A below-water Blackfathom screenshot was taken during
loading in the capture. The three above-surface screenshots were taken afterward,
with the identical source/disc and QMP-paused guest; their later frame identities
are recorded separately and excluded from timing acceptance. No controller input
was injected or physically tested. These cameras are synthetic probes, not verified
walkable player routes, swimming or server gameplay.

Visual limitation found: some letters in the lower diagnostic environment label
are missing in the Blackfathom and Blackrock images, while the Dire Maul label is
complete. CPU UI failures remain zero. Cause is not established; investigate UI
submission/state/cache boundaries next. Full UI restoration/visual acceptance is
therefore still open, and this candidate does not replace the stable hardware kit.

Host checks: 26,359 liquid mesh/clock/layout/publication/cancellation/failure;
223 texture codec; 464 environment; 453 runtime; 5,144 bounded streaming;
123 repacking; two material telemetry tests. Initial repacking-test failures were
incorrect expectations of unused DXT endpoints; the final check independently
reconstructs the selected pixel. Failed logs remain preserved. Native builds pass
with the pre-existing indentation/aggregate warnings and linker merge warning.

Only owned xemu PID 66088 was quit, after exact executable/disc/QMP ownership checks.
No emulator remains. Its post-QMP GLib shutdown assertion is retained. The frozen
331 sources / 16 fixture outputs and 331 sources / 132 normal outputs match after
capture. The source ZIP and private dependency identities are preserved.
Normal XBE: 1,994,752 bytes, SHA256 `6868076776beb8fc59e334a4bb0b651cf8bc5548492d3168a2e53dbc20fd0ea0`.
Normal XISO: 499,515,392 bytes, SHA256 `32c26ce2e3472d0c645450666de00f37061d471b59f227497daa1389abaec8bc`.

Next: resolve the observed diagnostic glyph loss, extend the shared converter to
terrain MCLQ and accept native terrain routes, improve exact water/room/material
behavior and streaming/cold stalls. Continue original UI and all-class gameplay;
shared-account live acceptance remains reserved for hardware. No saved characters,
credentials, server state or stable hardware files were changed. Physical hardware,
controllers, all-content gameplay and release gates remain unmet. Autonomous work
continues with no old morning cutoff. See LIQUIDS.md for format and limitations.
