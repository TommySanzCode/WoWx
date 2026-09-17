# Local development playtest

This is an incomplete third-person Xbox development client. It uses the private
local vMaNGOS server and the disposable `Xboxer`, `Xboxnight` and `Xboxdawn` Human warriors.
Character selection and creation work through the controller. Appearance customization,
complete player appearance coverage, audio and full Vanilla coverage remain.

## Start the current build

With the dedicated test xemu instance closed:

```powershell
# Run from the repository root.
.\scripts\start-server.ps1
.\scripts\run-xemu.ps1
```

The launcher builds `build/xbox/default.xbe` and `build/wowx.iso`, disables test
replays and starts the 64 MiB emulator at the account menu. The local account
name and server address are filled from the private development configuration;
the password starts empty. A opens a field, D-pad moves through keyboard keys,
X erases, Y changes keyboard page, Start finishes editing, and B cancels an edit.
At the account form, Start logs in. Choose the realm with D-pad/A, then select
your character. Its saved position and progress are preserved.
Normal play starts with diagnostics hidden; click the right stick
to show timing, memory and connection details. Select an appropriate controller in xemu's input settings; the
default launch configuration uses xemu's keyboard controller. Physical controller
and original Xbox validation are still pending.

`-AutoLogin` explicitly retains the development shortcut to Xboxer for neutral
world captures. Existing gameplay replay switches also use that shortcut.
`-LoginReplay` selects a separate private test: account keyboard, one rejected
password, successful authentication, realm cancellation/retry, roster and saved
world entry. It injects controller samples and preloads disposable credentials;
it does not test physical controller transport. The normal menu does not preload
the password. `LOGIN.BIN` records MENU/NONE/WXLT and is included in build receipts.
The new front end is under native validation; the original animated title scene,
full PC layout and remembered-account preferences remain unfinished.

The server launcher starts the existing workspace's vMaNGOS processes in WSL
Ubuntu, reuses them on subsequent calls and checks the loopback listeners. It
does not reset character progress or rewrite credentials/configuration. If the
client was already at a network error, use its connection menu to reconnect or
restart the dedicated emulator after starting the server.

## Xbox controls currently implemented

| Control | Behavior |
|---|---|
| Left stick / right stick | Move / turn camera |
| A | Jump; confirm dialogs; release spirit while dead |
| B | Stop attack and clear target; cancel dialogs |
| Y or D-pad left/right | Cycle nearby targets |
| X | Interact with the selected target, loot, or start melee |
| Left trigger + A | Default warrior attack binding; follows remapping |
| Left trigger + B | Default Heroic Strike binding; follows remapping |
| Hold LT, RT or both | Show the eight current assignments in that layer |
| Trigger layers + face/D-pad buttons | 24 configurable server action slots |
| Tap Black | Toggle bags |
| Hold Black | Utility wheel |
| Wheel: left stick or D-pad | Choose Bags, Spellbook, Quests or Settings |
| Wheel: release Black or A; B | Open selected menu; cancel |
| Bags: D-pad, A, X, Y, B | Select, equip, use, switch equipment view, close |
| Equipment: X | Move selected equipment into bags |
| Merchant: D-pad, A, Y, B | Select, review/confirm trade, switch buy/sell, close |
| Start | Controls/settings menu |
| Menu: Back | Log out or reconnect |
| Menu: Y | Log out and open character selection |
| Menu: White | Open the learned spellbook |
| Spellbook: D-pad, A, B | Browse, choose a spell to assign, close |
| Spell assignment: triggers + button, A | Choose a controller binding, review, then confirm |
| Spell assignment: X, A | Review clearing a binding, then confirm |
| Characters: D-pad, A, X, Y, B | Select, enter, create, refresh, close |
| Menu: D-pad up/down, X | Adjust dead zone, save settings |
| Right stick click | Toggle development diagnostics |

While a ghost, the HUD shows body distance and the remaining server delay. Back
opens the map with a red body marker. A reclaims an owned corpse within 39 yards
after the timer expires; the server makes the final decision. A fresh corpse
query is sent after spirit release and after reconnecting as a ghost.
X on a nearby Spirit Healer opens a resurrection prompt; A confirms the
displayed durability cost. Spirit Healer and corpse-run coverage are recorded
separately in [STATUS.md](STATUS.md).

