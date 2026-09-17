# Overnight handoff â€” September 15â€“16, 2026

> Current workflow: the user requests larger implementation batches and fewer
> repeated tests. Follow the testing cadence in docs/PARITY.md and the latest
> handoff below; older per-change capture/checkpoint sequences are superseded.

The user authorized autonomous implementation, builds and testing overnight.
Continuation is scheduled in this task every 10 minutes. The former morning
cutoff was superseded by the user's repeated autonomous-continuation requests. Keep working toward the full 64 MiB Xbox port; ask only for missing
access/materials or a major scope-changing decision.

## Current work

The latest accepted feature batch and normal-build checkpoint are recorded at the end of
this file. It supersedes all older emulator PIDs and saved-character summaries.

The test Human now renders natively with a modular body, runtime clothing atlas,
independent sword/shield, shared animation poses and collision-aware camera.
Live unequip/re-equip/reconnect draws and a camera sweep pass. All sixteen default
race/sex preparations pass host loading and are staged with five supported
items; all 16 default race/sex profiles now have native preview evidence; Human profiles also have in-world switch evidence. Extend profile/catalog selection, gameplay UI and
continuous play testing; do not claim full appearance or full-game coverage.
The nearby active NPC catalog now has 92 displays and no missing instances at
the saved interior. Seasonal and scripted model gaps remain explicitly tracked.

`docs/STATUS.md` describes the verified native quest, combat, inventory,
hearthstone, death/resurrection and merchant tests. `docs/PLAYTEST.md` has
controls and build/test instructions. Update both when native behavior changes.

## Execution details

- Workspace: the repository root; preserve all existing changes and game assets.
- Host build: `scripts/build-tools.ps1`; native launch/build:
  `scripts/run-xemu.ps1`. Inspect native process/QMP before each launch.
- Stop only this project's xemu via `python tools/qmp.py quit`, wait for its PID
  from `build/xemu.pid`, then rebuild its mounted ISO. Port 4444 is its QMP.
- Local vMaNGOS/MariaDB use WSL Ubuntu. Credentials stay in the ignored server
  directory; never print them. Preserve the disposable character's earned state.
- xemu uses 64 MiB, native scale and disposable disk writes. Record telemetry
  with `tools/record_telemetry.py`, screenshots with the Computer Use skill,
  and build hashes. Native replay is not physical controller verification.
- Require at least 8 MiB measured free-memory headroom. Treat emulator timings
  separately from hardware performance. Physical Xbox validation remains later.
- The latest tested full-scene normal build is `native-projected-z` in
  `build/evidence`, replay off, with 34,568 KiB minimum free memory. Check the
  final checkpoint below for its successor and recheck processes before resuming.
- No sub-agents, purchases, publishing or power-setting changes are authorized
  by this handoff. Continue within the user's already authorized port scope.

At each checkpoint record exact changes, checks, failures and next steps here.
Preserve each accepted normal development build before beginning the next batch.

## 22:30 checkpoint

- Implemented `tools/player_appearance.hpp` and `tools/character_atlas.hpp`:
  original body/face/hair/clothing composition, shared geosets, held weapon/shield.
  `assetc --player Data output.wxp profile.txt`; `config/player.txt` matches the
  disposable Human's appearance and current starting gear/shield. This is a
  host preparation path, not an integrated in-world avatar.
- `tests/player_assets.py --data 'game\Data'`: 18 real-asset
  profiles validated through the runtime loader and eight malformed inputs
  rejected. Results/logs/packs: `build/player-validation`. Default Tauren rows
  name absent optional overlays; omissions are explicit in results. NPC pack
  refactor rebuilds byte-for-byte unchanged.
- Character enum + world view retain the five appearance bytes and twenty
  display/type pairs. 346 parser checks, 10 atlas checks, 60 encrypted world
  scenarios pass. In-world appearance/equipment updates are still needed.
- New `scripts/start-server.ps1` invokes an idempotent WSL launcher; it started
  stopped realmd/mangosd and reused the same PIDs on a second invocation. Run
  this before native testing. Existing credentials/configuration/state preserved.
- Cold first-map load took about 33 seconds and exceeded the old handshake
  timeout. World entry now has its own bounded 60-second budget; an independent
  16-second delayed-login fixture passes. Normal HUD exposes world status too.
- Native failed attempts are retained: `native-player-foundation` was offline
  because servers were stopped; `native-appearance-state` authenticated but
  timed out loading the first map. `native-appearance-state-fixed` successfully
  entered the world: 1,651 samples, minimum 36,848 KiB free, 33/34/48 ms p50/p95/max,
  zero asset/region failures. Neutral capture, not physical controller proof.
- The user's saved position/progress changed since prior testing. Preserve it:
  (-9378.81,-77.882,69.1786), yaw 1.61278; level 2, XP 768, 10 copper, health 79,
  shield 2362, 11 inventory entries. Do not reset to the old 658 XP/merchant state.
- This interior exposed floor artifacts and 11 nearby NPC instances with missing
  prepared displays. A 24-bit depth-range experiment did not remove the floor
  artifact; a solid-color shader diagnostic then produced an empty blue scene
  and was inconclusive. **Both experimental shader changes were reverted.**
  Original textured shading and its 65535 depth constant are restored.
  `build/probe-floor.py` raycasts the saved interior against its actual WXP;
  nearest floor texture is intact brown wood, entry 17769 / placement 54705 /
  batch 23 in M0003249.WXP. Do not claim the floor problem is fixed.
- Found and fixed another Windows/MSYS dependency issue: shader-only changes
  regenerated `.inl` files without recompiling `main.obj`. Makefile now explicitly
  makes the owning object depend on both generated shaders. The first solid
  diagnostic build did not include its changed shader; only the rebuilt diagnostic
  is meaningful. `native-floor-solid-inconclusive.png` records that failure.
  `overnight-normal-launch.log` confirms shader generation, recompilation and
  packaging; the final normal capture verifies restored textured rendering.

## 22:43 normal-build checkpoint

- xemu PID 64160 is running the normal build with replay disabled. The last
  verified server PIDs are realmd 851 and mangosd 863 in WSL; recheck before use.
- `build/evidence/native-overnight-normal.csv` and `.json`: 1,501 samples over
  50 seconds, minimum 36,848 KiB free, 33/34/34/34 ms p50/p95/p99/max; world active,
  no asset/region failures. This is an idle interior capture, not travel, combat,
  a stability acceptance run or physical controller verification. The 11 missing
  nearby NPC instances and visible floor artifacts remain unresolved.
- Saved actual screenshot `native-overnight-normal.png` and associated
  `native-overnight-normal-receipt.json`. XBE and XISO SHA-256 hashes were checked
  against that receipt. User progress remains XP 768, 10 copper and health 79.
- Overnight heartbeat `wowx-overnight-development` is active every 30 minutes
  until 08:00 PDT September 16. Continue this handoff, avoid duplicate automation,
  and preserve the latest earned character state.

## 23:06 live appearance checkpoint

- `src/appearance.c` now publishes player identity, five appearance choices,
  display/native-display IDs, player flags and nineteen worn item entries from
  live create/value updates. The entity cache retains compact PLAYER_BYTES,
  PLAYER_BYTES_2 and 260+12*slot entries (84 additional bytes per entity).
- `SMSG_ITEM_QUERY_SINGLE_RESPONSE` (0x58) is parsed transactionally through its
  complete 5875 layout. Metadata shares the existing bounded 256-name cache;
  negative responses clear stale display data, evictions reset metadata, and
  queries use a capped working set and at most 20 requests/second.
- Host: 609 entity checks, 1,338 gameplay checks, 12 live-look checks and 61
  encrypted protocol scenarios pass. The encrypted appearance scenario covers
  equip/remove/replace/zero look updates; truncated packets do not change state.
- `native-live-appearance` proved the actual Xbox build resolved the real server's
  current equipment (shirt 9891, pants 9892, boots 10141, sword 1542, shield 18730).
  Actual player/native model ID is 49. This is network state, not avatar drawing.
- New `-AppearanceReplay` / WXC7 uses ordinary inventory UI controller samples to
  move the equipped offhand item to the bag, re-equip, log out and reconnect.
  It requires an offhand item and free backpack slot. It does not reset position.
- `native-appearance-replay-v10`: all eight stages and the dedicated checker pass;
  1,930 samples, minimum 34,572 KiB free, 33/34/39/162 ms p50/p95/p99/max. Position,
  level 2 / XP 768 / 10 copper / inventory 11 remained unchanged. Shield entry
  2362 and display 18730 persist. Actual passed screenshot and receipt saved.
- Telemetry now uses version 10 / WXTA and explicitly identifies the scenario.
  The earlier `native-appearance-replay` had a generic COMBAT HUD label and an
  overly broad derived terrain-crossing boolean. Those were reporting bugs, not
  combat/travel passes; both are fixed and the corrected replay proves the fix.
  The original capture and its versioned receipt remain as evidence.
- Floor isolation found the floor mesh alone clean, with the artifact returning
  as other building batches were included. Those images and build receipts are
  retained as `native-floor-isolation`, `native-floor-pair`,
  `native-floor-lower-batches`, `native-floor-first-batches`, `native-floor-first10`.
  The pair capture timed out before receiving telemetry; its screenshot is
  useful diagnostic evidence but that capture is not a passing telemetry test.
- The cause in this view was projected Z being perspective-corrected a second
  time: nxdk's back-buffer selection enables Z_PERSPECTIVE by default. World
  rendering now explicitly clears that flag, retaining texture perspective.
  Full-scene `native-projected-z.png` shows the floor gaps removed. All temporary
  placement/batch filters are removed; shader constants remain unchanged.
  Broader camera/scene coverage and hardware proof remain pending.

## 23:19 final normal checkpoint

- Running xemu PID 52860 / QMP 4444, normal controls and replay disabled.
  `native-appearance-normal` is the current build and evidence set. The only
  remaining build warning is nxdk/lld's existing `.edata` section merge notice.
- A 65-second boot/capture window produced 959 frames: minimum 34,568 KiB free,
  33/34/37/164 ms p50/p95/p99/max, zero asset or region failures, online at the
  saved position. This includes startup and loading, not a 65-second steady
  performance or stability run. Actual screenshot confirms the full interior
  floor remains corrected. Receipt hashes match the final XBE and XISO.
- Native appearance state is complete for the character's nineteen equipment
  slots. Avatar drawing is still pending; eleven nearby NPC instances still lack
  prepared models. No physical controller or physical Xbox verification occurred.
- `native-appearance-server-state.log` independently confirms level 2, XP 768,
  money 10, health 79, original saved position/yaw, and shield 2362 in slot 16.
  All temporary floor filters are gone. Continue from this tested normal build.

### Next implementation

Continue player appearance integration, controller UI and the interior rendering
investigation. The profile cooker proves asset
preparation; runtime needs bounded component streaming/composition and live
equipment state rather than baking every possible player combination. Live
appearance and resolved equipment now come from `WxWorldView`, updated by
`wx_player_appearance`; integrate those with the renderer and implement a
collision-aware third-person camera. Keep the full port goal and memory floor.
Expand missing actor coverage through a reusable preparation/catalog path.

## 23:54 modular avatar and camera checkpoint

- Added `include/wx_avatar.h`, `src/avatar.c`, `tools/avatar_cooker.hpp`,
  `tests/avatar_tests.c`. `assetc --avatar Data output.WXP profile.txt [catalog]`
  prepares a shared body/skeleton, all body geosets and separate equipment parts;
  WXAV v1 `.WXA` holds the bare body atlas and clothing overlays. No outfit
  permutation baking. Additional catalog rows are display/slot/inventory-type.
- Native runtime follows `WxWorldView`, composes clothing and swizzled mipmaps on
  equipment changes, selects geosets/helmet hide masks and streams independent
  items. Maximum 32 selected component IDs, 128 prepared item entries, 4 MiB
  scene budget, two loads/frame, plus accounted auxiliary allocations. Components
  use a single complete clip together. Unprepared looks/transforms are hidden
  and reported; unknown equipment is counted. Full visual coverage remains.
- Staged pair: `build/xbox/AVATAR.WXP` 1,853,282 bytes / `AVATAR.WXA` 106,996 bytes.
  Five items cover the saved Human: shirt, pants, boots, sword, shield. Thirteen
  selected components draw fourteen batches, including clothing and held gear.
  Combined idle avatar memory is 1,005,828 bytes (738,680 resident + 267,148 aux).
  Repreparation replaces existing outputs and produces identical hashes.
- `tests/player_assets.py --modular --data 'game\Data'`:
  all sixteen default race/sex profiles, equipped Human and alternate Human load
  successfully; eight invalid profiles preserve previous output. Native visual
  validation covers the current Human only. Host: 137 avatar checks and 285
  runtime/camera checks pass. Existing protocol code was not changed this step.
- WXTB telemetry (version 11, 142 words / 568 bytes) appends avatar match/readiness,
  draw count, missing items, rebuild revision, bytes, visible equipment mask,
  camera millimeters, clip and animated vertex hash. Older decoders remain.
  `tools/check-avatar.py` verifies drawing, animation, budget and gear transitions.
- `native-avatar-first`: actual screenshot shows the animated player in the saved
  interior. 1,263 frames, minimum 34,224 KiB, 33/34/44/260 ms p50/p95/p99/max.
- `native-avatar-gear`: WXC7 passes both state and avatar checkers; shield drawing
  disappears while body/sword remain, returns and persists after reconnect.
  1,929 frames, minimum 34,220 KiB, 33/34/51/247 ms. Inventory panel occludes
  the world while open; draw telemetry and final screenshot are distinct evidence.
  Actual passed screenshot and matching receipt are retained. Character remains
  level 2 / XP 768 / money 10 / HP 79 / inventory 11 / original saved position.
- Camera uses five rays covering center/corners of the near plane, original
  terrain/building collision, pending-collision fallback and cached stationary
  queries. Added conservative triangle-box rejection. The tested saved view is
  5.686 yards back from the focus. No swept sphere or full traversal claim.
- `-CameraReplay` / WXC8 turns four headings, looks down/up, restores the original
  heading/pitch without translation. `tools/check-camera.py` verifies that trace.
  First sweep: 1,158 frames, min 32,924 KiB, 33/47/69/263 ms; pruned sweep: 1,156
  frames, min 32,920 KiB, 33/42/68/254 ms. Both pass with zero asset failures.
  Moving-view frames alone are 33/37 ms p50/p95 in both; don't claim a measured
  speedup from triangle rejection. Hardware and sustained 30 fps remain unproven.
- `scripts/catalog-creatures.py` reads nearby vMaNGOS spawns, patch selection,
  current event eligibility and alternate-gender displays. It never writes SQL.
  The first all-event proposal found unsupported Winter Reveler display 15713
  (Humanoid replaceable body with no extra appearance). That failure is preserved
  in the initial catalog; inactive seasonal models remain a coverage gap.
  Active catalog adds 25 displays: 92 total, 778 batches, 109 mip chains,
  32,144,728 bytes. All batches validate; the camera sweep has zero missing nearby
  NPCs, eight submitted from the restored view. `config/creatures.txt` is updated.
- `native-avatar-server-state.log` independently records preserved saved progress.
  No physical gamepad or original Xbox validation occurred. No commits made.

### Continue from this checkpoint

Verify the final normal-run entry below before restarting xemu. Priorities:
controller character selection/creation and login UI, loading prepared avatar
profiles/catalogs by selected character, native walk/run/attack/death animation
checks, then a continuous fresh quest/death/reconnect walkthrough. Keep the full
Vanilla goal; the current avatar/profile and limited terrain are a development
slice. Missing terrain is prepared through generic map/tile tools, not zone code.
Full NPC handheld equipment, action/UI coverage, audio, water/effects, professions,
travel, social/pet/instance/PvP and hour-long tests remain. Keep 8 MiB headroom.

## 23:56 final normal checkpoint

- xemu PID **15784**, QMP **4444**, normal controls, no replay. Local server was
  reused (realmd 851 / mangosd 863 in WSL); check liveness before resuming.
- `native-avatar-normal`: 65-second boot/capture window, 919 sampled frames,
  minimum **32,924 KiB** free; p50/p95/p99/max **33/40/63/248 ms**, zero asset and
  region errors, zero missing nearby NPCs. This includes loading and is not a
  long stability or sustained 30 fps result. Avatar checker passes; final state
  is online, matched/ready, 14 avatar draws, zero missing avatar items, 1,005,828
  avatar bytes, both held-item bits set. Final actual screenshot shows the Human
  and nearby NPCs in the interior; shader floor fix remains present.
- Final XBE **1,728,512 bytes**, XISO **507,576,320 bytes**. Receipt hashes match
  both outputs, both avatar files and actors.wxp. Evidence set is
  `build/evidence/native-avatar-normal.{csv,json,png}` with `.avatar-check.json`
  and `-receipt.json`. No new build warnings; only the existing lld section merge.
- Database evidence confirms XP 768, ten copper, eleven inventory records and
  shield 2362 still equipped. Camera replay restored yaw within 0.000003 radians
  and did not translate the character. Default normal pitch is restored.
- Overnight heartbeat was verified ACTIVE through 08:00 PDT. Source diff passes
  whitespace checking. New source files are intent-to-add; repository still has
  no commits. Keep the current assets and earned state intact.

## September 16, 00:30 character workflow

- Added a bounded character-session mailbox and controller roster, creation form,
  name keyboard and confirmation. Start -> Y logs out and opens the roster;
  X creates, A enters. Valid Vanilla race/class combinations and both sexes are
  available; appearance defaults to zero. Native preview/customization/profile
  catalogs and account login remain. The server handles availability/restrictions.
- 358 pure packet/UI checks, 346 enumeration checks, ten encrypted character
  scenarios and all 61 existing world scenarios pass. Logs: character-ui-host-build,
  character-protocol and character-world-regression in build. Native launch log:
  character-native-launch. No source-control commits or asset/credential staging.
- WXC9 / -CharacterReplay created `Xboxnight` GUID 2 through normal controller
  inputs and the name keyboard, entered Northshire, then returned to `Xboxer`
  GUID 1. `native-character-create` has 2,660 samples, min 32,880 KiB free,
  p50/p95/p99/max 33/44/71/475 ms, zero asset/region/session failures; the character
  checker passes. Its receipt and actual passed-screen PNG are retained.
- Database before/after evidence in native-character-before/after.log confirms
  original level 2, XP 768, ten copper, HP 79, eleven inventory records, shield
  2362 and position (-9378.81,-77.882,69.1786), yaw 1.61278. New Human male Warrior
  `Xboxnight` is level 1, XP/money 0, HP 60 at (-8949.95,-132.493,83.5309), yaw 0.
  Preserve both characters. Repeat WXC9 reuses Xboxnight; it expects that character
  still to have zero XP/money, so don't reset earned gameplay to rerun the fixture.