Merchants show bundle quantities and buying prices. Selling currently removes one
item per confirmation. Repairs, buyback, quantity selection and sell-price previews
remain to be implemented.

Northshire and two adjacent terrain cells are staged, plus one separately tested
Orc starting-area conversion. This does not provide unrestricted continent travel.
An unavailable terrain pack stops walking and displays a loading/asset message.
Completed quest rewards remain saved. `Xboxnight`, created through the native
character screen, is now level two with quests 783 and 7 rewarded, 324 XP and
54 copper. `Xboxer` remains level two at the inn with 768 XP and ten copper.
Xboxdawn is also level two, with313 XP and49 copper after the camp reward.
All characters' earned quests and progress are retained between runs.

## Spellbook

The spellbook reads the current character's learned abilities and names/ranks
from a compact catalog prepared from the supplied Vanilla `Spell.dbc`. Passive
abilities are shown after active ones and cannot be assigned. Internal hidden
spells and profession child spells are excluded; recipe sublists remain pending.
Assignment follows the controller's
saved mapping and the current supported stance bar. Review shows the existing
action before replacement; B cancels. Bindings belong to the realm character and
are read back on reconnect. This UI does not yet provide spell descriptions,
icons, cooldown displays, talents or full class/form targeting support.

`-SpellbookReplay` assigns Heroic Strike to an empty controller slot, reconnects,
clears the slot and reconnects again. It preserves progress and verifies the
original binding. Check its capture with `tools/check-spellbook.py`.
`-SpellbookScreensReplay` only opens the list for inspection and closes it; it
does not edit bindings. Both use injected inputs, not a physical controller.

## Utility wheel

Hold Black for 350 ms to open the wheel. Choose a direction with the left stick
or D-pad, then release Black or press A. Release without choosing to cancel;
B also cancels. Start opens Settings. A short tap on Black retains quick bag
access. Input is captured from the initial press through the closing frame, so
moving either stick or pressing trigger actions while choosing a menu does not
move, turn, jump or cast. Disconnecting or entering an unavailable state cancels
the wheel. The menu contains only the four currently implemented destinations.

`-UtilityReplay` tests a quick bag tap, all four destinations, modifier-plus-button
confirmation, held-button suppression and cancellation. Record 140 seconds and
run `tools/check-utility.py <capture>.csv` plus `tools/check-avatar.py <capture>.csv`.
It leaves both position/progress and server bindings unchanged. Native input is
injected; physical controller operation still needs verification.

## Trigger action prompts

Hold LT, RT or both to show that layer's eight button assignments. Names follow
the character's server action bar, saved controller remapping and supported
warrior stances. The panel updates after assignment changes. Health and target
details move above it; menus, inventory, spellbook, dialogs and death controls
hide it. Empty/loading/unsupported actions have explicit labels. Names longer
than the cell are shortened; the spellbook provides the full name. Cooldowns,
resource availability and icons are not yet displayed.

`-ActionbarReplay` holds all three trigger layers, then checks menu/inventory
suppression without pressing any action button. Record 135 seconds and run
`tools/check-actionbar.py <capture>.csv --bindings <server-bar>.json`, where the
JSON is the 120 packed bindings captured read-only from the local server before
the run. `-SpellbookReplay` now also holds the edited layer after assigning and
clearing a spell; check it with `tools/check-spellbook.py <capture>.csv --actionbar`.
These are injected input tests and do not verify a physical controller.

## Character creation

Press Start then Y. In the roster, X opens creation. Use the D-pad to choose a
name, race, a valid class and sex; A on the name opens the 2-12-letter keyboard.
A selects a key, X erases, Start finishes and B cancels the edit. Select Create
and confirm with A. A rejected name leaves the draft available for correction.
Select the new character in the roster and press A to enter.

This development build still auto-enters `Xboxer` at boot. Reconnect remembers
the selected character within the running client. Credentials come from the
private local test setup; an account-login screen remains to be implemented.
All Vanilla race/class choices are offered, but the staged terrain and native
avatar currently cover only a small test set. Appearance fields use defaults;
customization, preview, complete native avatar coverage and deletion are pending.