- WXTC adds lobby/UI/create-result/player identity. WXTD additionally reports
  effective controller connection. Replays can inject that connection flag.
  Normal input reports an attached adapter, but brief sky.press_key keyboard
  automation did not register game buttons. Physical controls remain unverified;
  do not present this as a proven controller/USB test or a proven client defect.
- `native-character-input` contains only the first normal boot (4,634 samples),
  split at the first decreasing guest timestamp. The original diagnostic capture
  and summary are preserved as -mixed files; it accidentally included the next
  replay boot. Recorder now ends automatically before a guest clock restart.
- Added -CharacterScreensReplay: paced roster/form/keyboard inspection, cancel
  empty draft, re-enter original. It creates/deletes nothing. Actual roster image
  native-character-roster.png captured; remaining screenshots/final normal run
  are recorded in the following checkpoint. Timing is xemu-only, not hardware.

## 00:37 final normal checkpoint

- xemu PID **31356**, QMP **4444**, normal controls, replay disabled. Local server
  loopback ports 3725 and 8086 are listening. Preserve both characters and inspect
  liveness before another launch; do not start concurrent emulator/server copies.
- Current XBE **1,736,704 bytes**, XISO **507,576,320 bytes**. Receipt matches both
  outputs, avatar pair, actors, input/scenario files and all recorded source
  hashes. Normal evidence: native-character-normal csv/json/png, avatar-check,
  normal-check, receipt and receipt-check. Normal capture has 2,099 samples over
  70 seconds; min **32,908 KiB**, p50/p95/p99/max **33/34/34/51 ms**. It is a neutral
  session, not crowded/combat performance or hour-long stability acceptance.
- Paced screen capture passes its screens-check: 4,923 samples, min 32,720 KiB,
  p50/p95/p99/max 33/36/38/192 ms. Roster/form/keyboard states were observed in
  telemetry; actual screenshots are native-character-roster.png and
  native-character-keyboard.png. Empty draft was cancelled; original restored.
- WXTB/WXTC/WXTD and reboot isolation passed an independent three-packet UDP
  recorder fixture. Its restarted fourth packet was excluded. Evidence is
  telemetry-reboot-fixture.* and build/telemetry-reboot-fixture.log. Current
  normal telemetry reports attached adapter, complete avatar and no load errors.
- native-character-final-server.log verifies both saved characters and original
  inventory after the screen test. The automated keyboard attempts remain
  unverified; do not equate connected=1 with working physical buttons. Possible
  short-press sampling is a hypothesis, not an established cause. The native
  adapter polls SDL at the render rate; nxdk's USB callback retains only the
  latest report. Investigate input transport/tap retention before claiming it.
- Next useful work: investigate interactive adapter input, then build a continuous
  quest/combat/loot/death/reconnect route using fresh Xboxnight without teleporting
  or resetting either character. Extend native avatar profile/catalog selection
  and customization afterward/in parallel with local work where appropriate.
  No subagents are authorized. Full Vanilla and physical-hardware gates remain.
- Overnight automation remains ACTIVE through **08:00 PDT**. Source whitespace
  check passes. No commits made; maintain all existing changes and private assets.


## 01:18 starter quest, camp route and journal work

- `native-starter-quest` completed a continuous fresh-character route: selected
  Xboxnight, accepted quest 783 from Deputy Willem, walked through the Abbey
  entrance, turned in to Marshal McBride, accepted quest 7, logged out/reconnected.
  The starter checker passes: 3,042 samples, 60.175 yards walked, min 26,612 KiB,
  p50/p95/p99/max 37/61/81/394 ms. No asset/region/session failures. Capture begins
  at stage 2; initial menu-opening stage 1 precedes recording. Receipt and actual
  passed-screen PNG retained. This covers the starter delivery quest, not combat.
- Added host walkpath A* and `regioncheck --route` using native collision/streaming.
  Initial camp searches lacked settled floor data, then a smoothed route hugged
  a wall. Retained failed searches/checks. Planner now retries pending/missing
  floor after loading and checks lateral clearance when smoothing. Regioncheck
  uses anchored interpolation to avoid float accumulation drift. The 18-leg
  Abbey/camp roundtrip passes actual-asset host collision, zero load errors.
- `native-camp-quest` walked Abbey -> camp, earned eight kills, three loot items,
  eleven accepted spell casts and level two, then failed approaching a fenced
  target. It is retained as a failed test, with PNG/receipt. Native replay now
  blacklists blocked targets after bounded strafe attempts. Physics unchanged.
- Disconnect saved earned progress: Xboxnight GUID2 level2 XP44 money22 HP79,
  position (-8780.71,-113.244,82.8205), yaw1.44947; quest7 active, 8/10 kills;
  quest783 rewarded. `native-camp-quest-saved.log` verifies this. Xboxer unchanged.
  Earlier -after.log read while still online and used an invalid mob_count column;
  use the -saved.log, not that unsuccessful read, for persistence evidence.
- Added bounded quest journal query parser/cache and controller list/objectives/
  description/pages. Open Start -> A; B returns through list to world. Live six-bit
  quest counters, completion/failure flags, inventory item counts are retained.
  Cache is 83,604 bytes. 372 host journal checks and all ten CTest targets pass;
  encrypted quest valid/truncated/trailing/NaN plus existing world regressions:
  65 scenarios pass. Test caught and fixed selection retention across characters.
- WXTE adds six journal state fields; retains all earlier telemetry layouts.
  Native build/test is in progress, not yet accepted. First native build exposed
  missing telemetry header fields (failed patch); corrected before retry. Logs
  camp-quest-journal-launch and launch2 retain both results. Recorder session
  native-camp-quest-journal starts before successful boot and stops at terminal.
- Fixed character UI status message at row16 (SDK has only rows0-15), now row15.
  Earlier dense-HUD text fragments are not reproduced in the latest fresh capture;
  no speculative text-renderer/SDK patch applied. Observe further screenshots.
- xemu host Ctrl+P pause/resume works via sky; short keyboard-as-pad taps remain
  unverified. SDL keyboard-state sampling timing is a hypothesis, not proof of
  a client transport defect. Physical pad and Xbox checks remain pending.


## 01:26 quest continuation

- `native-camp-quest-journal` verified journal objectives/description with quest7
  at 8/10, then two real kills, a loot item and two accepted casts. All objectives
  are complete, but the direct walk home hit a tree and failed at stage27.
  Actual description and failed-route PNGs are retained. 4,830 samples, min
  33,104 KiB, p50/p95/p99/max 33/42/65/428 ms; zero load/region/packet failures.
  Its strict camp checker intentionally reports failure. The journal title had
  trailing header characters; the native text buffer overwrite is now corrected.
- After disconnect, Xboxnight saved level2 XP154 money29 HP79 at
  (-8790.68,-150.918,83.1206), quest7 at10/10, quest783 rewarded. Read the actual
  `native-camp-quest-journal-saved.log` for authoritative values. Xboxer remains unchanged.
- Host A* found a five-point path around that tree; independent native collision
  traversal passes. Added a bounded development-only CAMP.RTE/WXRP route file,
  max128 finite XYZ waypoints, enabled only for CampQuestReplay and only within
  one yard of the route's start. This controls ordinary input; no relocation.
  Launcher accepts `-CampReturn build/journey-return-complete.txt` and resets
  the route to NONE on normal runs. XISO dependencies and receipts include it.
- `native-camp-return` is running the return/turn-in/reconnect continuation.
  Use `tools/check-camp-quest.py <csv> --return-only`: it explicitly excludes
  combat coverage in that run and requires the journal to start at10/10.
  Do not combine these interrupted runs into a claimed continuous quest pass.
- All ten host test targets and 65 encrypted world scenarios pass. The latest
  successful camp-journal native build used 640-byte WXTE records; the current
  return build also fixes the title overwrite and initializer warning. Next:
  actual journal screenshots, turn-in and saved-state evidence, then normal
  rebuild/receipt verification. Continuous full quest/death remains pending.


## 01:34 normal checkpoint â€” resume here

- Normal xemu PID **43556**, QMP **4444**; replay and CAMP.RTE both **NONE**.
  Server loopback ports3725/8086 listen. Build log journal-normal-launch. Original
  Xboxer is online at the inn; preserve both saved characters and all progress.
- XBE **1,761,280 bytes**, XISO **507,576,320 bytes**. The normal receipt matches
  all120 recorded source hashes and14 outputs, including input/scenario/route,
  avatar, NPC and terrain packs. `native-journal-normal-receipt-check.json` passes.
  No source edits occurred after this receipt; documentation edits are separate.
- `native-journal-normal`: 1,782 samples within a95-second recorder window that
  includes boot/loading; min **32,788 KiB**, p50/p95/p99/max **33/34/75/299 ms**.
  Normal/appearance checks pass, original XP768/money10/11 inventory records,
  zero asset/region/packet failures, attached adapter. Actual normal PNG retained.
  This is not stress, crowded-scene or physical hardware/controller acceptance.
- `native-camp-return` passed the explicitly return-only checker and avatar check:
  5,121 samples, min **26,900 KiB**, p50/p95/p99/max **33/60/103/602 ms**,213.90yards
  walked. Both journal views, normal walking to McBride,170XP/25copper reward,
  logout/reconnect with saved state. The first checker used the immediate reward
  event frame, before the separate player/inventory updates; comparison now uses
  the settled last stage31 frame before logout. No evidence was discarded.
- Actual `native-quest-journal-objectives.png` shows10/10 and the corrected title.
  `native-camp-return-passed.png` shows the final reconnect. Earlier first journal
  description PNG keeps the trailing-header bug; corrected title verified later.
- Authoritative saved character state: Xboxnight GUID2 level2,XP324,money54,HP79,
  eight inventory records, (-8905.02,-159.955,81.9389),yaw5.11945. Quests783 and7
  are rewarded (quest7 mob_count1=10). `native-camp-return-saved.log` verifies it.
  Xboxer GUID1 remains level2,XP768,money10,HP79,11records, original inn position.
- Do not rerun fresh/active-quest fixtures expecting Xboxnight to have zero XP or
  an active quest7. No reset/teleport was used in these tests. The ten kills and
  return occurred across interrupted captures; a full uninterrupted quest/death
  loop still needs a genuinely fresh character workflow and robust test routing.
- Added route/generic-check sources plus journal: all ten CTest targets passed,
 372 journal cases and65 encrypted world scenarios. Native only warnings are
  the existing upstream unused SRP variable and lld section merge. Whitespace
  checks pass. Git still has no commits; preserve all existing source changes.
- Next useful implementation: asset size/renderer optimization for broad-world
  coverage, while retaining the current RGBA baseline and tested normal build.
  `build/pack-size-audit.json` measures four staged map packs at422,359,042bytes.
  Northshire/Goldshire packs are167.5/166.0MB, with19,164/20,496 entries. Each has
  roughly80MB of vertex/collision data and hundreds of unique RGBA mip chains;
  textures are shared inside each pack, but placements/textures repeat across
  packs. Avoid extrapolating a stock-storage feasibility claim from four tiles.
  Native NV2A supports DXT1 (nv_regs.h constant0x0C); current main.c texture format
  is ARGB8888 and pack.c assumes four bytes/pixel. Investigate GPU-compressed
  terrain/static textures and cross-pack geometry/texture sharing with strict
  boundaries, rollback, measured sizes and actual xemu screenshots. No compressed
  texture implementation exists yet. Avatar's runtime clothing atlas must retain
  its known working pixel format unless explicitly updated and tested.
- Full-game UI/race/class/travel/instances/audio/water/effects and30fps remain
  incomplete. Physical pad and original hardware remain release gates. Overnight
  automation is verified ACTIVE through08:00PDT; no new automation was created.

## 01:55 checkpoint â€” compressed world textures

- Current xemu **PID31776**, QMP4444, normal replay-disabled build. Native SDL
  adapter is attached; no physical controller verification. Local server remains
  running. No pending recorder/tool sessions. Stop only this xemu through QMP
  and wait for its PID before rebuilding its mounted ISO.
- Implemented WXP v5 DXT1/DXT5 mip payloads and NV2A sampling, with v4 RGBA
  compatibility. Strict sizes/flags/kinds/mip tails, allocation rollback and
  texture-cache encoding identity are checked. Avatar/NPC packs stay unchanged.
  This supersedes the earlier note that compression was not implemented.
- `wowx_packopt` uses vendored, pinned stb_dxt v1.12, MIT alternative. Header,
  notices and hash verified. Bootstrap checks the hash; receipts include the
  vendored source. Repacking rejects character/atlas packs and existing outputs;
  it preserves non-square textures. Partial temporary outputs are removed.
- `scripts/compress-world.py` independently compares every vertex/index byte,
  metadata and placement ID, runs runtime verification, writes new tile sizes
  and hash receipts. Reuse requires matching source/optimizer/verifier/output.
  `scripts/stage-world.py --source <directory>` stages a verified batch.
- Original assets remain in **build/rgba-baseline** (five WXP + WXI). First
  compressed run:build/dxt-packs; final recipe:**build/dxt-world-final**. Both
  runs produced identical pack hashes. Final packs/index are staged in build/xbox.
- Five packs:**471131438 ->253173554 bytes**,46.3% smaller. Texture bytes:
  **253838920 ->35881036**. Four indexed tiles:422359042 ->238570978. Repeated
  world-space vertex/collision geometry is still large; no full-stock-disk claim.
- All11 CTest targets pass: runtime340, compressed-texture187, plus22 optimizer
  integration checks. Independent WoWee color/reference alpha decoders check
  block layout, color and mip tails. Fixtures:build/packopt-rg2spmwm. Initial
  C++ linkage build failure was fixed with a C linkage wrapper; all build logs
  remain. No protocol edits; prior65 encrypted cases remain the latest run.
- `native-dxt-normal`:1346samples,min38072KiB,33/34/42/298ms p50/p95/p99/max.
  Normal/avatar checks pass;5284KiB more headroom than journal normal, with
  different capture windows. No asset/region/packet errors. Actual PNG retained.
- `native-dxt-camera`:1156samples,min38072KiB,33/40/50/309ms including startup.
  Moving-view667frames:33/37/65ms p50/p95/max. Full checker/avatar pass:
  heading/pitch sweep, collision, no translation, restored view. Actual turning
  and passed screenshots retained. Input was injected by the native test fixture.
- Latest **native-dxt-final-normal**:1550samples within85s including boot,
  min**38068KiB**,p50/p95/p99/max**33/34/42/302ms**. Normal/avatar checks pass,
  no asset/region/packet faults or replay. PNG/CSV/JSON/receipt retained. New
  `tools/check-build-receipt.py` verifies all**127sources and14outputs** match.
- XBE**1761280B**,XISO**289669120B**. No source edits after final receipt; docs
  only. Existing compiler/linker warnings unchanged. Preserve intent-to-add
  worktree; no commits/resets/reverts. An old CP1252 dash in this handoff was
  normalized to UTF-8 so patching works again; content preserved.
- Saved characters unchanged: XboxerGUID1 atinn level2 XP768,money10,HP79,
 11inventory; XboxnightGUID2 atAbbey level2 XP324,money54,HP79,8inventory,
  quests783/7rewarded. Do not reset or reuse fresh-character assertions.
- Next high-value playable work: controller spellbook and action assignment.
  WxGame already tracks512 known spell IDs and120 server action slots. Use
  Vanilla Spell.dbc for bounded name/rank/passive metadata and pinned protocol
  sources for persistent action updates; current Start menu has no spellbook.
  Validate parsing/input/protocol before native replay, then restore existing
  bindings/progress. A fresh-name continuous quest/combat/death fixture is also
  needed. Test routes may name Northshire; world engine loading stays generic.
- Full-game content/UI/audio/water/effects/performance and hardware gates remain.
  Existing overnight automation ACTIVE through08:00PDT; not duplicated.

## 02:40 checkpoint â€” controller spellbook

- Current xemu **PID43408**, QMP4444, normal replay-disabled build. Local server
  remains running. No pending recorder/tool sessions. Stop only this owned xemu
  through QMP and wait for its PID before rebuilding its mounted ISO. Physical
  controller input remains unverified; the SDL adapter is attached.
- Start -> White opens the learned spellbook. D-pad browses/pages, A chooses an
  active ability, trigger/button combinations or left/right choose its control.
  A reviews then confirms; X reviews clearing a control; B cancels. Gameplay
  input is suppressed while browsing. Stale confirmations are invalidated when
  spell, stance, controller mapping or current action changes.
- Prepared Spell.dbc from the supplied 13 archives: 22,357 records in
  **SPELLS.WXS**, 2,459,286 bytes. Staged in build/xbox. The resident ID index is
  44,714 bytes; fixed learned metadata and display indices total 58,368 bytes,
  plus small state fields. At most one record loads per frame. All actual
  catalog records pass the runtime loader. No game data is in source control.
- Actual screenshots exposed internal abilities in the first list. Added
  Vanilla 0x80 hidden and 0x20 trade-child filtering using pinned vMaNGOS flags;
  active abilities sort before passives. The final warrior has 40 server spell
  IDs and 10 visible top-level entries. Profession sublists remain pending.
- CMSG_SET_ACTION_BUTTON 0x128 sends the Vanilla five-byte spell/clear payload.
  Local action state updates after the socket write; Vanilla has no positive
  acknowledgement. Reconnect validates the authoritative 120-slot server bar.
  Existing 24-control mapping and warrior stance offsets are retained.
- All **12 CTest suites**, **1,016 spellbook checks**, and **68 encrypted world
  protocol scenarios** pass. Tests cover malformed metadata, allocation limits,
  display filters/sorting, confirmation changes, action bytes and transactional
  truncated action lists. Existing compiler/linker warnings remain unchanged.
- New -SpellbookReplay (scenario12/WXCC) injects ordinary controller samples:
  assign Heroic Strike78 to an empty control, logout/reconnect, verify78, clear,
  logout/reconnect, verify original empty. No direct packet/state shortcuts.
  Three full captures passed; original character progress/position retained.
  The first host checker used the first online sample before player/inventory
  initialization; corrected baseline uses initialized stage2. Initial failed
  report and raw evidence are preserved. This was a checker baseline error.
- Latest **native-spellbook-final**:4,025 samples, minimum **37,820 KiB** free,
  **33/35/45/313 ms** p50/p95/p99/max. Spellbook and avatar checkers pass. Actual
  final-list and final-confirm PNGs show the corrected ten-entry list and prompts.
  Receipt verified all **134 source files and15 outputs** at capture time.
- Latest normal **native-spellbook-normal**:1,536 samples, minimum **38,012 KiB**
  free, **33/34/37/245 ms** p50/p95/p99/max, including startup. Normal/avatar and
  receipt checks pass, no asset/region/book/packet failures, no replay. Actual
  normal PNG shows the equipped player in the inn. These are xemu guest-clock
  measurements, not physical hardware or hour-long stress acceptance.
- Native **default.xbe 1,765,376 bytes**, **XISO 292,093,952 bytes**. Latest normal
  receipt and checks are under build/evidence/native-spellbook-normal.*. Source
  hashes match; only documentation changes followed. Git remains without commits;
  preserve all existing changes, including intent-to-add files.
- Saved characters unchanged: XboxerGUID1, level2 XP768/money10/HP79/11inventory
  at the inn. XboxnightGUID2, level2 XP324/money54/HP79/8inventory at Abbey;
  quests783/7 rewarded. Test slot23 was restored empty after two reconnects.
- Next useful implementation: automatic avatar profile selection for changing
  characters. Runtime currently opens only D:\\AVATAR.WXP/WXA and matches its
  seven look bytes; mismatch suppresses drawing. Host preparation already
  supports other profiles. Read src/avatar.c and tools/avatar_cooker.hpp first.
  Keep one bounded resident avatar, validate exact profile identity and failure
  rollback, and avoid repeated disk-open attempts every frame. Broaden prepared
  default appearances and native character-switch tests without claiming all
  race/class zones or cosmetics are complete. A continuous fresh-character
  quest/combat/death fixture and hour-long mixed workload also remain needed.
- Spell descriptions/icons/cooldowns/talents/profession lists, all form bars,
  full-game content/audio/water/effects/performance and hardware gates remain.
  Existing overnight automation continues through08:00PDT; do not duplicate it.

## 02:55 checkpoint â€” automatic avatar selection

- Current owned xemu **PID48576**, QMP4444, normal replay-disabled build. Server
  remains running. Stop through QMP and wait for its PID before modifying its
  mounted ISO. The normal capture and final measurements are recorded below.
- Added `WxAvatarSelection` in avatar.c/h. All seven server look bytes select
  A +14 hex digits +.WXP/WXA; one profile stays resident. Prior GPU work drains
  before old allocations are released. Exact metadata identity is validated.
  A failed profile is attempted once per look/connection; switching or reconnect
  can retry. A missing/wrong profile cannot display the previous character.
- `scripts/prepare-avatars.py` prepares sixteen default race/sex profiles with
  the existing five-item test catalog, or a supplied exact profile/additional
  item catalog. Existing output directories are rejected. Prepared files live
  in **build/avatar-profiles**, with per-file hashes/logs; all32 staged files
  match. Total **32,643,410 bytes**. These are not complete cosmetic/equipment
  catalogs; only Human male/female have native character coverage so far.
- All12 host suites pass; **383 avatar checks** cover transitions, exact paths,
  wrong identity, missing profiles, retry bounds, allocation failure and cleanup.
  All16 actual prepared pairs pass the runtime verifier. No protocol changes;
  the previous68 encrypted-world scenarios remain the latest protocol run.
- New `-AvatarSwitchReplay`, scenario13/WXCD, uses the ordinary character UI to
  create/select **Xboxdawn**, Human female warrior, then return to the original.
  First run actually created her through name keyboard/gender/confirmation.
  Second run selected her existing record. Both verify male/female/male drawing,
  exactly3 profile loads, no selection errors, and preserved original progress
  and position. No direct packet, teleport, SQL mutation or earned-state reset.
- First **native-avatar-switch**:3,470 samples, min**37,976 KiB**,
  **33/37/48/629 ms** p50/p95/p99/max. Character and avatar checkers pass. Source
  receipt matched135 sources/47 outputs at capture. Actual creation/female PNGs
  retained. A stale startup variable caused the first native compile to fail;
  it was fixed before this run. Existing upstream/linker warnings remain.
- Final replay **native-avatar-switch-final**:2,844 samples,min**34,600 KiB**,
  **34/40/66/590 ms**. Both checkers pass. The lower headroom coincides with more
  submitted NPCs (12 versus8) and a larger scene/actor cache, not an established
  leak diagnosis. Peak stalls occur during world entry; 30fps transitions remain
  unproven. Final-female PNG shows the complete body without diagnostic text.
  Receipt matched135 sources/47 outputs before the following packaging cleanup.
- Removed the obsolete mandatory AVATAR pair from Makefile/receipt fixed lists;
  profile globs include all staged pairs (including legacy files if present).
  This keeps fresh profile-only staging buildable. Native normal rebuild includes
  this cleanup. Legacy files remain preserved; runtime no longer opens them.
- WXTG telemetry appends three profile counters (173 words/692 bytes); older
  formats still decode. The native test injects controller samples, so physical
  USB/controller and Xbox validation remain separate.
- Saved **Xboxdawn GUID3**:race1,class1,gender1,default appearance,level1,XP0,
  money0,HP60,map0,(-8949.95,-132.493,83.5309),yaw0. No gameplay earned progress
  yet. XboxerGUID1 and XboxnightGUID2 remain as before. Authoritative rows are
  in `native-avatar-switch-saved.log`. Do not reuse the old fresh Xboxnight
  assertions or reset his rewarded quests783/7. Xboxdawn is available for a new
  continuous quest test, but this switch fixture expects her to stay level1/XP0.
- Next useful controller work: show the eight current bindings when LT, RT or
  both are held, using the existing spellbook metadata, item names, saved24-to120
  mapping and warrior stance offsets. The HUD still hardcodes `LT+A: attack`,
  which becomes wrong after remapping. Keep target/health readable, suppress the
  panel inside menus and validate all3 layers/remaps and live assignment changes.
  This action panel is **not implemented yet**. The existing pbkit text grid has
  16 rows/60 columns; plan layout from that bound. Full utility radial, inventory
  detail, spell targeting/audio and broader mechanics also remain.
- Full Vanilla coverage, optimized world transitions, one-hour mixed sessions,
  physical controls and stock Xbox validation remain release gates. Existing
  overnight automation stays ACTIVE until08:00PDT; do not duplicate it.
- Final normal **native-avatar-select-normal**:1,576 samples, min**38,004 KiB**,
  **33/34/36/352 ms** p50/p95/p99/max, including startup. Normal/avatar checks
  pass, one profile load, no asset/region/book/profile/packet errors or replay.
  Source/output receipt verifies **135 sources/47 outputs**. XBE**1,769,472B**,
  XISO**324,796,416B**. PNG/CSV/JSON/receipt retained. No recorder/tool session
  remains active. Only documentation edits followed; whitespace checks pass.

## 03:25 checkpoint â€” live controller action prompts

- Added an eight-cell action panel while holding LT, RT or both. It uses the
  saved24-to120 controller mapping and the same supported warrior-stance resolver
  as gameplay. Names come from bounded spell/item metadata; no file or network
  access happens during drawing. Empty, loading, missing and unsupported actions
  have distinct labels. Cells are19 name characters; spellbook has full names.
- Health and target rows move above the panel; it is hidden inside menus,
  inventory, spellbook, journal, dialogs, death UI and diagnostics. Actual button
  holding highlights the matching cell. The old hardcoded attack hint is replaced
  by LT/RT action guidance. Cooldowns, resource indicators, icons, remaining class
  forms, full spell targeting and casting mechanics remain separate work.
- Shared `WxActionPrompt` records feed both drawing and **WXTH** telemetry:
  actionbar_layer,8 resolved server slots,8 packed actions. Total190words/760B;
  older formats still decode. All12 host suites pass; **1,079 spellbook/action
  checks**, including arbitrary24 remaps, warrior stance slots, cached item/spell
  names, unavailable metadata, invalid indices and control-byte sanitization.
- New `-ActionbarReplay` is read-only: three450-frame trigger holds, neutral gaps,
  menu/inventory suppression and normal return. No action buttons during holds.
  `check-actionbar.py` compares each displayed binding against a separate,
  read-only server snapshot. Original server slots72=6603,73=78,83=item117.
  Snapshot is `build/evidence/actionbar-server-bindings.json` (private ignored).
- **native-actionbar** passes its checker and avatar checks:3,063samples,
  min**38,004KiB**,**33/35/40/357ms** p50/p95/p99/max. Actual RT screenshot shows
  Tough Jerky in the expected control. All three layers are verified in telemetry;
  only RT has a panel screenshot. No unintended cast, movement or progress change.
  Nine zero pad samples at frame0 are its startup wait, and one at frame2690 is
  its deliberate neutral completion. Normal adapter captures are still separate
  from physical controller use.
- The first receipt-check report failed because check-spellbook.py was extended
  after that build, and the new checker did not exist in its source set. All
  native source/output identities were unchanged. Preserve that failed report;
  subsequent native builds include both checkers and their136-source receipts pass.
- Spellbook scenario12 now holds the edited layer for300frames after assigning
  and clearing, before each reconnect. `check-spellbook.py --actionbar` checks
  both visible updates and suppression while browsing. **native-actionbar-spellbook**
  passes:4,926samples,min**37,812KiB**,**33/34/36/357ms**. Slot23 changed to78,
  survived reconnect, then was cleared and read back empty on the next connection.
  Original progress/position retained. Actual clear-confirmation screen was
  inspected; panel screenshot timing missed the short holds, so don't claim those
  images exist. Both update windows have300 native telemetry samples.
- A final small label fix distinguishes permanently missing spell metadata from
  loading. Its new host case passes. The preliminary normal capture is retained
  as native-actionbar-normal; the final normal capture below includes that fix.
- Normal launch still chooses XboxerGUID1 at the inn. Saved Xboxer/Xboxnight/
  Xboxdawn states remain unchanged from02:55; no SQL reset or teleport. The added
  action panel does not change bindings or gameplay dispatch. Git has no commits;
  preserve all intent-to-add changes and private assets. Latest protocol testing
  remains68 encrypted scenarios; this checkpoint did not change protocol code.
- Next useful controller work: utility radial on Black, while carefully preserving
  access to bags and updating affected input fixtures. The approved plan still
  calls for this, cursor mode, map, talent and fuller inventory/quest/game menus.
  A hold-to-open radial with a tap for existing bags is one possible compatible
  design; decide based on code and test behavior, no routine user question needed.
  Do not add decorative entries for unimplemented game systems. Alternatively,
  pursue the uninterrupted fresh-character quest/combat/death test with Xboxdawn;
  once she earns progress, the avatar-switch fixture must stop expecting XP0.
- Overnight automation remains ACTIVE until08:00PDT; no duplicate. Full-game
  coverage, hour-long mixed workload, smooth transitions, physical controls and
  original hardware remain unproven. Preserve the final tested normal build when
  ending each checkpoint.
- Final **native-actionbar-final-normal**:owned xemu **PID42880**, normal replay
  disabled, no pending recorder/tool sessions.1,560samples,min**38,008KiB**,
  **33/34/36/343ms** p50/p95/p99/max. Normal/avatar checks pass; no asset, region,
  book, profile or packet failures, and the action panel stays hidden without
  triggers. SDL adapter connected; no physical-input claim. Actual normal PNG
  includes the corrected trigger hint. All**136sources/47outputs** match the
  receipt. XBE**1,769,472B**,XISO**324,796,416B**. Only docs followed; whitespace
  checks pass. Before the next build, stop this PID through QMP4444 and wait.


## 03:58 checkpoint â€” Black utility wheel

- Tap Black toggles bags on release; hold350ms opens a four-sector wheel:
  Up Bags, Right Spellbook, Down Quests, Left Settings. Left stick or D-pad
  selects; release or A opens; B or neutral release cancels. No placeholder
  entries. Input capture includes both sticks and trigger actions, and persists
  while Black remains held after A confirmation. Disconnect/context loss cancels.
- New allocation-free utility state and pbkit rendering,60 host checks; all13
  suites passed before the next quest fixture work. Telemetry WXTI196words/784B
  includes wheel/opening/selection/action/revision and inventory-open status.
- native-utility passes all15 assertions plus avatar/receipt checks:
  3,210samples,min38,008KiB,33/36/41/348ms p50/p95/p99/max.
  Six destinations/openings, cancellation, no gameplay movement/camera/cast leak.
  Actual native-utility-wheel.png shows selected Quests;942 visible-wheel frames
  have33/34/35ms p50/p95/max. Physical input remains separate.
- native-utility-actionbar passes all12 regression assertions and avatar/receipt:
  3,035samples,min38,008KiB,33/35/39/369ms. All trigger layers preserve saved
  bindings and menu/bag suppression. Actual LT screenshot shows Attack/HeroicStrike.
- native-utility-normal passes normal/avatar/source-output identity checks:
  1,527samples,min38,008KiB,33/34/36/347ms,141sources/47outputs.
  Actual screenshot saved, no replay/packet/asset/profile failures, one profile
  load; saved XboxerGUID1 level2 XP768 money10 inv11 unchanged. PID42404 was
  this build (stop through owned QMP before replacing). Recorder is finished.
- Work continues on a continuous fresh quest loop using XboxdawnGUID3. New
  scenario14/WXCE and bounded trail fixture are in progress; they were NOT in
  the utility normal receipt. Do not claim this new quest fixture tested until
  the new native capture passes. Preserve all earned character progress.


## 04:17 in-progress checkpoint â€” quest return and collision work

- New scenario14/WXCE combined starter delivery and camp quest on XboxdawnGUID3.
  native-quest-loop FAILED at the automatic return walk beside a camp obstacle:
  13,516samples,min32,300KiB,33/54/103/597ms p50/p95/p99/max. It did earn quest783,
  accept7/read journal, kill10, collect3loot items and accept14spells, reaching
  level2 XP143. No asset/region/profile/packet failures. Actual Abbey/kobold/failure
  screenshots retained. First source receipt145sources47outputs passed before
  later development edits; this is not a passing full loop. Never reset progress.
- Coarse0.5-yard breadcrumbs and0.055-yard arrival tolerance were insufficient
  next to a barrel. Dense trail now records every moved sample,4096points (~48KiB),
  with0.004-yard arrival and tighter steering. A host replay of all recorded camp
  motion retraces844points in9634simulated frames with0blocked frames/failures,
  ending at camp anchor. Source/log/input artifacts:build/evidence/trail-return-*.
  Native fresh-loop verification remains pending. All14host suites pass;4242trail
  bounds/relocation/overflow checks. Current native recovery predates only the
  last tight-steering change (irrelevant to its supplied-route scenario11).
- Collision optimization: conservative XY bounds before floor barycentrics and
  swept-box rejection before six body-ray tests. Zero allocation.8000 real-pack
  queries exactly match previous floor/height/walk/blocker results;6770allowed,
  1230blocked. Host CPU old1377ms/new564ms over spawn/Abbey/camp/inn, not an Xbox
  performance claim. baseline source SHA2562dd262d059210c3e4075e987e654e9ef8e49f13d5c527ceb0b62110e035c688e
  inbuild/evidence/collision-before-bounds.c; comparison source/exe/log nearby.
  Current src/collision.c is OPTIMIZED. Do not accidentally restore baseline as
  final. Native before/after walking comparison is still pending.
- Added scenario15/WXCF `-WalkReplay`, read-only Xboxnight Abbey-to-camp-and-back,
  restores view and reconnects with XP/inventory/money unchanged. Checker exists
  but this scenario has NOT run natively yet. Intended for collision comparison.
- Character/journey replays now accept -TestCharacter name via TESTCHAR.BIN;
  normal mode writesNONE. Avatar-switch fresh assertions apply only to newly
  created characters, so selecting a character with earned progress is allowed.
  Next fresh character can be Xboxrise: AvatarSwitchReplay -TestCharacter Xboxrise
  creates femaleHuman with existing stagedprofile, then QuestLoopReplay on same.
  Both checkers support the saved GUID as needed; don't assumeGUID4untilverified.
- CURRENT owned xemuPID40840 runs `-CampQuestReplay -TestCharacter Xboxdawn
  -CampReturn build/evidence/dawn-return-route.txt`. Its native-dawn-return.csv
  recorder session25369 is active, finish-on-scenario,540s maximum. New receipt
 146sources48outputs passed. Supplied returnroute was found+validated with actual
  hostcollision, from(-8757.8213,-194.5618,85.85147)via(-8758.8213,-194.0618,85.7448)
  tocampanchor. This continuation should turninquest7 and reconnect. Run
  check-camp-quest.py ... --return-only --guid3 whenfinished. Keep failureevidence.
- After recovery, complete fresh dense-trailnative test, optional walkingbefore/
  after, then leave a tested normal replay-disabled build. No active subagents.

## 04:46 checkpoint â€” fresh loop passes, profiling in progress

- `native-dawn-return` finished successfully. GUID 3 Xboxdawn retains level 2,
  313 XP, 49 copper and 10 inventory entries at the Abbey. Its separate supplied
  route recovery is not an uninterrupted fresh test. PID 40840 was stopped.
- `native-rise-create` created GUID 4 Xboxrise, a female Human warrior with all
  appearance values zero, through the native character UI. Character/avatar
  checks and the 146-source/48-output receipt pass. Original Xboxer is unchanged.
- `native-rise-quest` now PASSES all 17 full-loop checks plus avatar checks.
  Quest 783 delivery, quest 7 acceptance/journal, ten kills, fifteen Heroic Strike
  hits, four looted items, 717.81 yards of walking, reward and saved reconnect.
  Server readback: level 2, 308 XP, 51 copper, HP 79, Abbey position
  (-8905.0195, -159.955, 81.9389). Both quests rewarded; ten objectives persisted.
  Do not rerun the fresh-character fixture on Xboxrise or reset earned progress.
- That capture has 25,465 samples, minimum 32,456 KiB free, no asset/region/
  profile/packet faults, 33/131/208/599 ms p50/p95/p99/max. Receipt matched 146
  sources and 48 outputs at launch. Actual return/pass screenshots and server
  readback are in build/evidence/native-rise-*. PID 55436 was stopped normally.
- Dense return succeeds but stop/start switches correlate with animation loads:
  prior sample with loads averages about 104 ms versus 30 ms without, in an
  observed return segment. Investigate animation residency after profiling;
  preserve 4 MiB avatar budget, fallback behavior and 8 MiB free-memory guard.
- New WXTJ telemetry appends ten guest wall-time counters: update, movement,
  stream, camera, actors, draw, UI, present, pacing and total work. Eight work
  phases sum to total; frame_ms belongs to the preceding work. GPU waits are
  included, so these are not isolated CPU measurements. New check-profile.py
  validates the counters. Journey replays now hide the large diagnostic HUD.
- CURRENT owned xemu PID 58036 runs `-WalkReplay` (scenario 15, Xboxnight GUID 2)
  with these counters. Recorder session 91239, native-profile-walk.csv, 540-second
  maximum and finish-on-scenario, is ACTIVE. Its 147-source/48-output receipt
  already passes. Native build succeeded with only the usual lld section warning.
  Do not rebuild while it runs. Run check-walk.py, check-profile.py --walking and
  check-avatar.py when finished. Then investigate measured costs and leave a
  tested normal build before the morning cutoff.