`scripts/run-xemu.ps1 -AvatarSwitchReplay` runs a separate character fixture:
it creates or selects Xboxdawn, a default female Human warrior, enters the world,
then returns to the original character without moving either. It uses the same
controller picker, name keyboard, gender row and creation confirmation as normal
play. Do not reset an existing character to satisfy fresh-character assertions.
Record with `tools/record_telemetry.py --seconds 400 --finish-on-scenario --output
build/evidence/<capture>.csv`, then run `tools/check-character.py <capture>.csv
--avatar-switch` and `tools/check-avatar.py <capture>.csv`. The checker requires
male/female/male draw coverage, exactly three profile loads and preserved original
progress. Physical pad operation remains a separate check.

Prepare profiles with `scripts/prepare-avatars.py` as shown in the README. The
fifteen-character stem and extension fit Xbox filenames. Each exact seven-field
look selects its own pair; only one pair is resident, with a 4 MiB geometry cache.
Missing profiles are reported and retried only after a look or connection change.

`-CharacterReplay` creates `Xboxnight` through controller inputs if absent, enters
it, then returns to the original character and checks saved progress. Later runs
reuse that character without deleting it. Record with `--seconds 300
--finish-on-scenario`, then run `python tools/check-character.py <capture.csv>`.
WXTC telemetry includes lobby/UI state, creation result and player identity.
WXTD adds adapter connection reporting. `-CharacterScreensReplay` is a paced
visual-inspection trace: roster, empty creation form and keyboard, cancellation,
then return to the original character. It does not create or delete a character.
The recorder ends before a guest clock restart so captures cannot combine boots.
This replay expects the new character to remain level one with no XP or money;
after gameplay, use a separate fresh scenario instead of resetting earned state.

## Repeatable evidence

Replay options are development fixtures, not normal play. They inject controller
samples through the normal input path and do not verify a physical gamepad.
Use unique capture names; retain failures along with passing evidence.

```powershell
python tools/record_telemetry.py --seconds 650 --finish-on-scenario --output build/evidence/death-run.csv
python tools/check-death.py build/evidence/death-run.csv
```

The death replay requires the dedicated character near the Kobold Vermin fixture
and is launched with `-DeathReplay`. It provokes a creature, takes real damage,
releases, visits a Spirit Healer, resurrects, logs out and reconnects. Position
fixture changes are limited to the offline disposable account; XP, quests and
inventory are retained. Restore a normal launch after any replay.

`-AppearanceReplay` runs a state-driven equipment test at the saved position.
It requires an equipped offhand item and a free backpack slot, moves that item
into the bag through the controller UI, re-equips it, logs out and reconnects.
Its state checker checks live item/model IDs; the separate avatar checker checks drawing. Record
with `--seconds 190 --finish-on-scenario`, then run
`python tools/check-appearance.py <capture.csv>`. Version 10 captures (magic WXTA)
include the scenario ID and all nineteen worn item entries, display IDs and
inventory types. The checker requires the equipment scenario ID.

## Avatar and camera validation

Prepare `AVATAR.WXP`/`AVATAR.WXA` as described in the README before native builds.
`wowx_avatar_tests` exercises metadata spans, composition, equipment transitions,
unsupported appearances and allocation rollback. `tests/player_assets.py --modular`
prepares eighteen actual profiles and rejects eight malformed inputs.

`scripts/run-xemu.ps1 -AppearanceReplay` now exercises the native avatar as well
as server state. Record using `--finish-on-scenario`; run both
`tools/check-appearance.py capture.csv` and
`tools/check-avatar.py capture.csv --gear-replay`. The first checks protocol and
saved progress; the second checks animated geometry and equipped-slot submissions.
The inventory panel covers the world while open, so draw telemetry and the final
unobstructed screenshot provide different evidence.

`-CameraReplay` / WXC8 turns through four headings, looks down/up and restores
the original heading/pitch with ordinary controller samples. It does not move
the player or directly change position. Inspect the camera's collision response,
frame timings and actual screenshots. It is separate from physical pad testing.
Run `python tools/check-camera.py capture.csv` for scenario, collision-distance,
view-restoration, position-preservation and memory checks.


## Quest routes and journal

Start -> A opens the quest journal. D-pad chooses an active quest; A reads it.
Left/right changes text pages; X switches objectives and description; B returns
through the list to the world. It uses real server quest queries, current player
quest counters and bag item counts. Quest abandonment, reputation details and
precise timed-quest countdown are not implemented yet.