- Current source checks pass; all 14 host suites passed before the native-only
  profiler addition. No active subagents or other build/recorder sessions.

## 05:01 checkpoint â€” animation reuse passes

- Fixed avatar continuity in src/avatar.c and src/pack.c. The complete previous
  clip stays resident across all body/equipment parts until all new parts load.
  Idle/run reuse stays within the existing 4 MiB cache, adds no prefetch and keeps
  two loads per frame. Third sequences remove the optional old clip. Measured
  memory below 12 MiB disables optional reuse; normal budget/headroom guards
  still apply. This is avatar-only; NPC templates retain their existing policy.
- All 14 host suites pass; 539 avatar checks cover full fallback, 100 idle/run
  switches without reloading, third-sequence changes, allocation failure/retry,
  low-memory eviction and budget limits. All 16 prepared profiles pass real-asset
  host idle/run/attack/death transitions; log avatar-cache-profiles.log. The first
  shell glob also included actors.wxp and rejected it as a player profile. The
  corrected strict A+14-hex-digit glob tests exactly the 16 real player profiles.
- Native baseline native-profile-walk passed route, saved reconnect, avatar and
  profiler checks: 4,802 samples, minimum 32,784 KiB, 33/45/74/504 ms. It walked
  365.61 yards; among 2,568 walking frames, 177 had avatar_ready=0 during clip
  transitions. Profile mean movement 0.85 ms, actors 6.70 ms, present 14.27 ms.
- Native-cache-walk repeats the same route with zero unavailable-avatar frames,
  1,468 total walking asset loads versus 1,879 before, maximum walking avatar
  bytes 1,166,508, actor mean 5.94 ms. All walk checks (including new optional
  --continuous-avatar), profile and avatar checks pass. Full capture: 4,842
  samples, minimum 32,908 KiB, 33/43/72/526 ms. NPCs and host work are uncontrolled;
  comparison is observational, not a hardware benchmark. Actual walking PNG and
  avatar-cache-comparison.json are saved. Both launch receipts 147/48 passed.
- The first native-cache-gear capture started at stage 3 and failed checks for
  initial evidence, despite the native scenario completing. Preserve the failure;
  do not count it as passing. Start recorders BEFORE launch for short scenarios.
  The repeated native-cache-gear-full includes every stage and passes appearance,
  avatar --gear-replay and profiling checks. Shield removal/restoration and saved
  reconnect verified, original character progress unchanged. Receipt 148/48 pass.
- Added tools/check-normal.py --baseline earlier-normal.csv for reusable neutral
  session validation: at least 60 seconds received, no replay/staged fixture,
  saved identity/progress/equipment/position/view, one profile and fault/headroom
  checks. Only host checker/docs changed after the full gear build.
- CURRENT normal-build recorder session 88282 is ACTIVE, native-cache-normal.csv,
  135 seconds maximum, started before launch. Normal xemu PID is in build/xemu.pid.
  Build and its 148-source/48-output receipt already pass. Next: finish normal
  capture, run check-normal.py with native-utility-normal.csv baseline, plus
  avatar/profile checks, actual screenshot and server readback. Update PID below.
- Future performance work: pinned nxdk lib/pbkit/pbkit_print.c:242 implements
  pb_draw_text_screen using many pb_fill calls per glyph; its own TODO suggests
  texture copies. Current presentation phase includes GPU scene drain and text,
  so 14 ms is not proven to be text alone. A bounded textured font/HUD backend
  could reduce commands; measure first and preserve MIT notices if borrowing
  nxdk font data. Do not edit the user's global toolchain. No font rewrite yet.
- Full Vanilla, audio/water/spell visuals, cursor/map/talents and remaining menus,
  crowded scenes/hour-long mixed sessions, physical pad and Xbox gates remain.
  The successful quest loop is a development milestone, not completion of the
  entire requested port. Continue autonomous work until the 08:00 PDT cutoff.

## 05:04 final checkpoint for this development turn

- Normal recorder 88282 is FINISHED; no recorder or build sessions remain.
  Owned xemu PID 58144 stays running in normal mode with replay disabled.
  No user UI input was injected during this turn; screenshots use the Sky skill.
- Native-cache-normal passes all 11 normal checks (baseline
  native-utility-normal.csv), avatar and profiling. 2,580 samples with at least
  60 seconds received, minimum 37,952 KiB free, 33/34/35/337 ms p50/p95/p99/max.
  All four staged fixtures are disabled (scenario/CAMP/TESTCHAR NONE, input.rpl
  WXR1 zero records). Original Xboxer GUID 1 remains level 2, XP 768, copper 10,
  inventory 11, shield 2362, HP 79 at the original inn position/view. Server
  readback and actual normal PNG are saved. Other characters retain their state.
- Native-cache-gear-full final figures: 1,925 samples, minimum 37,764 KiB free,
  33/37/54/367 ms. Appearance, gear draw, reconnect and profile checks all pass.
- Receipt nuance: full gear and normal matched 148 sources/48 outputs at launch.
  The new normal checker initially raised TypeError joining a tuple and list;
  that host-only bug is fixed and every check now passes. The explicit
  native-cache-normal.validation-change.json confirms the ONLY changed receipt
  source is tools/check-normal.py and all 48 outputs are unchanged. Native source
  is identical to the tested build. Do not misreport a later full receipt recheck
  mismatch for this checker as a native failure; next build records its new hash.
- Output sizes: default.xbe 1,773,568 bytes; wowx.iso 324,796,416 bytes. Source
  whitespace check passes. The heartbeat remains ACTIVE every 30 minutes until
  08:00 PDT / 15:00 UTC. No duplicate automation or subagents were created.
- Next useful step: investigate presentation/text draw cost and improve the
  controller UI/performance while preserving the tested quest path. Pinned nxdk
  glyph rectangle implementation is described above. Other major pending work
  is tracked in STATUS; do not imply all Vanilla or hardware readiness. Stop only
  this owned emulator through QMP and wait before rebuilding its mounted ISO.

## 05:29 checkpoint â€” texture font complete; map work next

- CURRENT owned xemu PID 21160 is running the tested NORMAL build. All build and
  recorder sessions from this turn are finished. No subagents or new automation.
  Active heartbeat still ends at 08:00 PDT / 15:00 UTC. Stop only the saved owned
  emulator through QMP and wait before rebuilding its mounted ISO.
- New bounded font backend: include/wx_font.h, src/font.c, font_xbox.c,
  font.vs.cg and font_data.h. The grid retains pbkit dimensions/placement and
  scroll behavior; formatting is bounded to 512 bytes. 128x128 RGBA atlas plus
  960-quad vertex buffer requests exactly 157,696 bytes once. GPU alignment is
  additionally reflected in measured free memory. No frame allocations/I/O.
- Font data is copied unchanged from pinned nxdk pbkit_print.c with SPDX/MIT
  notices and source-file hash. No global toolchain changes. Font shader occupies
  vertex slot 96 (three instructions); world shader is 20 instructions at slot 0
  and restores that slot on render. Static assertion prevents overlap. Existing
  texture/color pixel shader is shared. Depth/texture state restores next frame.
- All 15 host suites pass, including 161 font layout/scroll/atlas/vertex checks.
  Actual xemu text and 3D screenshots look correct. Forced fallback uses the same
  bounded grid and original pbkit draw routine. `-LegacyText` writes FONT.BIN=WXFL;
  normal launch resets NONE. FONT.BIN is an output receipt/build dependency.
- WXTK is 214 words/856 bytes: WXTJ plus scene drain, text submit, text drain,
  swap, glyphs, backend, bytes and font failures. tools/check-text.py validates
  subinterval sums and backend/storage. Old wire versions retain compatibility.
- Native-text-baseline-walk (original drawing, split counters) passes walking,
  avatar, profiler and text --baseline checks: 4,788 samples, min 32,844 KiB,
  33/44/66/410 ms. Text mean 2.82 ms; scene drain 11.10 ms. Receipt 148/48 passed
  at launch before subsequent font files/changes. It is historical baseline only.
- Native-font-walk passes same 365.61-yard route/reconnect, continuous avatar,
  profiler and text checks. 4,816 samples, min 32,920 KiB, 33/39/64/565 ms. Across
  2,568 walking frames, text submit+drain mean 0.51 ms, scene drain 10.99 ms.
  154/49 launch receipt passes. Later changes are debug-cache accounting,
  overlap assertion and host checker/probe only. Do not claim full 30 fps.
- Native-font-utility and native-font-legacy-utility both pass all utility,
  avatar and text checks. Exactly 942 wheel frames/105 glyphs each. Text mean
  0.616 ms textured vs 2.906 ms legacy. Textured: 3,991 samples, min 37,792 KiB,
  33/34/35/346 ms. Legacy: 3,795 samples, min 37,948 KiB, 33/35/40/396 ms.
  Both 155/49 receipts passed at launch; the later host map probe source edit
  belongs to the final normal receipt. PNGs of both wheels and font menu saved.
  Comparison JSONs font-walk-comparison and font-utility-comparison are preserved.
- Native-font-normal passes all 11 normal checks (baseline native-cache-normal),
  font, avatar, phase and full current receipt checks: 2,688 samples, at least
  60 seconds received, min 37,788 KiB, 33/34/35/342 ms. All five fixtures reset:
  scenario/CAMP/TESTCHAR/FONT NONE and input.rpl WXR1 zero records. Receipt fully
  matches 155 sources/49 outputs, no post-receipt source changes. Server state and
  actual native-font-normal.png saved. XBE 1,781,760 B, XISO 324,796,416 B.
- Next feature: controller Back-button map, using generic WorldMapArea bounds and
  prepared map textures. Added read-only `wowx_assetc --map-info Data` probe,
  built/ran successfully against all 13 supplied MPQs. Private result is
  build/evidence/worldmap-info.tsv: 51 records, EIGHT FIELDS, every record has all
  12 base BLP tiles (mask 4095). Upstream classic layout advertises optional fields
  8/10 that this actual Vanilla table does NOT have; only use core fields 0..7.
  Fields: ID, map, area, folder, left, right, top, bottom. Elwynn is record 30,
  map 0, area 12, folder Elwynn, bounds 1535.42,-1935.42,-7939.58,-10254.2.
- Map source references already read: pinned WoWee rendering/world_map/
  data_repository.cpp and coordinate_projection.cpp. For this client's direct
  server coordinates use u=(left-player.y)/(left-right),
  v=(top-player.x)/(top-bottom). Sanity: saved Goldshire is ~46.5%,62.2%; Abbey
  ~48.8%,41.7%. These are inference checks, not a native map implementation.
  Inspect composite_renderer.cpp for tile crop/projection before implementing:
  base grid 4x3 of 256 tiles; visible map is normally cropped to 1002x668. Need
  validate real BLP decode and exploration-overlay behavior. No map exporter or
  native map UI exists yet. The new font quad approach can inform a bounded UI
  texture renderer, but preserve world state restoration and input capture.
- Main remaining performance cost in this scene is 3D GPU drain, not text alone.
  Full Vanilla/remaining menus/audio/water/effects, long mixed stability,
  physical controls and Xbox hardware remain gates. Preserve the full goal.


## 05:52 checkpoint â€” native controller map; verification active

- CURRENT owned xemu PID 65300 runs MapReplay. Recorder session 33052 is capturing
  build/evidence/native-map-verified.csv for 200 seconds; launch/build session
  83352 finished. Receipt native-map-verified-receipt.json matches 161 sources /
  101 outputs; no source edits after it (docs only). Normal restore remains due.
- Implemented include/wx_map.h, src/map.c/map_xbox.c, tools/map_export.hpp,
  tests/map_tests.c, tools/check-map.py. New --maps Data output assetc mode writes
  MAPS.WMI plus 51 Z*.WMP files. Runtime shader at slot 100, same three passthrough
  instructions as font; world resets slot 0 and font uses 96. Source image layout
  1024x768 cropped1002x668, downsampled501x334 in512square; one 1,048,672-byte GPU
  texture+quad request, free-on-close, 8MiB guard and rollback. Source overlay
  edge textures may be smaller than DBC extent (Silithus); transparent padding
  fixed after initial conversion failure. All51 actualpacks nowpasshostloader
  no/all-explored validation (155checks), and all16 CTest suites pass.
- Entity explored[64] retains private1.12fields0x457..0x496; delta/truncationtests
  pass. Extra256B/entity reflected in native measuredmemory. Map uses thosebits,
  smallest coordinate rectangle for currentzone, playerprojection, zoom1/2,
  pan, left/right browse,upcontinent,downlocation,Xcenter. Back/B/Startclose and
  latch allgameplay input untilneutral. MenuBacklogout preserved. Maplegend etc
  documented inPLAYTEST; fullquestPOIs/party/cursor/worldoverview/dungeonfloors
  stillopen.51mapimagesdoesnotmean51playablezones.
- WXTL228words912bytes adds14 mapfields; recorder supports allhistoricformats.
  Mapfiles are included inMakefile ISOdependencies and receipts. READMEbuild
  includes --maps preparation. Newunit tests includeinvalidfiles, bounds,
  allocationfailures,lowmemory,inputcapture,projection,explorationalphablending.
- First native-map capture finished:6332samples,min36424KiB,33/34/35/351ms.
  All16mapchecks,avatar,profile,textpass:1982mapframes,sixloads,Elwynn/continent/
  Alterac,threeexploredElwynnoverlays,marker46.5%,62.2%,noinputleak,logout/
  reconnect,unchangedXboxer,free-on-close. Initial161/101launchreceiptpassed.
  After that receipt, hostcheckercorrectedexpectedpanverticaldirection(up means
  smaller v), andmain.c added explicitzero map_ui initializeronly. The fresh
  verification currentlyrunning includesbothchanges andfullymatchesreceipt.
- IMPORTANT Windows displays "We've got an update for you" withrestart6AM.
  ComputerUse seesPickerHost.exewithzerotargetablewindows. Attempts toclick
  AnotherTime throughxemu and WindowsUpdate throughSettings were rejected as
  non-targetPickerHost; noinputapplied. Settings isnowrestoredfromminimized.
  NoOSupdate/securitypolicychanged. shutdown.exe /a returned1116(no shutdown
  inprogress); noReboot tasks appeared inread-onlyscheduledtaskquery. Restart
  HAS NOT been confirmed deferred. Keepcheckpointsfresh. Userasleep; do not
  bypassAPIusingdirectWindowsUIautomation. CurrentmapvisualQAstillobstructed.
  Skycanreadactualxemuwindow butpopupcoverscenter. QMPquery-commands showsno
  screendump. NeedactualmapPNGafterobservation,evenifmarkedpartiallyobstructed.
- Existingheartbeatstillactiveuntil08:00PDT. No newautomation/agents/goals.
  StoponlysavedownedxemuwithQMP/waitbeforewritingitsISO. Serverstatepreserved.


### 05:55 capture note

The fresh native-map-verified recording ended 14 neutral replay frames before
the replay flag cleared (at frame4139 of4153). All1982 mapframes and sixloads,
menu logout/reconnect and restoredposition were captured. A20-second tail capture
on the SAME PID65300 is now running to verify finalneutralstate. Preserve both
originalCSV files; if combining them, record the measuredtimestampgap and source
filenames explicitly. Initialnative-map remains a completepassingcontinuousrun.
Actualnative-map-continent-obstructed.png shows the continentrendering correctly
outside the Windows update modal; center/marker remain visuallyobstructed.
Currentfull161/101receiptstillmatches. Normalrestorestilldue.


## 05:57 â€” normal restore in progress

- The MapReplay process65300 was stopped cleanly through QMP. Its second capture
  was short by14neutralframes; capturedtail600samples laterproves closedmap,
  releasedmemory, no replay and unchangedstate. FullcombinedCSV andsegments.json
  explicitly record83,281ms gap: notcontinuous. Originalnative-map iscontinuous
  andpasses; freshnative-map-verified-full passesall16 mapchecksandavatar.
- Normal recorder session79341 (135sec) and launchsession58837 active. Files
  native-map-normal.*. Nativecodeunchangedsinceverifiedreplay; hostnormalchecker
  adds mapclosed/unallocated andmap/fontfailurechecks. Finalnormalreceiptincludes
  thatchange. Stop/checkthese sessionsbeforefinishing; normal shouldrun after.
- Current docs contain all assetformat/projection/controls details. Newmaps80,498,624B
  total,51records526overlays,maxfile2,090,496B; catalog3892B. Allprivateignored.
- Windows update stillscheduled6AM, NOTdeferred. Read-onlyshutdownabortattempts
  return1116. Actualcontinentscreenshot savedwithobstructionlabel; centerQAstill
  unavailable. NoWindowsupdate/securitypolicychangesoruserstatechanges.


## 05:59 COMPLETE CHECKPOINT â€” current normal build, PID 21712

- Supersedes prior PIDs/sessions. Owned xemu PID21712 is running NORMAL input.
  All build and telemetry sessions from this turn are finished.
- native-map-normal passes12normalchecks (baseline native-font-normal), avatar,
  profile, font and final receipt checks.2349samples,>60sec received,min37452KiB,
  33/34/35/365ms. All5fixtures cleared; mapclosed/ready0/bytes0/failures0.
  Finalnative-map-normal-receipt fully matches161sources/101outputs. No source
  edits after receipt, docs only. XBE1789952B, XISO405405696B.
- Server readback confirms all four characters unchanged; Xboxer at inn remains
  level2 XP768 money10 HP79. Privateonline characterDB dump saved in ignored
  server/backups/characters-20260916-0559.sql (single-transaction; not testedrestore).
- Map implementation and data are complete for this milestone; full Vanilla is
  still incomplete. Firstnative-map is uninterruptedcompletePASS; secondverified
  evidence explicitly has two segments/gap, not a continuouscapture. Actual
  continent PNG shows rendering outside Windows modal; cleanmap/marker/overlay
  visual QA still due. 16host suitespass; all51 mapfiles155loadercheckspass.
- Windows update modal still claimsrestart06:00. It exposes no targetablePickerHost
  window, and shutdown /a has repeatedly returned1116. Deferral NOTconfirmed.
  Do not bypass UItool restrictions or alter update/security/power policies.
  At next heartbeat first check whether Windows restarted: verify ownedxemuPID,
  dedicatedWSL MariaDB/realmd/mangosd-main processes and ports before resuming.
  Restore only project-owned services if required; never reset savedcharacters.
- Best next work: finish clean map visual review if modalcleared, then missing
  playable-loop functions (corpse-run recovery is still unverified), or bounded
  renderer optimization. Read STATUS acceptance matrix. The already-passing
  freshquest loop was XboxriseGUID4, do not reuse its fresh-statefixture.