`-StarterQuestReplay` / WXCA uses Xboxnight for the starter delivery quest and
acceptance of Kobold Camp Cleanup, ordinary walking, and saved reconnect. Its
fresh-route checker is `tools/check-starter-quest.py`. That character has now
advanced; never reset earned progress to repeat a fresh-state assertion.
`-CampQuestReplay` / WXCB continues quest 7, inspects the journal, walks to the
camp, fights/loots, returns for the reward and reconnects. The first native run
failed after eight kills on a blocked approach; a continuation finished the kills
but hit a tree returning. A separate return/turn-in/reconnect run passes. These
interrupted runs are not one continuous full quest acceptance test.
These are controller test drivers, not game pathfinding or physical pad tests.

Host development route tools use the same pack streamer and walking collision:
`wowx_walkpath world.wxi map start_x start_y start_z goal_x goal_y goal_z margin`
and `wowx_regioncheck --route world.wxi map route.txt` (one XYZ waypoint per line).
A host path is still subject to independent validation and native replay. The
engine remains map-independent; Northshire coordinates belong to test fixtures.


For a captured failed return position, an independently checked host route can
be supplied with `-CampQuestReplay -CampReturn path/to/route.txt`. It writes the
bounded development-only `CAMP.RTE`; the driver refuses to apply it more than one
yard from its first waypoint. A normal launch resets it to NONE. Use
`tools/check-camp-quest.py capture.csv --return-only` only when starting at10/10;
that check explicitly excludes combat during the return run. Replays require
appropriate existing quest state; Xboxnight has now rewarded both starter quests,
so do not reset it or expect old fresh/active-quest fixtures to pass unchanged.


## Continuous fresh quest test

`-QuestLoopReplay` selects the female Human named by `-TestCharacter`, accepts and
rewards A Threat Within (783), accepts Kobold Camp Cleanup (7), reads its journal,
walks to the camp, fights and loots ten kobolds, walks back for the reward, logs
out and reconnects. This development scenario injects ordinary controller input.
It records up to 4,096 collision-moved positions for the camp return and refuses
non-finite positions, relocations or overflow. It cannot be rerun as a fresh
loop once this character earns progress; preserve that progress and use a new
explicit fixture for subsequent fresh tests. Use `tools/check-quest-loop.py` on
its complete capture with the character's `--guid`. The default name, Xboxdawn,
has earned progress, as has Xboxrise; both must be preserved. Create a new test
character for another fresh run. Xboxrise's uninterrupted native loop passes;
evidence and performance limitations are recorded in STATUS.


`-WalkReplay` is a separate read-only walking scenario using `Xboxnight` at the
saved Abbey position. It walks to the camp and back, restores the view, then logs
out/reconnects and checks inventory/XP/money. No combat, quest or trade commands
are requested. Use `tools/check-walk.py` for path/progress checks and per-stage
frame summaries. It refuses an unexpected initial position or active starter
quests, and returns to within 0.08 yards of its starting position. Use
`tools/check-profile.py capture.csv --walking` for current-frame phase timings.
Add `--continuous-avatar` to the walking checker to require a complete avatar
through every walking sample. The animation-cache regression passes this check.

Start a capture before launching a short replay so its initial state is included.
For a normal build, record at least 60 seconds of received telemetry and run
`tools/check-normal.py capture.csv --baseline earlier-normal.csv`, plus the
avatar/profile and build-receipt checks. This verifies normal input, disabled
staged fixtures and unchanged saved character state; it is not a controller test.

The bounded texture font is enabled by default. `-LegacyText` forces the original
pbkit drawing backend for comparisons and is reset by a normal launch. Validate
the expected backend with `tools/check-text.py capture.csv` (add `--legacy` for
the forced fallback, or `--walking` for route-only timing summaries). The checker
also validates that presentation subintervals sum correctly and font storage
stays fixed. Check actual screenshots independently for text and scene legibility.