- Existing overnight heartbeat remainsACTIVE until08:00PDT/15:00UTC. No new
  automations, agents or goals were created. Keep the stock64MiB/fullVanilla goal.

## 06:34 corpse recovery checkpoint and sustained test in progress

- Native `native-corpse.csv` completed all 16 corpse checks plus avatar, text and
  phase-timing checks. 9,033 samples; minimum 32,620 KiB free; frame intervals
  33/35/52/597 ms (p50/p95/p99/max), measured in xemu only.
- Xboxspirit GUID 5 was created through the native controller keyboard, then
  died in actual server combat. Release, a 30-second reclaim delay, corpse map,
  ghost reconnect, a 198.35-yard walk back, owned-body reclaim, and alive reconnect
  all passed without kills, loot or progress reset. Level 1, XP 0, money 0, HP 60
  at (-8803.03,-176.988,81.8151). Other four characters retain their prior state.
- Actual clean screenshots: native-corpse-map.png and native-corpse-passed.png.
  The Windows update dialog is no longer obscuring xemu. No policy was changed.
- The corpse-run launch receipt matched 167 sources/101 outputs at launch. It is
  historical now: subsequent edits add the sustained-test fixture and WXTN
  telemetry. Host tests: all 17 suites passed before those fixture-only edits.
- Current owned xemu PID 48340 runs a 300-second sustained-test smoke fixture.
  Recorder session 7034 (650-second maximum); build session 19082 is finished.
  native-soak-smoke-receipt matched 169 sources/102 outputs. The smoke must pass
  before a one-hour run. Check with check-soak.py --minimum-seconds 300
  --minimum-cycles 2. It exercises repeated walking, UI and saved reconnect;
  it does not complete crowded-combat, forced-disconnect or hardware gates.
- New SOAK.BIN stores bounded duration; normal launch clears it along with the
  five prior fixtures. Normal restore remains due after sustained testing.
  Never rebuild the mounted ISO while the dedicated emulator is running.

## 06:41 sustained smoke passed; one-hour run starting

- `native-soak-smoke` passed all 16 sustained-test checks plus avatar, text and
  timing checks. Two full laps, 731.22 yards walked, all six menu states per lap,
  both maps, saved reconnects and no errors. 10,325 samples; 331,927 ms elapsed;
  minimum 31,500 KiB free; 33/41/78/538 ms p50/p95/p99/max. Map memory released.
  All 17 host suites also pass after the sustained fixture was added.
- Clean continent/player-marker screenshot: native-soak-continent.png. This
  completes the visual check previously blocked by the Windows update modal.
- Stopped smoke xemu PID48340 cleanly through QMP. A fresh private online dump
  is server/backups/characters-20260916-0640.sql (75,331 bytes; restore untested).
- One-hour recorder is a hidden background Python process, PID18704, also stored
  in build/soak-recorder.pid. Its maximum duration is 4,200 seconds. Output is
  native-soak-hour.csv; stdout/stderr are native-soak-hour-recorder.log and
  native-soak-hour-recorder-errors.log. It exits ten seconds after stage8/9.
- Launch/build session54607 is starting `run-xemu.ps1 -SoakReplay` with default
  3,600-second minimum. Finish this build and confirm its receipt and initial
  telemetry. No other recorder/build sessions remain active.
- DO NOT stop/rebuild this dedicated emulator while the one-hour test is making
  progress. Expected completion approximately 07:44-07:47 PDT after a whole lap.
  The 07:00/07:30 heartbeats should inspect progress and preserve the uninterrupted
  capture. Source/output receipt should remain unchanged during this run.
- After recorder exits, run check-soak.py native-soak-hour.csv with defaults,
  then avatar/profile/text checks and SQL readback. Record actual screenshots
  and free-memory trend. This is walking/UI/reconnect evidence; crowded combat,
  forced link loss and hardware still remain separate release gates.
- Restore normal input afterward: quit only this xemu through QMP, wait for its
  saved PID, start a >=135-second normal recorder BEFORE run-xemu.ps1, and check
  normal capture against native-map-normal.csv. Capture matching build receipt,
  verify six staged fixtures cleared, update STATUS/OVERNIGHT and leave normal
  emulator running by the 08:00 cutoff when feasible. No saved-state resets.

### 06:43 active-hour-run handoff (supersedes earlier PIDs)

- Owned xemu PID22124; background recorder PID18704. No build/tool sessions remain
  running. The recorder is a standalone hidden Python process; use its saved PID
  and CSV/JSON/log files, not a write_stdin session.
- native-soak-hour-receipt and its receipt-check matched all169 sources/102 outputs.
  Initial telemetry confirms kind17, limit3600000 ms, Xboxnight GUID2, world
  revision2 and active walking stage20. At15,378 ms, free memory33,348 KiB.
- Both source and output identities should remain stable while this test runs.
  Do not run run-xemu.ps1/build-xbox.ps1/prepare-replay.py/stage-world.py until
  it completes (they overwrite the mounted disc or its inputs).
- The host launcher message "Live controller input selected" is misleading for
  state-driven fixtures: input.rpl is empty but scenario.bin is WXCH. Telemetry
  confirms the active injected scenario. Fix that display-only message AFTER the
  hour run, before final normal build/receipt; do not mislabel this as live input.
- Next: inspect CSV progress at scheduled heartbeat; preserve the uninterrupted
  run. Its final result is still PENDING. At terminal stage8/9 wait for recorder
  exit, check evidence, then restore/verify normal build as specified above.
- Full port is incomplete. Corpse recovery and two-lap smoke are proven; one-hour
  result, combined combat stress, broader Vanilla coverage and hardware are not.

### 06:52 renderer review while hour run continues

- One-hour run remains untouched (xemu22124, recorder18704), with three completed
  laps at456,806ms, no asset/region failures and31,536KiB free in the current UI.
  No early one-hour acceptance claim. Do not stop it to test the candidate below.
- Prepared an UNAPPLIED renderer candidate under build/experiments/draw-batch/.
  README.md, candidate.patch, baseline-sha256.txt, exact-host-results.txt and
  pack-batch-counts.json record its basis. It groups960indices per GPU primitive
  using four <=128-word pbkit CPU blocks. Exact-loop FIFO checks preserve every
  index and avoid a command-buffer reset during a primitive (1,494,544 checks).
  This does not verify GPU execution or measured speed. Current source/build
  receipt is unchanged. Keep the tested renderer for morning if native A/B work
  would delay normal restore. Do not claim static batch-count reductions as FPS.
- Two small build-workflow fixes are due AFTER the hour completes: add SOAK.BIN
  to explicit XISO Makefile dependencies, and make prepare-replay.py distinguish
  state-driven scenario.bin fixtures from live controls in its final message.
  Current launcher still rebuilds XISO because it regenerates entropy.bin, so
  the mounted one-hour image is correct (telemetrykind17/3600000ms confirms it).

## 07:11 CURRENT: monitored one-hour retry (supersedes all previous PIDs)

- Latest user said "keep going great progress has been made" atabout07:01.
  Existing heartbeat updated to every15minutes through09:00PDT/16:00UTC to finish
  the restarted hour and normal restoration. After that checkpoint, pause the
  overnight heartbeat; do not claim the full port is complete.
- Originalhour xemuPID22124 exited by06:59:38; causeunknown, noexitcode retained.
  No matching app crash event/dump/logerror found. OriginalCSV preserved:
  native-soak-hour.csv,31,178samples,1,044,335ms,sixlaps,2,539.64yards,min31,300KiB.
  check-soak FAIL: duration/completion/minimumlaps/returnedposition. Other checks
  pass. Logs copied to native-soak-hour-xemu-stdout/stderr.log, explicit reason
  recorded in native-soak-hour.interruption.json. Owned recorder18704 was stopped.
- XboxnightGUID2 now at(-8913.1,-142.268,82.2122), yaw5.39382. Level2XP324money54HP79
  preserved. DO NOT reset it or run the strict start-at-Abbey fixture on it.
- Current dedicated xemuPID56712 and recorderPID59824 (saved in build/xemu.pid and
  build/soak-recorder.pid). Recorder is hidden standalonePython, not a tool session.
  Current native-soak-retry.csv/log/json family. Recorder uses --guest-pid-file
  build/xemu.pid; stdout confirms Watching dedicated xemuPID56712. Buildsession
  34567 finished. No other build/record sessions active.
- Run selects XboxdawnGUID3, which was already at the correct Abbey start with
  level2XP313money49HP79. Currenttelemetrykind17/limit3600000, activewalkingstage20,
  18,564ms elapsed,32,684KiB free, nofailures. Expected completion08:10-08:13PDT.
  Do not stop it at the OLD08:00 cutoff. Latest continuation now allows through09:00.
- Current receipt matches171sources/102outputs (native-soak-retry-receipt.json and
  receipt-check). Do not change sources or mounteddisc while it progresses.
  Renderer candidate remains UNAPPLIED; preserve stable test behavior.
- Implemented tools/process_watch.py, tests/process_watch_tests.py (15PASS),
  recorder exit/silence detection + lifecycleJSON, checker lifecycle acceptance,
  explicit SOAK.BIN Makefile dependency, and corrected launcher input description.
  Synthetic recorder integration process-watch-fixture.* received4fakepackets,
  capturedrealchildexit7 and passed; strictly NOT native game evidence.
- Afterterminalstage8/9 and recorderexit: check-soak.py native-soak-retry.csv,
  avatar/profile/text checks, actual screenshots and serverstate. Preservefailure
  ifany; never fabricate/concatenate an uninterruptedhour. Reviewmemorytrend.
- Then restore NORMAL: stoponlyownedxemuviaQMP/waitPID, start135+secondnormal
  recorder WITH --guest-pid-file build/xemu.pid, launchrun-xemu.ps1, verify normal
  against native-map-normal.csv (XboxerGUID1 unchanged), avatar/profile/text,
  matching receipt and all6fixturescleared. Leave normalrunning, updateSTATUS/
  OVERNIGHT, pause heartbeat viaautomation_update, reportcheckpoint/fullportlimits.


### 07:31 retry progress (still pending)

- Dedicated xemu 56712 and recorder 59824 remain alive. The retry has passed
  the previous attempt's 17-minute interruption point: 36,584 samples,
  1,228,263 ms elapsed and seven complete walking/UI/reconnect laps.
- Minimum free memory so far is 30,784 KiB. Asset, region, avatar selection,
  spellbook, map and font failure counters remain zero. Latest interim table is
  native-soak-retry-progress.json; it is explicitly not final acceptance.
- Map texture bytes return to zero at each completed UI lap; avatar bytes at
  those points are consistently 1,466,060. End-of-lap free memory varies from
  32,272 to 31,804 KiB as resident scene cache also varies. Review the complete
  trend after the hour; this does not yet establish either a leak or a plateau.
- No source or staged-disc changes, rebuilds, guest restarts, character resets
  or new native input were made during this check. The draw-batching candidate
  remains unapplied. The same 08:10-08:13 expected completion and required final
  checks/normal restoration in the 07:11 handoff still apply.
- Existing 15-minute heartbeat is ACTIVE through 09:00 PDT. Preserve this run
  on subsequent wakeups and pause that heartbeat after verified normal restore.


### 07:39 monitored retry remains active

- xemu 56712 and recorder 59824 are alive; telemetry is fresh and the recorder
  error log is empty. The launch receipt still matches all 171 sources and
  102 outputs. No recorded source or staged-disc changes were made.
- At the latest interim scan: 52,609 samples, 1,771,961 ms elapsed, 11 complete
  laps, stage 21. Minimum free memory is 30,712 KiB; maximum sample gap is
  1,084 ms. All six observed client failure counters remain zero.
- Each completed UI lap releases the map texture. Recent end-of-lap free memory
  varies between 31,800 and 31,924 KiB (laps 8-11), with varying scene cache.
  Full-hour completion and memory-trend review remain pending.
- Preserve the uninterrupted run. Existing final validation/normal restoration
  instructions and 09:00 heartbeat cutoff remain in force.


### 07:54 monitored retry remains active

- Same owned xemu 56712 and recorder 59824 are alive, with fresh telemetry and
  an empty recorder error log. No source, disc, configuration or process changes.
- Interim scan: 78,421 samples, 2,656,249 ms elapsed (44.27 minutes), 16 complete
  laps; stage 6 is the next saved reconnect. All six client failure counters
  remain zero. Maximum sample gap remains 1,084 ms.
- Minimum free memory remains 30,712 KiB, unchanged since the previous check.
  UI laps 12-16 finish with 31,772-31,924 KiB free, map bytes zero, and the same
  1,466,060 avatar bytes. This is still interim evidence, not hour acceptance.
- Next scheduled check is near the one-hour target. Do not stop early or rebuild
  while the scenario/recorder is progressing. Follow the 07:11 final checks and
  normal restoration after terminal completion; preserve any failure honestly.


## 08:17 FINAL OVERNIGHT CHECKPOINT (supersedes earlier active-run instructions)

- One-hour retry completed normally: native-soak-retry.csv, 107,864 samples,
  3,661,998 ms, 23 complete walking/UI/reconnect laps, 8,409.05 yards, minimum
  30,684 KiB free. All 17 soak checks plus avatar/font/profile checks PASS.
  Lifecycle is terminal_scenario; xemu 56712 did not exit during the run.
- Actual native-soak-retry-walking.png and native-soak-retry-passed.png were
  inspected and saved through Computer Use. Matching 171/102 receipt was
  verified before normal restoration; that receipt is now historical because
  the normal launch changed fixture/disc outputs. Logs and server snapshot saved.
- Initial world-entry stall: 1,084 ms, predominantly streaming (529 ms) and
  actors/avatar (552 ms). In-world walking also includes 389-399 ms streaming
  frames. Preserve native-soak-retry-long-frames.json; 30 fps is not established.
  End-of-lap memory after lap 7 varied within 31,772-31,928 KiB, with map texture
  released every time. No continuing decline observed for this workload.
- Owned test xemu 56712 was stopped only after evidence was complete. Current
  NORMAL xemu PID 10736 is running (build/xemu.pid). Native normal recorder 45012
  and soak recorder 59824 have EXITED; saved recorder PID files are historical.
  No tool command sessions are running.
- native-endurance-normal.csv PASS: 13 normal checks plus avatar/font/profile,
  all six fixture files disabled, map unallocated, 3,927 samples, 131,457 ms
  received telemetry, minimum 37,444 KiB free, 33/34/35/373 ms intervals.
  Lifecycle duration_limit, no guest exit. Current receipt matches 171 sources
  and 102 outputs. Actual native-endurance-normal.png inspected and saved.
- Xboxer GUID1 remains level2/XP768/money10/HP79/inventory11 at the inn with
  unchanged equipment and view. All five characters match the end-of-hour
  read-only snapshot. Xboxnight remains saved inside the Abbey entrance; do
  not reset it or run strict Abbey-start fixtures on it. Xboxdawn is back at
  the valid fixture start. PLAYTEST soak example now names Xboxdawn explicitly.
- New private online character backup: server/backups/characters-20260916-0814.sql
  (75,192 bytes); restore untested. Database/server remain running.
- XBE 1,798,144 bytes; XISO 405,405,696 bytes. Renderer candidate is UNAPPLIED.
  Keep game assets, credentials and private server data outside source control.
- Overnight heartbeat wowx-overnight-development is PAUSED via the app tool
  after the requested checkpoint. A later explicit continuation can resume
  development; do not automatically restart the overnight monitor.
- Remaining gates: streaming stalls, complete account/controller UI, audio,
  water/spell effects, broader class/world systems, combined combat/quest/death
  endurance, forced network loss, crowded scenes, physical controls and Xbox
  hardware. This scoped hour pass does not complete the full Vanilla objective.


## 09:38 renewed full-parity development (supersedes overnight pause)

- User explicitly requested continued work toward full PC Vanilla functionality
  and UI/main-menu parity, retaining only agreed controller/performance changes.
  Existing heartbeat was updated and resumed as WOWX Vanilla parity development,
  every 30 minutes with no old 09:00 cutoff. No duplicate automation/task.
- Full compatibility matrix and acceptance scope are in docs/PARITY.md.
- Exact previous checkpoint preserved under build/checkpoints/20260916-0817:
  default.xbe, wowx.iso, receipt.json and sources.zip. All 171 archived source
  bytes match the previous receipt, including reconstruction of pre-extraction
  assetc.cpp. Private game/credential content remains in ignored output directories.
- Added local archive listing/extraction/BLP-to-TGA commands to assetc, plus
  prepare-ui.py/Pillow11.3.0. Fixed INTERFACE.WUI atlas: 1,054,400-byte file,
  285 original font glyphs and seven menu sprites from supplied Vanilla archives.
  Runtime UI requests 1,376,256 bytes for atlas/2,048 quads, with 8 MiB guard.
- New ui.c/ui_xbox.c replaces character roster/create/name/confirmation drawing
  using original logo, FRIZQT__/MORPHEUS and red buttons. This is a foundation,
  not completed PC layout/preview/login parity. Other in-world UI is still legacy.
  164 host bounds/failure checks plus eight actual-pack checks pass; all 18 CTest
  suites pass. Native build has no new warnings. WXTO is 1,016 bytes, four UI stats.
- Old normal xemu10736 was already absent when QMP stop was attempted; no listener
  or alternate xemu existed. Cause unknown, after completed neutral capture.
  normal-post-capture-closure.json records this. It had not run the new UI code.
- CURRENT native test: xemu34652, recorder29264, native-vanilla-ui.* files.
  CharacterScreensReplay has 5,398 frames; recorder is 280 seconds. Host launch
  session73006 finished. Test sources and staged disc should remain unchanged
  until capture ends. Original characters are only browsed; empty draft cancelled.
- Actual first roster/keyboard screenshots saved (native-vanilla-ui-*-first.png).
  UI is readable and telemetry shows ready1, fixed bytes, quads263, failures0.
  VISUAL FIXES DUE after capture: solid rectangles sample the center of the white
  atlas texel (full 3x3 UV causes broad transparency gradients). Top/bottom frame
  border tiles must rotate 90 degrees and tile, following pinned WoWee
  src/ui/widget_renderer.cpp around698. Current straight UV produces silver bands.
  Keep the first screenshots as diagnostic evidence; do not call visuals passed.
- After capture: run check-ui.py, avatar/profile/text and verify saved progress,
  then fix those sampling defects with host UV tests. Rebuild and rerun the
  character screen capture; inspect all three states and restore normal controls
  before moving to the account/title/realm UI milestone. Do not pause new ongoing
  development merely because this local UI checkpoint passes.

## 09:50 original-art UI validation in progress

- Solid highlights and rotated/tiled horizontal borders are now corrected.
  Host checks: 174 geometry/bounds/allocation assertions plus eight actual-pack
  checks. native-vanilla-ui-fixed.csv passes UI/avatar/profile/text checks and
  preserves Xboxer and all equipment/position; 6,962 samples, minimum 35,892 KiB
  free, fixed UI allocation 1,376,256 bytes, maximum 367 quads, no failures.
- A second identical visual trace is CURRENTLY RUNNING: xemu62240, recorder29520,
  build/evidence/native-vanilla-ui-visual.csv (280 seconds); build session68932
  finished. Sources/disc remain fixed until the recorder ends. Receipt matches
  178 sources/103 outputs. Roster and creation screenshots inspected/saved;
  keyboard still due. Activate the exact xemu window before each Sky capture:
  background capture has sometimes shown only the dim world layer.
- After visual trace, check UI/avatar/profile/text and progress, then restore a
  normal capture before account/title/realm implementation. Development remains
  active; no pause at this checkpoint. Full front-end/game parity is incomplete.
- Protocol audit found a concrete bug to address next: auth_client.cpp skips
  five version bytes for realm flag4, but pinned vMaNGOS pre-6299 realm packets
  explicitly do not serialize that extension. Use AuthSocket.cpp:1032 onward
  as the primary format reference; add malformed and flagged-realm fixtures.

## 10:11 controller account/realm implementation in native validation

- Previous visual UI run finished: 6,946 samples, UI/avatar/profile/text PASS;
  roster/create/keyboard screenshots inspected. Owned xemu62240 stopped after
  capture completion. No character reset/creation.
- Added bounded realm.c parser (all255 entries/65,535-byte packet), safe selected
  session API, correct Vanilla flag4 handling, empty/offline list support.
  239 realm checks, 326 login UI/password-masking checks, 24 auth scenarios and
  all20 CTest suites pass. Xbox builds with only prior warnings.
- Added controller account/password/IPv4-server keyboard, authentication error,
  realm selection, cancel/retry, and real character-picker handoff. Normal launcher
  now opens the account menu with empty password. -AutoLogin is explicit neutral
  development mode; gameplay fixtures retain their shortcut. -LoginReplay stages
  LOGIN.BIN=WXLT, preloads private disposable credentials and injects controller
  input. This does not verify physical controller transport or full PC UI parity.
- CURRENT capture: xemu45224, recorder57816, native-login-flow.csv, 300 seconds
  from10:08:02 (endsabout10:13:02). Launcher session67704 finished. Receipt matches
  187 sources/104 outputs. Preserve source and mounted disc until recording ends.
  Login replay already reached stage16 in saved Xboxer world: three auth attempts,
  one rejected password, one realm cancellation, successful retry/realm/roster.
  Actual account/keyboard/realm/world screenshots saved and inspected.
- Visual fix due AFTER capture: login_xbox.c field panels use height28 but shared
  wx_ui_panel deliberately requires >=32, so input boxes lack borders. Make them
  at least32px, rebuild and visually verify. No native counter/asset errors seen.
- WXTP is1,060bytes/265words; recorder receives2048bytes and adds11 public front-end
  counters. check-login.py validates fixture/menu state and saved progress.
- Host world/character protocol regression commands: sessions72350/10753 (inspect
  completion; no private live server affected). Next finish all login checks,
  correct border, validate updated normal world with -AutoLogin, then validate
  default idle account-menu boot and leave that clean build staged/running.
- Title M2 has7,970vertices/57bones and authoredcamera metadata; original portal
  scene/material/effect support and PC layout/preferences still unfinished.
  Continue full-parity development; do not pause heartbeat at this checkpoint.

## 10:19 validation correction and current capture

- native-login-flow completed normally and passes login/avatar/UI/font/profile
  checks; all5 server character records unchanged. 7,586samples, min35,744KiB,
  33/34/37/373ms guest intervals. 68 world/10 character protocol fixtures pass.
- Border height fixed to32px, population categories aligned to pinned WoWee,
  and world rendering explicitly disables inherited UI blending. Targeted tests
  pass:242 realm/326 login checks. No new native compiler warnings.
- Agent sequencing error INVALIDATED native-login-normal stability acceptance:
  QMP clean quit occurred at17:16:57UTC, eight seconds before the recorder's
  deadline. Its lifecycle is guest_exit/code0, not a spontaneous emulator crash.
  Preliminary normal-check JSON is explicitly marked failed with this reason.
  The next native-login-menu recorder failed to bind UDP39001 while the first
  recorder was exiting; its error log and attempt note are retained.
- check-normal.py now requires an actual completed duration_limit lifecycle and
  null guest_exit. Missing lifecycle can no longer pass. Both old recorders are
  confirmed exited and UDP39001 was released before starting the replacement.
- CURRENT native-login-menu-retry: xemu66788, recorder65760, recorder started
 10:18:18PDT for120seconds (deadline10:20:18). This records the already booted
  manual menu with mode1, attempts0, worldinactive and zero replay. Screenshot
  native-login-menu-retry.png inspected: corrected borders and empty password.
  Receipt187sources/104outputs matches. No running build/exec sessions.
- Preserve sources/disc until recorder65760 exits and its lifecycle file exists.
  Then validate check-login.py --menu, UI/profile and receipt, repeat the neutral
  world run under a NEW capture name with -AutoLogin and full recorder completion.
  Finally stage/verify the default MENU launch and preserve a source/binary
  checkpoint. Do not declare the earlier prematurely stopped neutral run passed.

## 10:33 handoff: login checkpoint complete, title work next

- Final manual-menu recorder65996 completed duration_limit, guest_exit null,
  2,797 samples. Login --menu/UI/profile/187-source104-output receipt pass.
- Corrected neutral world retry30572 completed independently: 3,986 samples,
  all normal/avatar/UI/profile/font checks pass; min35,744KiB, same saved state.
- Current xemu65688 is the final manual menu. No recorder/build sessions active.
  Safe to stop this owned emulator for next title-renderer build. Sources/disc
  need no further freeze for completed captures.
- Verified build in build/checkpoints/20260916-1032. Source archive verifies
  every187 source SHA against final receipt; private assets/credentials excluded
  from source zip. XBE/XISO remain private development artifacts.
- Next implement supplied UI_MainMenu M2 scene with authored camera/materials/
  animation and bounded loading. Read-only analysis main-menu-model-info.txt and
  main-menu-model-inspection.json exists. Do not claim effects/PC parity complete.
- Active30-minute heartbeat must continue; old pause-at-checkpoint superseded.

## 10:49 title scene native validation underway

- Login checkpoint20260916-1032 is safely archived (187source SHA checks).
- New wx_backdrop core/cooker/NV2A renderer loads supplied UI_MainMenu geometry,
  independent bone/alpha tracks and true alpha/additive materials.4,142,740bytes
  requested CPU+GPU. 215host checks and all21CTest suites pass. Actual asset
  multi-loop finite checks and9independent WoWee/GLM comparisons pass.
- native-title-first completed3031samples,35,216KiB minimum,33/34/34/38ms.
  Counter/lifecycle/194source105output checks pass, but screenshot shows excessive
  field of view. Preserve as diagnostic; do not call visual parity complete.
- Corrected FOV conversion now matches reference renderers' classic M2 scale.
  CURRENT xemu67064, recorder64404: native-title-framing,135sec from10:47:57PDT,
  due10:50:12PDT. Freeze code/mounted ISO until recorder exits and lifecycle exists.
  Then inspect screenshot and run menu/UI/backdrop/profile/receipt checks.
- Next run -LoginReplay to verify title memory releases at roster/world transition,
  then neutral world and restore default manual menu. Do not reset saved characters.
- Sources now195 (newcheck-backdrop.py), outputs105; WXTQ telemetry1092bytes.
  Shader slot112,18instructions; c8 literal0/1 explicitly set.
- Title effects/lighting/exactPC layout, character previews and fullmatrix still
  open. Active30min heartbeat continues, no pause-at-checkpoint.

## 10:54 corrected title replay and effects study

- native-title-framing finished normally:3031samples,35,212KiBminimum,
  33/34/34/38ms. Backdrop/menu/UI/profile/195source105output receipt pass.
  Actual corrected and later screenshots inspected and preserved. Full title
  effects/lighting/widgets still incomplete.
- CURRENT native-title-login: xemu67356,recorder55156,330seconds from10:51:05PDT,
  due10:56:35PDT. Preserve sources/disc until recorder exits+lifecycle confirms.
  Then check-backdrop --world-transition, check-login --baseline prior neutral,
  avatar/UI/profile/font/receipt, saved state, and restore clean manual menu.
- Read-only study in build/experiments/title-particles and evidence JSON:28emitters,
  sampled rate*life upper sum1359,flags1/33/49,blend4. Not a bound or native proof.
  PINNED LOADER BUG: Vanilla v256 has uint16blend0x28 anduint16emitterType0x2a;
  loader readsu8at0x28/0x29, misreporting everytype0. Actualassettypes1and2.
  Raw header0x13c/count28/stride504 evidence preserved. Independent schema linked
  in studyREADME. Use version-aware local adaptation; preserve upstream pin.

## 10:59 handoff: title login transition passed

- native-title-login completed8708samples/duration_limit/noexit. All16login,
  title-release/avatar/UI/font/profile and195source105output checks pass.
  Minimum35,212KiB,33/34/37/476ms. Title bytes0 throughout saved world. Actual
  world screenshot inspected;5DBcharacters match priorcheckpoint.
- CURRENT neutral native-title-normal: xemu48160,recorder53176,135seconds from
  10:57:51PDT, due11:00:06PDT. Preserve sources/ISO until complete. Check-normal
  against native-login-normal-retry.csv; avatar/UI/font/profile and receipt.
- Then stop ownedxemu only after recorder/lifecycle complete, restore normal
  manual menu via run-xemu.ps1 (no flags), capture/check it and preserve final
  source/XBE/XISO checkpoint. Leave clean menu running, heartbeatACTIVE.
- Wrong --legacy checker parameter caused an expected-backend mismatch on the
  title-login capture; both wrong-mode files preserved. Correct GPU-backend
  invocation passes. No source/runtime failure or altered fixture to hide it.
- All21host suites now freshly rebuilt and pass. Remaining titleparticle parser
  adaptation, effects/authored lighting, PCwidgets/preview and fullmatrix open.

## 11:01 neutral title regression complete; final menu recording

- native-title-normal passed all13normal checks plus avatar/UI/profile/font/
  195source105output receipt. 2,997samples; all7fixtures disabled; saved Xboxer
  progress/view/position/equipment unchanged. No title allocation on auto-login.
  Timing and minmemory in native-title-normal-summary.json; screenshot inspected.
- Final default manual-menu launcher has now run. Resolve current IDs from
  build/xemu.pid and build/title-recorder.pid; capture native-title-final lasts
  135seconds from recorder start. No source edits before lifecycle completes.
  Check-backdrop/login--menu/UI/profile/receipt; screenshot; archive195sources+
  currentXBE/ISO/receipt and updateddocs. Leave owned final xemu running, no recorder.
- Do not reduce next work to visual-only polish: title particles/lighting, PC
  widget workflows/character previews and the rest of the functional matrix remain.

## 11:05 CURRENT HANDOFF — title checkpoint complete

- Current project-owned xemu: PID 57928, clean manual account menu, LOGIN.BIN=MENU.
  Recorder 40100 has exited; lifecycle is duration_limit / guest_exit null.
  No recorder/build sessions are active. Safe to stop this emulator through QMP
  and wait for exit before changing its mounted ISO. Leave no duplicate emulator.
- `native-title-final`: 3,084 samples. Backdrop/menu/UI/profile and exact
  195-source/105-output receipt checks pass; actual screenshot inspected.
- `native-title-login`: 8,708 samples, all login/title-release/avatar/UI/font/
  profile checks pass. Three auth attempts, one rejection, one cancellation;
  roster and Xboxer world entry. Title bytes become zero. Saved DB records for
  all five characters unchanged. Minimum 35,212 KiB, 33/34/37/476 ms intervals.
- `native-title-normal`: 2,997 samples, all 13 neutral checks plus avatar/UI/
  font/profile pass; every replay fixture disabled. Minimum 35,724 KiB and
  33/35/37/345 ms. Actual world screenshot saved. Normal checks were run while
  LOGIN.BIN was NONE; do not rerun against today's restored MENU staging.
- Preserved checkpoint: build/checkpoints/20260916-1105, XBE/XISO/source ZIP/
  receipt/notes. Every 195 archived source hash and both binaries verified.
  Earlier 1032 and 0817 checkpoints retained. All 21 freshly built host suites pass.
- Title is original mesh animation/material rendering, not finished visual parity.
  Next: version-aware Vanilla particle-header adapter, bounded authentic effects,
  authored lighting/fog, PC UI workflow/layout and character previews. Keep the
  broader functional matrix moving; full game and physical hardware remain open.
- Particle research: build/experiments/title-particles/README.md and two evidence
  JSONs. Raw Vanilla uint16 emitter type is at +0x2a (actual values 1/2); pinned
  WoWee reads the later uint8 +0x29 and gets zero. Preserve the upstream revision;
  add a tested local adaptation. 28 title emitters, sampled rate*life sum 1,359
  (not an absolute bound); all additive blend 4. No native particles added yet.
- WXTQ telemetry: 273 words / 1,092 bytes. TITLE.WXB: 2,139,220 file bytes,
  4,142,740 requested CPU+GPU bytes. Independent keyed animation matches pinned
  WoWee/GLM at nine times. Correct classic camera FOV scale is in the exporter.
- Keep heartbeat `wowx-overnight-development` ACTIVE every 30 minutes. No old
  9 AM cutoff or pause-at-checkpoint. User requires continued full PC parity
  with stock 64 MiB hardware/controller adaptations, without routine questions.

## 11:05 user steering — increase implementation per test cycle

The user says testing is too frequent relative to new functionality. Adopt the
new cadence in docs/PARITY.md immediately. Build related features together;
compile and run targeted host checks as needed, then one combined xemu acceptance
run for the batch. Do not routinely repeat login + neutral + restored-menu
captures or archive every minor change. Endurance remains milestone-based.
The next batch should advance title effects/lighting and shared character-preview
scene support before comprehensive native testing. Existing tested checkpoint
20260916-1105 remains the rollback. No new tests are needed for this workflow-only
update. Full PC parity, memory limits, saved characters and autonomous continuation
remain required. Current xemu57928 is idle at the tested manual menu; no recorder.

### 11:43 CURRENT HANDOFF — effects checkpoint complete

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

Next batch: integrate real roster/create previews using existing prepared avatars
and racial scene camera/attachment placement, with original PC screen workflows.
Continue the larger-batch cadence in PARITY.md. Do not treat the two prepared
racial backgrounds as completed previews or implement zones individually.
Keep wowx-overnight-development ACTIVE. No routine approval is needed.
Safe to quit owned49428 through QMP and wait before rebuilding its mounted ISO.
Current receipt is native-effects-manual-receipt.json; accepted combined runtime
receipt is native-effects-final-receipt.json. Both have identical sources/XBE;
launch fixtures, entropy and therefore ISO differ. First/cached run source ZIPs
are preserved separately, including the first run's known failure.

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

## 2026-09-16 — physical Xbox test handoff

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
SpellRange data now feed WXS v2 (22,357 records, 4,426,702 disc bytes); v1 remains
readable with unknown extended metadata. Fixed spellbook is 105,064 Xbox bytes
plus the 44,714-byte ID index, with one bounded record read per frame.

The parser tracks cast starts/completion/failure, pushback, finite/indefinite
channels and channel shortening. B sends the matching Vanilla cast/channel cancel
command. Instance charges survive inventory snapshots; item prototypes identify
charge slots. START/GO decoding is shared with cooldowns. Fixed cooldown state
is now 65,336 bytes; cast state 28 bytes; inventory 8,792 bytes. No new GPU allocation.

All 81 encrypted world scenarios pass, including command-wire cancellation and
malformed channel/delay rejection. Targeted host checks: 205 cast lifecycle,
40 feedback, 1,097 catalog/controller, 4,666 icon/UI and 13 telemetry cases. All
22,357 supplied spell records validate. The 4,511 global and 1,445 base cooldown
checks also pass this batch.

One combined offline native capture: build/evidence/native-action-feedback.csv
and .actions.csv companion, all 12 injected phases. 1,736 main / 1,735 companion
samples; minimum free 42,096 KiB; guest frame p50/p95/p99/max 33/34/34/35 ms; at
most 535 UI quads. QMP confirms 67,108,864 bytes RAM. No parser/asset/UI/modifier
failures. Actual screenshot: build/evidence/native-action-feedback.png. Its cast
bar overlaps the fixture notice; that notice is not part of the live UI.
Resources/item quantities/charges are synthetic fixture data. This is not live
class combat, physical input or hardware performance, and does not satisfy the
combined front-end/live-world acceptance gate.

The credential-free disc/source ZIP is in build/actions-test-20260916. All 258
source and 60 mounted output hashes verified. Only owned xemu PID 51456 was quit
after capture. Candidate: build/candidates/20260916-action-feedback (XBE 1,921,024
bytes; XISO 498,991,104 bytes). No verification.json. Stable hardware kit, server,
shared account, saved characters and credentials remain untouched.

Remaining: per-skill costs, overlapping/charged aura identity, exact movement
leeway, facing/LOS/faction/reagents/combo and all class-specific availability.
Cast bar is functional native layout, not final original artwork. Full parity,
live integrated acceptance and release gates remain open. Approved roadmap:
docs/ROADMAP.md. Next: shared fog and bounded world streaming, retaining the
account reservation. Third-party addons are deferred; native original UI remains
required. Autonomous continuation remains active with no morning cutoff.


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

Latest completed batch: see the matching STATUS.md section and STREAMING.md.
The staged payload/index streamer, prefetch/retention, pending-memory accounting,
collision priority and loading/error distinction are implemented and built.
Host: 3,562 staged loader checks, 343 runtime, 539 avatar, strict auxiliary decoder
and 13 telemetry tests pass. Real-pack one-tick-per-frame host traversal passes.

Native-streaming: 7,057 main / 7,056 companion samples, two offline boundary/Abbey/
camp cycles, zero blocked steps/errors, 36,340 KiB minimum free, guest frame
33/34/34/35 ms p50/p95/p99/max. Region p99/max 4/9 ms; payload plus world animation
p95/p99/max 9/13/21 ms. First cold load precedes received telemetry; later pack
reopen transitions are measured. No NPC/avatar/server/combat or physical input.
Explicit fixture relocation separates the boundary and Abbey routes.