Character/journey replays accept `-TestCharacter <name>` (2–12 ASCII letters).
For example, `-AvatarSwitchReplay -TestCharacter Xboxrise` creates or selects a
female Human, inspects her avatar, then restores the original character.
`-QuestLoopReplay -TestCharacter Xboxrise` subsequently runs the quest route only
if that character is untouched. Normal launches remove the override. Use the
appropriate saved GUID with `check-quest-loop.py --guid` or
`check-camp-quest.py --guid`; never reset earned progress to satisfy a fixture.


### xemu keyboard mapping

The [official xemu controller reference](https://xemu.app/docs/controller/)
lists A/B/X/Y as the same letter keys, Enter for Start, Backspace for Back,
1/2 for White/Black,3/4 for the stick clicks and arrow keys for the D-pad.
Left-stick directions are E/S/F/D and right-stick directions I/J/L/K;
W and O are the triggers. A real gamepad can instead be selected in xemu.
These documented mappings do not replace a transport test: physical gamepad
operation remains unverified, and earlier brief automated keyboard taps were
not observed by the guest. The pinned emulator polls current keyboard state,
so an extremely short synthetic press may be missed between polls.


## Back-button world map

In the world, press **Back** to open the current zone. **A** toggles 1x/2x zoom,
**left stick** pans at 2x, **X** centers on your position. **D-pad left/right**
browses maps on the current continent; **up** opens the continent and **down**
returns to the current location. **Back**, **B** or **Start** closes the map.
Movement, camera, triggers and action buttons are captured while open and through
the closing input until the controller returns to neutral. The settings menu
retains Back for logout/reconnect.

The yellow cross marks your position. Unexplored overlays stay hidden using the
server's Vanilla exploration fields. No quest POIs, party markers, map cursor,
world-level continent selection or dungeon-floor maps are implemented yet.
The 51 prepared map images do not imply 51 playable zones.

Prepare all map images with xemu stopped:

```powershell
.\build\host\wowx_assetc.exe --maps 'game\Data' build\xbox
.\build\host\wowx_map_tests.exe './build/xbox/'
.\scripts\run-xemu.ps1 -MapReplay
```

This replay injects client pad state and preserves Xboxer's progress. It does
not verify a physical controller. Capture telemetry before launching with
`tools/record_telemetry.py`; validate it using `tools/check-map.py`.

## Corpse-run and sustained testing

`-CorpseReplay -TestCharacter Xboxspirit` selects a dedicated fresh Human warrior,
walks to the camp, provokes one kobold, stops attacking, dies, releases, opens the
corpse map, reconnects as a ghost, walks back and reclaims. It then reconnects
alive and verifies progress. The existing Xboxspirit has completed this test;
create another fresh character through the character picker for a full rerun.
Do not reset earned characters. Recovery from an already-dead/ghost test character
is supported, but a partial recovery does not pass the full-run checker.

```powershell
python tools/record_telemetry.py --seconds 1000 --finish-on-scenario --output build/evidence/corpse-run.csv
# In a second terminal, after stopping only the dedicated xemu:
.\scripts\run-xemu.ps1 -CorpseReplay -TestCharacter Freshname
python tools/check-corpse.py build/evidence/corpse-run.csv
```

`-SoakReplay -TestCharacter Xboxdawn` repeats the verified Abbey/camp walking route, then
opens and closes the zone/continent map, utility wheel, bags, spellbook, journal
and settings. Every lap logs out/reconnects and checks saved progress. The default
is at least one hour of guest time, ending at a complete lap. `-SoakSeconds 300`
is a short fixture smoke test, not one-hour acceptance. No combat, trades or
character resets occur in this fixture. The selected character must already be at
the fixture's Abbey start. Xboxdawn returned there after the passing hour test.
Xboxnight is saved elsewhere following the interrupted first attempt, so the
implicit default is not currently suitable. Never reset a character to force a pass.

```powershell
python tools/record_telemetry.py --seconds 4200 --finish-on-scenario --guest-pid-file build/xemu.pid --output build/evidence/soak.csv
.\scripts\run-xemu.ps1 -SoakReplay -TestCharacter Xboxdawn
python tools/check-soak.py build/evidence/soak.csv
```

The optional `--guest-pid-file build/xemu.pid` observes the dedicated emulator
and records its exit code if it closes. A minute without valid telemetry also
ends the capture with a failure reason. Lifecycle evidence is saved separately
from the CSV. The watcher never closes or controls the emulator.

These fixtures inject ordinary client controller samples after SDL input. They
do not establish physical controller transport or physical Xbox performance.
The sustained fixture also does not cover crowded combat or forced network loss.
A normal launch clears all six fixture files, including SOAK.BIN.


### Original title scene preparation

After building the desktop converter, prepare the supplied original menu model:

```powershell
build/host/wowx_assetc.exe --backdrop 'game/Data' 'Interface\Glues\Models\UI_MainMenu\UI_MainMenu.m2' build/ui-prepared/TITLE.WXB
Copy-Item -LiteralPath build/ui-prepared/TITLE.WXB -Destination build/xbox/TITLE.WXB
```

Default manual login draws this prepared scene. `-LoginReplay` additionally
checks login/error/cancellation and transition to the saved Xboxer character.
`tools/check-backdrop.py <capture.csv> --world-transition` checks title residence
and its release at world entry. Original title particles, lighting/fog and full
PC menu widgets remain in progress; a rendered model is not complete title parity.

## Live character previews

The roster shows the selected character's prepared appearance and equipped
items. Creation previews follow race and sex. Right stick rotates horizontally
and zooms vertically; D-pad keeps its selection/form controls. The default
appearance catalogue contains 16 race/sex profiles. The September 16 afternoon
candidate adds all 80 legal starter outfits and Tauren/Undead backgrounds, with
host verification. The next candidate also contains Orc/Troll and NightElf
backgrounds. Native acceptance remains unfinished; the current candidate adds all five appearance controls and Randomize.

Prepare Human and Dwarf/Gnome backgrounds with the emulator stopped:

```powershell
build/host/wowx_assetc.exe --backdrop 'game/Data' 'Interface\Glues\Models\UI_Human\UI_Human.m2' build/xbox/HUMAN.WXB
build/host/wowx_assetc.exe --backdrop 'game/Data' 'Interface\Glues\Models\UI_Dwarf\UI_Dwarf.m2' build/xbox/DWARF.WXB
```

The revised `-PreviewReplay` combines the title/login/error/cancel fixture with
saved-world entry, equipped roster inspection, all 80 race/class/sex draft
outfits, all five appearance selectors and Randomize on all sixteen models,
rotation/zoom, keyboard cancellation and saved-world return. Capture 900 seconds
and run `python tools/check-preview.py build/evidence/native-front-end.csv --customization-plan build/evidence/hud-customization-plan.json`,
plus `tools/check-backdrop.py <capture.csv> --world-transition`. Capture a fresh
read-only server baseline first. The preview checker also compares the final
character against the settled first world entry instead of hardcoded XP/money.
This trace has passed host UI replay only; do not run it against the same account
while the user is testing physical hardware. It injects controller samples and
does not validate a physical controller. A normal launch clears the fixture.

The earlier accepted batch remains `native-preview.csv` (old 270-second trace);
full transition timings are in `native-preview-timing.json`. The new candidate's
memory and timing improvements must still be measured in xemu and on hardware.
The supplied physical-test folder uses the unchanged earlier checkpoint; see
[HARDWARE-TEST.md](HARDWARE-TEST.md).


## Character-management candidate (native acceptance pending)

On the roster, White opens deletion for the highlighted character. A opens the
keyboard; type DELETE and choose Done/Start, then review the named character and
press A separately to submit. B cancels either screen. A success requires the
server reply and a refreshed list without that character. "Deletion not confirmed"
means reconnect and inspect the roster before deciding whether another attempt
is needed. The client never resends a timed-out deletion automatically.

Use only a separate disposable account/character for native deletion acceptance.
Preserve Xboxer, Xboxnight, Xboxdawn, Xboxrise and Xboxspirit, as well as any new
characters the user creates. Do not run an automated shared-account login during
the user's physical test. The installed stable hardware package does not yet
contain this feature. Future native verification must cover keyboard cancellation,
list reordering/error paths, confirmation, empty roster, creation after deletion,
and entering a retained disposable character, with >=8 MiB measured free memory.

`python tests/character_protocol.py` runs the synthetic encrypted loopback suite
without private credentials or a real server. It includes a 31-second keepalive
and a 15-second timeout; the development batch ran the 40 applicable scenarios,
leaving the unchanged keepalive scenario to its existing evidence. Controller
state tests use generated button events, not a physical Xbox controller. No new
native management acceptance is claimed by these host tests.


## Appearance controls (development candidate)

On Create Character, Y opens appearance choices. Up/Down selects a category,
Left/Right changes it, X randomizes the appearance, right stick rotates/zooms and
A/B returns to the form. Randomize preserves the name, race, class and sex.
Race/sex changes reset those choices. The appearance is included in the creation
request after the separate name/form confirmation. Skin/hairstyle changes adjust
dependent face/color choices when required by the supplied Vanilla tables.

Stage WXP, WXA and WXL files together; old default-only packs do not provide this
editor's choices. The current hardware-test folder remains on its earlier
verified checkpoint. Native acceptance of this candidate is pending; inspect all
sixteen race/sex combinations, retain >=8 MiB measured free memory, and use a
separate disposable character for nondefault create/enter/reconnect verification.
The current PreviewReplay includes customization but has passed host simulation
only. Native acceptance is still pending. See
[CHARACTER-APPEARANCE.md](CHARACTER-APPEARANCE.md) for formats and host evidence.


### Combined customization acceptance preparation

Prepare and check the display-only fixture without changing the normal disc:

```powershell
python scripts/prepare-replay.py --preview --output build/hud-replay-20260916
build/host/wowx_preview_replay_tests.exe build/hud-replay-20260916/input.rpl build/avatars-looks-20260916 build/evidence/hud-customization-plan.json
```

This trace has 961 records / 18,994 frames, below the existing 1,024-record bound.
The host simulation produces 80 expected rendered-look samples (five per race/sex)
from the real catalogs. It also checks 16 randomizations and all 80 starter outfits.
No create/delete command is allowed. Keep this plan with the trace and source
receipt; regenerate it whenever those inputs change. The normal staging remains
MENU/live input; `--output` alone does not enable automatic login.

When the shared account is available, the already-authorized `-PreviewReplay`
launch rebuilds the disc with that fixture. Keep its new build receipt with the
capture; a normal-input candidate receipt cannot identify a replay-mounted disc.
The 900-second allowance includes the login fixture and approximately 633 seconds
of raw input at 30 fps. This is a capture allowance, not a measured performance
result. Capture current saved-character state before running; preserve that state
and inspect actual xemu screenshots of the new screen and randomized appearances.

WXTU telemetry (1,220 bytes) adds the **rendered** look, facial feature, successful
composition counter and full-size RGBA atlas hash, plus the UI category and
randomization count. The hash is taken before mip generation overwrites its CPU
scratch canvas, only on composition changes. No per-frame pixel readback occurs.
`--customization-plan` requires three drawn samples within each choice's hold,
increasing composition revisions, randomized rendered looks for every identity,
stable pixels between changes, all outfits/scenes, saved-world return and >=8 MiB
measured free memory. Missing new telemetry fails that gate. Hashes and selected
look coverage do not establish screenshot fidelity or physical input correctness.


## Player/target HUD candidate

The new development image adds original-art unit bars with live health and active
resource values, XP progress, dead/ghost states and target clearing. Later candidates
add real resident-model portraits and action icons; see [PORTRAITS.md](PORTRAITS.md)
and [ACTION-ICONS.md](ACTION-ICONS.md). Full HUD/menu parity remains incomplete.
The stable hardware-test kit is unchanged. New assets use WXU1 version 2; keep the
matching INTERFACE.WUI with the new XBE. Older atlases remain readable with the
previous text HUD.

The current PreviewReplay ends with Y target selection twice, then B to clear.
These actions do not cast, interact or attack. When the shared account is available,
use the same 900-second capture for all three checks:

```powershell
python tools/check-preview.py build/evidence/native-front-end.csv --customization-plan build/evidence/hud-customization-plan.json
python tools/check-backdrop.py build/evidence/native-front-end.csv --world-transition
python tools/check-hud.py build/evidence/native-front-end.csv --targets --portraits
```

Capture actual xemu screenshots of both unit frames, target changes, death/ghost
and the restored world. The read-only trace cannot cover live mana/energy combat,
and none of this establishes other class gameplay or physical pad behavior.
The host software preview is not a substitute for those screenshots. Do not run
shared-account automation while the user is testing the physical kit.