Actual matched fog-off/on screenshots now verify distant blending, near surfaces,
foliage cutouts and readable UI in that view. Abbey/camp/boundary screenshots also
preserved. Authored zone/time/indoor/underwater and all-material acceptance remain.
Fixture build/stream-test-20260916 preserves source ZIP/disc/config; 273 source/63
mounted-output identities verified. Analysis checker added afterward is included
in normal candidate build/candidates/20260916-staged-world-streaming (274 sources,
131 outputs, XBE 1,933,312 bytes/XISO 498,991,104 bytes). No verification.json or
full-world acceptance. Only owned PID 20424 was quit after completed capture; no
active emulator remains. Hardware kit/account/server/characters/credentials intact.

Continue NPC/avatar bounded payload scheduling and complete-pose retention, then
Vanilla authored zone/time fog data. Keep combined live login/customization/HUD/
combat regression pending while hardware account is reserved. Preserve all source/
capture identities. Full parity/release/hardware gates remain unmet; no cutoff.

## September 16 NPC/player streaming checkpoint and latest normal build

Latest scoped native candidate: build/candidates/20260916-actor-streaming. NPC and
avatar payload jobs now share the bounded world loader; complete-pose fallback,
player pending idle/run partner retention and rollback are implemented. Host:
3,644 staged checks, 348 runtime, 539 avatar, real equipped-Human transitions,
two strict auxiliary decoder suites and 13 telemetry tests pass.

Native-actor-streaming-retry: 7,112 main / 7,111 companions, two offline collision
route cycles, eight synthetic NPCs/six templates plus equipped Human, zero
asset/region/traversal failures, all eight complete NPC models after warmup,
zero incomplete player gaps across 964 synthetic clip changes. Free >=32,968 KiB;
guest frame p50/p95/p99/max 33/34/36/55 ms. Actor work p50/p95/p99/max 5/15/20/34 ms;
region max9, world streaming+animation max23. All byte/operation/validation limits
pass. Not live combat, account persistence, input or physical Xbox verification.

First launch failed before telemetry with host 0xc000041d, log ending in graphics
initialization; retained logs/Windows event in fixture startup-failure. Same disc
retry passed. Fixture build/actor-stream-test-20260916 preserves source ZIP/disc/
receipt; all 275 sources and 63 mounted outputs verified before later edits.
Only owned retry PID60792 was quit after duration_limit. No emulator remains.
Screenshots: native-actor-streaming-retry-run/attack/complete.png.

LATEST NORMAL SOURCE/BUILD: build/candidates/20260916-fog-scale-correction. A
post-capture review fixed a duplicate actor-scale multiplier in the fog uniform:
actor_render's camera/axis transform already yields world depth. Built successfully;
1,714 fog checks pass, independent arithmetic in fog-scale-audit.json. Scale-one
captured paths are unaffected; non-unit-scale native visual verification PENDING.
Current normal XBE1,937,408 bytes/XISO498,991,104 bytes. The preceding actor-native
candidate has the same lengths but different hashes; preserve both identities.
No verification.json. Normal disc has no fixture marker. Stable hardware kit intact.

Next substantial batch: authored Vanilla zone/time fog using pinned WoWee/data,
including scaled-actor native verification. Continue profile opening/composition,
animation scheduling and NPC idle/run request coalescing, and the pending combined
live front-end/customization/HUD/combat gate once the reserved account is released.
Maintain original full-game/UI scope, stock64MiB/+8MiB headroom, physical controller
and hardware gates. User authorization continues, no old morning cutoff. Do not
restart servers, log in with the reserved account, reset characters or overwrite
captured source/discs. Full parity remains unfinished.


## September 16 authored fog checkpoint (latest normal build)

Continue from build/candidates/20260916-authored-fog: XBE1,945,600 / XISO499,187,712
bytes; 285source/132normal output hashes verified. Offline fixture:
build/lighting-test-20260916, frozen source ZIP/disc/config/receipt/logs and
capture-identity.json;285sources/60mounted outputs verified before/after capture.
Only owned PID3092 quit after duration_limit+identity checks. No emulator remains.
Stable kit/checkpoints/shared server/account/characters/private assets/credentials
intact. Account remains reserved for hardware testing. No old morning cutoff.

Implemented WXL1 LIGHT.WLF using actual 1.12 DBCs + pinned WoWee helpers:
374volumes/26maps/395profiles;135,040resident bytes,384KiB ceiling. Map defaults,
top-two local smoothstep/residual blending, midnight wrap, signed fog starts,
capped distant fade and temporal smoothing. Five empty profiles are explicitly
reported. No frame-time allocation/I/O. Clear/outdoor is the normal request;
full material/sky lighting and weather/water/interior classification remain.
Near geometry retains fixed lighting at midnight: do not claim full night mode.

Clock: strict8byte LOGIN_SETTIMESPEED, locked snapshot, minutes-per-second scaling,
before/after world-verification handling, malformed rejection, logout reset,
noon fallback. Six encrypted localhost scenarios pass. No real account used.
28,120 host checks include actual data and both allocation failures. Strict84byte
WXL2 companion keeps WXTZ1472bytes. Frozen285sources remain current.

Native-lighting:2626main/2625companion samples,8phases,zero failures,3real wolves at
scales.5/1/2,39,488KiB minimum free in QMP64MiB/native-scale xemu. Guest frame
33/34/34/79ms p50/p95/p99/max; one79ms interval entering scale comparison; fogwork
0/1/1/2ms. Resident world, synthetic time/conditions; no streaming/live gameplay/
pad/hardware claim. First receivedframe156 excludes preload. ActualJPGs noon,
midnight,dawn,rain,scale-off/on; previous non-unit fog-depth correction now has
scoped native visual acceptance. Some screenshots follow the telemetry window on
the unchanged disc. Fixture float labels are blank; fix them next feature batch.

Next: full authored ambient/diffuse/sky and environmental classification; NPC
pending idle/run coalescing, bounded avatar profile opening/composition, animation
scheduling and all-map pipeline. Combined live login/customization/HUD/combat gate
stays pending while account reserved. Full ROADMAP/PARITY scope, stock64MiB/+8MiB
headroom and physical-controller/hardware gates remain. Continue substantial
batches, preserve capture identities, no full-world30fps or completion claim.


## Active material/sky capture (superseded only by its completed entry)

Source/disc frozen in build/material-sky-test-20260916, receipt + source.zip.
288 sources / 60 mounted outputs verified; normal receipt has 132 outputs.
Native capture build/evidence/native-material-sky.csv is active on owned PID74524.
Do not overwrite source, mounted disc, normal artifacts or start another emulator
while this capture is active. Run requests 185 seconds and leaves the emulator
running afterward for screenshots. Preserve source/disc identities before stopping.
XBE1,953,792 / XISO499,515,392 bytes. WXL1 v2 catalog451,040 resident bytes.
74,690 lighting/clock +2,226 UI/sky checks,2 decoder tests and6 encrypted local
clock scenarios pass. Full parity and hardware gates remain unfinished. Shared
account/server reserved; no login attempted. Stable kit and prior candidates intact.


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
sky build/submission/wait 1/2/3/4 ms. QMP confirms 67,108,864 bytes, native scale1,
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


## Weather-state batch capture in progress (September 16 night)

Frozen source/disc: build/weather-state-test-20260916, 294 source /60 output
hashes verified, source.zip saved. Normal XBE1,957,888 /XISO499,515,392 bytes;
132 normal outputs verified. Current native capture is native-weather-state.csv,
owned PID6688, requests200 seconds then leaves the emulator for screenshots.
Preserve its mounted portrait.iso, source and artifacts until capture and receipt
checks finish. Do not start a duplicate emulator or touch shared server/account.

Implemented exact13-byte Vanilla weather parser (including sound/instant flag),
locked snapshots, logout/transfer reset with early-transfer update preservation,
grade-driven clear/overcast lighting, smooth/instant transitions and68-byte WXW1
telemetry. Explicit underwater/indoor overrides exist, automatic classification
and precipitation/cloud/audio remain unfinished.23,163 host checks and12 encrypted
localhost scenarios pass; the default protocol suite now has99 cases. See WEATHER.md.
Overnight automation remains ACTIVE in this thread, every10 minutes, no9AM cutoff.
Latest accepted candidate remains20260916-material-sky until weather capture passes.


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
QMP confirms 67,108,864 bytes, native scale1, disposable disk writes. Frames
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


## Active actor-cadence capture (September 17)

Frozen source/disc: build/actor-cadence-test-20260917, receipt and source.zip.
All 294 source /63 mounted output identities matched before launch. Normal XBE
1,957,888 /XISO499,515,392 bytes. Owned PID13676; capture native-actor-cadence.csv
requests300 seconds, then leaves the emulator for screenshots. Preserve source,
mounted disc and normal artifacts until post-capture identity checks complete.
Do not start another emulator or touch the reserved account/server.

This batch implements NPC idle/run coalescing and optional-clip eviction before
allocation pressure; distance-based idle/run cadence with selected/combat priority;
shared pose-palette reuse and per-frame work telemetry (WXN2,196 bytes).
Host:3,829 streaming +360 runtime +539 avatar checks,3 decoder tests, prepared
equipped Human through idle/run/attack/death and back passed. Native build passed.
Avatar profile opening/composition remain synchronous and are the next batch.
Latest accepted candidate remains20260916-weather-state until this combined
route/animation capture passes; physical controls/hardware and full parity open.


## September 17 actor cadence checkpoint (latest feature acceptance)

Candidate: build/candidates/20260917-actor-cadence. Native XBE 1,957,888 bytes,
normal XISO 499,515,392 bytes. All 294 source /132 normal output identities and
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
scale1 and disposable disk writes. Guest frame p50/p95/p99/max:33/34/42/72 ms.
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


## Active shared-stream-budget capture (September 17)

Frozen source/disc: build/shared-stream-budget-test-20260917 (receipt/source.zip).
295 source /63 mounted outputs match; normal XBE/XISO built. Owned PID48052;
native-shared-stream-budget.csv requests300 seconds, then leaves the emulator
for screenshots. Preserve source, disc and normal outputs through post-capture
receipt checks. Do not duplicate emulators or touch the shared account/server.

Implemented one combined64 KiB read /16 read-op /64 KiB validation /3 payload-
allocation budget across index/world/NPC/avatar queues. Guaranteed shares, forward
borrowing, inactive-queue donations and next-frame request activation preserve
progress. Quota denial defers without failure; complete models/collision guards
remain. WXS2 is212 bytes, old WXS1 remains supported. Avatar profile/atlas and UI
loaders remain outside this budget and are unfinished work. Host5,144 stream,
360 runtime,539 avatar checks and4 decoder tests pass. Native build passes.
Latest accepted feature checkpoint is20260917-actor-cadence until this run passes.


## September 17 shared streaming budget checkpoint (latest feature acceptance)

Candidate: build/candidates/20260917-shared-stream-budget. XBE1,961,984 bytes;
normal XISO499,515,392. All295 sources /132 normal outputs and295 sources /63
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

QMP confirmed67,108,864 bytes, native scale1, disposable disk writes. Minimum free
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
All 296 sources /132 normal outputs and 296 sources /63 mounted fixture outputs
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
The 302-source/132-output normal receipt and 302-source/59-output mounted fixture
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
All 309 sources /132 normal outputs and 309 sources /14 mounted fixture outputs
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
All 316 sources /132 normal outputs and 316 sources /16 mounted fixture outputs
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
All 319 sources /17 mounted outputs and 319 sources /132 normal outputs matched
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


## 2026-09-17 UI diagnostic correction and preserved experiment

The earlier missing-letter report was not corroborated when the original saved
Blackfathom and Blackrock screenshots were inspected again. The complete label
is readable. An independent pixel-presence check finds gold strokes in all 35
non-space glyph boxes in both old images and the triangle experiment screenshot
(minimum 55 gold pixels per glyph box). This checks absence only, not exact glyph
shape or full UI parity; see build/evidence/ui-label-presence.json.

An explicit indexed-triangle submission experiment changed six source paths and
passed host and scoped native checks, but demonstrated no useful visual change.
It was preserved in build/candidates/20260917-ui-triangles-experiment (39-file
identity manifest), then exactly those six source changes were reverted from the
previous frozen source ZIP. No speculative cache/state patch was added.

The experiment used 334 frozen sources, 16 fixture outputs and 132 normal outputs;
all identities matched after its native capture. 5,569 main / 5,568 companion
samples covered one complete 14-case cycle. Minimum free 30,296 KiB; guest frame
p50/p95/p99/max 33/34/34/65 ms. These are xemu measurements, not a controlled
performance improvement or physical hardware proof. Only owned PID 64324 was quit
after capture and ownership verification. Shared account/server and stable kit
were untouched. Earlier candidate documents and manifests remain immutable;
this entry corrects the interpretation of their screenshots.

Work now extends the shared asset pipeline to original terrain MCLQ, including
correct MCNK-relative offsets and deep-water flags. Full Vanilla remains open.


## 2026-09-17 terrain MCLQ checkpoint

Candidate: `build/candidates/20260917-terrain-liquids`. Full Vanilla parity remains
unfinished. No stable hardware promotion or shared-account use occurred.

Original terrain liquids now feed the same WXP v10 geometry/texture-sequence
pipeline used for WMO water. The strict adapter validates MCNK-relative offsets,
all 256 chunk identities, masks and heights; deep-water bit 0x80 remains visible.
The pinned WoWee parser already produces 42/73 water layers in these ADTs; the
native pipeline previously omitted them. Original dry FLT_MAX corners initially
failed strict bounds. All 1,843 such corners are proved unreferenced by enabled
cells and canonicalized to zero; visible heights remain exact and their lighting
normals never use these dry corners. Referenced sentinels still fail. No terrain
floor is added for liquid surfaces. Flow/depth opacity and swimming remain open.

Two original terrain packs total 340,641,826 bytes. 115 MCLQ grids contain 3,835
enabled cells / 7,670 triangles; independent raw-ADT comparison passes for every
visible cell. The previous tile 32/48 retains all 19,164 ordinary entries and 369
WMO environment records exactly. Actual MCVT-to-cooked heights at the four water
probes also match exactly. First failed conversions and raw private diagnostic
ADTs are preserved separately. Host tests: 797 environment and 26,359 liquid
mesh/clock/layout/streaming/allocation-failure checks pass. Native builds pass
with the existing aggregate/indentation and linker warnings.

One native capture: 6,816 main / 6,815 each companion, frames 210..7025, three
complete ten-case cycles, no fixture/asset/region errors. Four original water
locations are sampled above/below plus both sides of the prepared boundary.
These are synthetic camera relocations, not walked routes or swimming. All 30
texture frames were submitted; up to 13 liquid patches. Both neighboring sources
are resident at the boundary. Metadata maximum 849 records / 197,332 bytes.

Minimum free 30,736 KiB (30.02 MiB), in verified 67,108,864-byte xemu RAM at native
rendering scale with disposable disk writes. Guest frame p50/p95/p99/max:
33/34/34/69 ms. Maximum index/payload/sample work 22/22/1 ms. Maximum scene bytes
9,386,560, current index 2,549,824, pending index 1,318,528. Shared quota maxima
64 KiB reads, nine operations, 64 KiB scans and three allocation batches.
The worst 69 ms interval follows boundary frame 1779 (32 ms combined streaming,
33 ms drawing, 68 ms work); later laps repeat 49/43 ms boundary intervals. Shared
streaming/draw scheduling needs further optimization. No full-world sustained
30 fps, controlled speedup or physical hardware performance is claimed.

The initial inherited checker incorrectly required exactly one loaded region
and all three environment classes. This terrain fixture correctly reports two
resident sources at the boundary and requests outdoor/underwater cases only.
A separately preserved checker corrects those two assumptions and passes the
same capture. Failed/corrected reports and checker hashes are preserved; no
repeat emulator run was required for this analysis correction.

Actual screenshots show the original lake surface and a below-water view. The
river-probe screenshot faces a bank and supplies no clear surface-appearance
acceptance. Source water heights exceed local terrain at all four probe points;
there is no measured terrain-height discrepancy at those points. The fixture
contains only two tiles and surrounding absent content remains visible. Exact
water opacity/flow, shore appearance, cave/ground containment and all-map coverage
remain unfinished. No controller input was injected or physically tested.
Above-surface screenshots are post-capture paused views with separate frame
identities. An initial transition pause displayed the preceding case; subsequent
pauses waited 30 frames into the requested case. The first pause record is retained.

Only owned xemu PID 52064 was quit after exact executable/disc/QMP checks.
The frozen 332-source / 14-fixture-output and 332-source / 132-normal-output
receipts match after capture. Source ZIP and private dependency hashes match.
Normal XBE: 1,994,752 bytes, SHA256 `26b72603b6400cd31dbc155afa0282fb588c3841f7e3adaf1e5c64f5e46a7e40`.
Normal XISO: 499,515,392 bytes, SHA256 `8b33bc0e66306f56e8e5a9721dd4295cc7a2073ecc466e8470fbc259eb32aa55`.
Normal installed terrain/actor assets, the stable hardware kit, prior candidates,
saved characters, credentials and server are unchanged.

Next: apply the two verified checker corrections to the shared tool after freezing
this checkpoint; improve boundary streaming/draw scheduling and terrain-liquid
cave/ground containment, then expand the same pipeline across maps and continue
original UI/all-class gameplay. Full PC parity, physical controller/hardware and
release gates remain open. Autonomous work continues without a morning cutoff.


Post-checkpoint analysis-only correction: the 94-file terrain-liquid manifest
was verified before changing source. The two validated checker corrections are
now applied to tools/check-global-world.py. Its executable text exactly matches
the independently run reanalysis script. This is the only source difference from
the frozen native capture; Xbox source, executable and mounted disc are unchanged.
The normal build receipt is refreshed for the current analysis-tool identity;
the candidate retains its original capture source ZIP/receipt unchanged. See
build/evidence/post-terrain-checker-source-correction.json. No emulator remains.

## 2026-09-17 latest hardware package and authorized password change

User requested a build with all latest updates for the real Xbox and a new local
game password. Completed `dist/WOWX-hardware-20260917`, preserving every file in
the September 16 kit against its original manifest. New package uses the exact
terrain-liquid capture XBE, SHA256
`26b72603b6400cd31dbc155afa0282fb588c3841f7e3adaf1e5c64f5e46a7e40`.
Base game assets match its frozen normal receipt; the two newer v10 terrain
packs and twenty global WMO packs are explicit separately recorded overlays.
The merged WXI retains both other original terrain entries: 24 total regions.
Manual login, empty packaged password, fresh entropy, zero input replay, and
disabled scenarios are verified. No offline fixture markers are present.

All 24 packs pass the production host loader. Xboxdawn's saved position resolves
floor 81.938919 with zero failures. The host tool's simulated memory value is
not physical hardware telemetry. No new emulator or gameplay acceptance run was
performed for packaging; previous scoped xemu metrics retain their original
limits. Full Vanilla parity and combined native/hardware release gates remain open.

HDD directory: 153 files / 1,060,926,510 bytes, largest file 170,923,304 bytes.
ZIP: `dist/WOWX-hardware-20260917.zip`, 439,887,638 bytes, SHA256
`82d91ce29acc1c3596d0f16b21fbdce9173a517aa611b00f3303b35630195094`.
XISO: `dist/WOWX-hardware-20260917/WOWX-20260917.iso`, 1,061,683,200 bytes,
SHA256 `b6ec8bab1c7452916232d3d2f2495b270dc80da9b3dcbc47776d1625984a32f6`.
ZIP CRC and every archived game-file SHA256 match; XISO creation reports all
153 files and Xbox volume markers pass. Manifest, conversion reports, capture
receipts, loader logs, validation and updated hardware instructions accompany it.
See build/evidence/hardware-20260917-delivery.json and package verification.json.

WSL was initially stopped. MariaDB reported recovery checks during startup;
explicit CHECK TABLE returned OK for characters, inventory, account and realm.
The project launcher started one owned realmd and one owned mangosd. Existing
LAN forwarding and firewall were reused without changes: PC192.168.50.156,
Xbox192.168.50.85, auth3725/world8086; realm advertises the PC LAN address.
Authentication-only probe with the new password passed and saw one available
realm; no world connection, character movement or scenario ran. Account remains
WOWXTEST with five characters; full character rows match before/after reset.
The password reset follows pinned vMaNGOS AccountMgr/SRP6 and changes only s/v.
Private credentials and a complete character database backup were preserved in
server/private-backups; the credential store was updated, temporary auth probe
removed, and no passwords/backups were included in the deliverables.

Source changes this handoff are packaging/password-maintenance scripts and
documentation only. Host auth/pack/region tools were rebuilt against current
source. Native runtime sources, existing XBE/disc and prior captures are unchanged.
The packaging script now supports explicit scoped-native evidence and validated
world overlays, preserving unmodified indexed regions instead of replacing the
whole index with a two-tile fixture.

Hardware reservation continues for BOTH packages, shared account, saved progress
and the current LAN realm endpoint. Do not use the shared account or update the
Xbox over FTP during independent overnight work. When explicitly released,
regenerate private xemu authentication data from the updated credential store;
older frozen discs still contain their original private test config. Autonomous
source/offline development remains authorized with no morning cutoff.

## 2026-09-17 cooperative streaming clock candidate (capture in progress)

Independent work while the user tests hardware: per-queue 2/6/2/2 ms cooperative
slices supplement the existing byte/read/scan/allocation quotas. Native clock
registered through a callback; expiry defers further work and preserves later
queues' independent opportunity to progress. Pending publication remains atomic.
Region begin now explicitly distinguishes time deferral from file failure, and
metadata footer reads check the actual grant after the room check. WXS3 appends
clock limits/observations/yield bits and keeps legacy decoding. Neither blocking
I/O nor whole selection/copy/draw work can be preempted; no hard frame bound.

Host streaming5,317/environment806 checks and eight telemetry test groups pass;
the liquid suite also passes. Initial synthetic workload did not reach its time
slice before byte quotas (27frames/zero yields in the diagnostic), so the clock
test was made explicitly slow:34frames/33yield frames, exact buffers, no leaks.
All failed logs remain. Native XBE/XISO build passes with existing warnings.

One owned xemu PID63332 launched on the new isolated
build/stream-deadline-test-20260917/portrait.iso. Source ZIP and334-source fixture
receipt are frozen. The same ten original terrain-liquid camera cases and pack
bytes are used; no server account, input injection or physical pad. QMP was not
ready during the first startup query; subsequent read confirms67,108,864 bytes
RAM, correct disc, disposable HDD/CD overlays. Capture prefix:
build/evidence/native-stream-deadline. Preserve this capture until completed and
checked; do not edit its source/disc or start a second emulator. Hardware kits,
LAN realm, shared account, credentials and character state remain untouched.


## 2026-09-17 cooperative streaming clock checkpoint

Candidate: build/candidates/20260917-stream-clock. The capture has completed;
only owned xemu PID63332 was quit after exact executable/disc/QMP checks. No
emulator remains. Both delivered hardware packages, LAN server/account,
credentials and character state were untouched by this independent batch.

5,394 main / 5,393 complete companions, frames210..5603, two complete ten-case
cycles, zero asset/region/fixture errors. World work exercised1,432 timed yields.
All30 original texture frames were submitted. Measured free minimum30,736 KiB
(30.02 MiB) in67,108,864-byte xemu at native scale with disposable disk overlays.
No controller input, account connection or physical hardware was involved.
Native NPC/player clock workloads remain unverified; host coverage is separate.

Guest frame p50/p95/p99/max33/34/34/81 ms; two intervals exceeded40 ms. This does
not establish a performance improvement over the previous69 ms maximum. The
largest remaining stall is now localized to index publication and work outside
the read loop. At frame1903, the final39 records are validated with zero index
reads, the joined table grows to2,549,824 bytes, and region work takes19 ms.
World selection takes7 ms with zero payload reads, drawing52 ms, total80 ms,
followed by the81 ms interval. The next lap's frame4107 records18/7/20 ms for
region/selection/draw and a50 ms next interval. World read loading was inactive
on both; adding read gates cannot preempt table reallocation/copy or drawing.
Other world operations reach22 ms before a later check can yield.

Next implementation: stage joined-index allocation/copy with atomic publication,
correct cancellation/restarts when attached sources change, and accounting for
both old/new tables plus pending indices. Keep at least8 MiB measured free.
Then stage spatial selection and investigate draw/GPU-wait time; test the combined
NPC/player consumers. Keep the frame-performance gate open. Do not silently
lower content coverage, alter geometry, or replace either hardware test package.

Host checks:5,317 streaming /806 environment; liquid suite and eight telemetry
test groups pass. Native build retains the existing warnings. Source ZIP and
both receipts verify334 sources,14 fixture outputs and132 normal outputs.
No appearance screenshots were added: this batch validates counters/timing,
not new visual parity, and has unchanged geometry/material code and pack bytes.
All prior images/captures remain preserved with their original identities.

XBE:1,994,752 bytes, SHA256 `4bfa46cd7e7f8e2e94cfb41b375b8f2a5a63bc698b509b89335dfd49f5968367`.
Normal XISO:499,515,392 bytes, SHA256 `b64e9abfe4ca023db14e12591d5d553183e1da87aebfa11939aa00100a5fc94f`.
The normal asset set remains the previous installed pack set; the isolated disc
references the two v10 terrain packs separately in its receipt. Source/disc,
capture, checks, lifecycle/ownership and documentation are frozen in the candidate.
Full Vanilla gameplay/UI, combined live acceptance and physical release gates
remain open. Shared-account hardware reservation continues; autonomous work has
no morning cutoff.


## 2026-09-17 staged index publication checkpoint

Candidate: build/candidates/20260917-index-publication. The newest user request
is a hardware build with latest updates and the requested local login password.
The pending index batch was completed first: bounded copy stages, atomic
publication, source mutation/cancellation recovery and full pending-table memory
accounting. First-source tables transfer directly. Host consumers pass; the
runtime boundary test now waits for staged second-source publication while
asserting continued first-source availability. The original failed log remains.

One credential-free xemu run completed three ten-case terrain-water cycles:
6,149 main / 6,148 exact companions, frames 241–6389, zero asset/region/fixture
errors, all 30 original liquid texture frames. Measured minimum free 29,968 KiB
(29.27 MiB) in QMP-confirmed 67,108,864-byte RAM at native scale. Guest frame
p50/p95/p99/max 33/34/34/56 ms. Thirty index-copy frames stay within 256 KiB and
2 ms; three 2,549,824-byte publications take 0/1/0 ms versus 18–19 ms previously.
The remaining first-publication stall has 8 ms selection and 46 ms drawing.
Native world queues exercise 456 time yields; native actor/player workloads are
outside this run. No screenshots, physical controller or gameplay claim added.

Frozen source ZIP and receipts match 336 sources, 14 fixture outputs and 132
normal outputs. XBE is 1,994,752 bytes, SHA256
2d7b84b03071fad2c02ca3eab7d444fadab3e56683f093189e9a6eee1379775d.
Only project xemu PID 63464 was quit after exact process, QMP-owner, mounted-disc
and memory checks. No emulator remains; the shared server was reused, not
restarted. Native source/disc identities are preserved. See INDEX-PUBLICATION.md.

The password was already changed in the earlier authorized handoff. A read-only
recalculation confirms both the private store and server SRP verifier match the
latest requested password. WOWXTEST remains with five characters; no auth or
world login, character edit or credential reset was needed this turn. Existing
owned realmd 804 / mangosd 851 and LAN realm 192.168.50.156:8086 are ready.
Previous hardware kits stay unchanged. Revision 2 is being packaged separately;
its final delivery/checksum record will follow after archive validation.

Next development: stage spatial selection and investigate draw/GPU waits, then
native actor/player integration. Keep the full Vanilla, original UI, stock-hardware
and physical controller release gates open. All hardware kits, LAN realm and
shared account stay reserved for the user's test. No FTP replacement or shared
account login during autonomous development. No morning cutoff applies.

### Revision 2 hardware delivery complete

`dist/WOWX-hardware-20260917-r2/WOWX`: 153 files / 1,060,926,510 bytes,
24 indexed world regions. All packs pass the current production host loader;
Xboxdawn's Northshire floor resolves without error. Manual input/menu, empty
packaged password, fresh entropy, no offline fixtures or replay are verified.
Both earlier kits match all their original game-file hashes (108 and153 files).

ZIP: `dist/WOWX-hardware-20260917-r2.zip`, 439,884,400 bytes,
SHA256 `90cc487101ebbb3b263d51102c6a305009fd2f33f3ee7c5281f1aacb9d01239c`.
ZIP CRC and every archived game-file SHA256 pass.
XISO: `dist/WOWX-hardware-20260917-r2/WOWX-20260917-r2.iso`,
1,061,683,200 bytes, SHA256
`08b11c0a04cf2e2acbd349869781e8ef0b9100d2854112fc9f866958c5b7a0fb`.
XISO volume markers and all 153 creation entries are present.
The 42-file index-publication candidate manifest was verified and stays frozen.
All three hardware kits stay protected. Updated instructions are
docs/HARDWARE-20260917-R2.md and packaged HARDWARE-TEST.md. No FTP operation,
shared-account auth/world login, character edit, repeat password reset or server
restart occurred. The password verifier and LAN readiness checks pass.
Full Vanilla, combined native gameplay, physical controller and actual stock
hardware release gates remain open. See build/evidence/hardware-r2-delivery.json.


## 2026-09-17 staged spatial selection (capture in progress)

Independent implementation while the user tests revision 2: world selection
now scores at most 2,048 entries per frame in groups of32, observing the world
queue clock. A fixed 6,692-byte per-scene workspace keeps candidates private
until a complete scan. Existing valid residents/payload jobs remain usable;
index mutation invalidates stale entry lists. Normal movement finishes snapshots
instead of starving the scan; relocations of32 units restart. Selection latency
and draw/GPU waits remain performance concerns, not silently reduced coverage.

Host sorted-oracle, quota, mutation, movement and clock tests pass, as do runtime,
index, environment, liquids, avatar, preview and streaming consumers. An old
inactive-lane test expected immediate payload preparation; it now correctly
checks deferred selection with no work. Its failed log remains. Ten telemetry
test groups pass. WXS5 preserves previous formats and adds selection telemetry.
Native build passes with existing warnings. Source ZIP/receipt freeze339 sources.

Capture: build/evidence/native-selection; fixture: build/selection-test-20260917.
Only owned xemu PID70524 is active, same ten original terrain-water camera cases
and pack bytes, 64 MiB, native rendering scale, disposable disk overlays. QMP
confirms67,108,864 bytes RAM and exact mounted fixture. Preserve source/disc and
this process until capture finishes and is checked. No account or input activity,
no server changes and no changes to any of the three delivered hardware kits.


## 2026-09-17 staged spatial selection checkpoint

Candidate: build/candidates/20260917-selection. Capture completed and all scoped
native checks pass. Only owned xemu PID70524 was quit after exact process,
QMP-owner, disc and RAM checks; no emulator remains. All three hardware packages,
shared account, saved characters, credentials and LAN server were untouched.

Three offline ten-case terrain-water cycles: 6,898 main /6,897 exact companions,
frames240–7137, zero asset/region/fixture failures, all30 original liquid texture
frames submitted. 425 selection frames stay within2,048 entries;35 complete
multi-frame selections are identified. Non-payload selection-frame stream-time
p50/p95/p99/max0/3/3/4 ms, including stream housekeeping. With payload work the
selection-frame stream maximum is12 ms. World queues yield on their clock603 times.

Minimum measured free29,936 KiB (29.23 MiB) in67,108,864-byte xemu at native scale
with disposable disk overlays. Guest frames33/34/34/48 ms; one interval over40 ms.
At first publication (frame1858), world-stream work is0 ms, drawing43 ms, total47,
followed by48 ms. Subsequent publication drawing is23/20 ms. The whole-selection
pause is distributed, but draw/GPU wait and possible emulator first-use costs
remain. No full-world/physical performance pass or new visual/controller/gameplay
coverage is claimed. No screenshots were added. See SELECTION.md.

Host sorted-oracle, atomic selection, mutation, movement, quota and clock tests
pass, as do runtime/index/environment/liquid/avatar/preview/streaming consumers.
Ten telemetry test groups pass. The earlier inactive-lane assertion failure is
preserved; its expected state now correctly includes deferred selection before
payload preparation. Global-world, selection, shared-clock and index-publication
native checks pass. Source ZIP/receipts match339 sources,14 fixture outputs and
132 normal outputs. Native build retains existing compiler/linker warnings.

XBE:1,998,848 bytes, SHA256
3c51efc3f52e7e369ebf1b7f98407fac844b832b980f4bdb58a2ac51061c9cce.
Normal XISO:499,515,392 bytes, SHA256
ab58658e9ddcd21a40704e2723b09d0f50f683bb457e700e2548b5b4259eb65d.
The normal asset set is unchanged; the isolated fixture references the same two
v10 terrain packs separately. The user's revision2 hardware kit stays frozen.

Next: split CPU submission from draw/GPU-wait costs, distinguish emulator
first-use effects, and measure moving/combined actor/player consumers. Continue
full Vanilla gameplay/UI implementation without reducing scope. Shared-account
hardware reservation and no-morning-cutoff authorization continue.


## 2026-09-17 geometry submission batch in native acceptance

Current source builds larger 1,536-index triangle packets (previously240), copying
existing packed 16-bit indices without new asset allocations. WXD1 companion
separates four pass submission wall times, explicit pushbuffer wrap wait/reset,
and final GPU drain. Submission includes pb_end cache flush/MMIO and emulator
scheduling, so its residual is not a pure CPU measurement. Decoder/layout and
independent topology/odd-tail/bounds tests pass; staged selection/stream tests pass.
Makefile now explicitly tracks the new renderer headers and pack_selection.h.
Native build retains existing warnings. Frozen source archive has345 files.

Fixture: build/draw-batch-test-20260917 (63 receipt outputs); normal132 outputs
also verify. All61 non-executable fixture files exactly match the earlier staged
appearance workload, including four original terrain packs. These are not the
two newer terrain-water v10 overlays used in native-selection.

First launch PID49036 exited before any telemetry: host0xc000041d (event also
records c0000005), offset509315 during graphics initialization. Evidence kept in
fixture startup-failure and native-draw-batch lifecycle/logs. The identical-disc
retry is running ONLY project xemu PID47180, capture native-draw-batch-retry.
QMP verifies64MiB and exact fixture, native surface scale, disposable overlay.
Preserve its source/disc until completed and validated. Captured actual xemu
phase3 boundary image through Computer Use; no input was sent. No account, saved
character, LAN server, credentials, hardware kits or console files changed.


## 2026-09-17 geometry submission / combined traversal checkpoint

Candidate: build/candidates/20260917-draw-submission. All five scoped native
checks pass: combined streaming/appearance, draw accounting, selection, clock,
and staged index publication. Capture native-draw-batch-retry has8,267 main and
8,266 exact stream/actor/draw companions, frames225–8491, two completed cycles.
Minimum free29,588KiB (28.89MiB), guest frames33/34/34/36ms with no interval>40ms.
No model/asset/traversal failures; eight complete NPCs and no avatar gaps;32
appearance commits,64 cancellations, three atlas hashes.346 complete multi-frame
selections stay within2,048 entries/frame.33 index-copy frames stay within256KiB,
maximum3ms; three publications1/0/0ms. Shared quotas and cooperative yields pass.

1,536-index batches produce1,036,801 submissions versus2,419,087 under the old
240-index rule for exact current geometry,57.14% fewer. Submission timings
3/5/5/10ms; final drain1/3/5/17ms. Worst draw20ms is3ms submission+17ms drain
at frame301, followed by33ms. No buffer wraps occurred, so wrap counters are not
natively exercised. Submission wall time is not pure CPU time; first-use cause
and physical GPU time are unproven. Four original terrain packs match the old
staged-appearance fixture; do not compare this directly with the two v10 water
packs or claim the earlier43ms drawing spike fixed. Boot/preload before225 excluded.

Four actual xemu images saved via Computer Use (no input) show geometry/equipment
and fog, but also partial residency/close-camera clipping at explicit relocation,
and low-pose terrain intersections. Full standing body is intact. Animation.c
loops every clip against global time; non-looping, per-instance playback and
ground contact need original-data review. These numerical tests are not visual
PC parity. Next batch should address these remaining animation/loading workflows
and continue original gameplay/UI work; reserve terrain-water spike profiling.

345 sources,63 fixture and132 normal outputs match after testing. SourceZIP and
receipts preserved. Only owned retry PID47180 quit after exact process/command
line, QMP-owner/disc/RAM checks. Post-quit GLib g_source_destroy assertion logged;
subsequent process inventory empty. Initial startup-failure49036 and Windows
events remain preserved; the identical disc succeeded on retry. No emulator
remains. All three hardware kits, saved characters, shared account/password, LAN
realm/server and private assets remain unchanged. No console transfer/new kit.

XBE1,998,848 bytes SHA256
2181f9da75bce655fe39c56533631f020264260cf21c6473ecfe7e821f756977.
Normal XISO499,515,392 bytes SHA256
8e251302c12eab7b486e4104cc152361ed92c694ad83d23a0ba6a6eae223a749.
Fixture XISO372,047,872 bytes SHA256
97404e8f11a07a42a13d987580025f992e759129fc60fd4a47ad69664b3b3f59.
See DRAW-SUBMISSION.md for implementation, evidence, reproduction and limits.
