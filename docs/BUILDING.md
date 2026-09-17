# Build and development notes

Run these commands from the repository root. A fresh checkout does not include
prepared game assets, a server database or emulator files. These notes cover
individual preparation steps; there is still some manual setup involved.

Start with your own copy of Vanilla 1.12.1, build 5875. The examples use
`game/Data` for the original game files. Asset preparation scripts also accept
`--data` (or `--installation` for the server assets) for another location.

## Local configuration

Copy `config/paths.example.json` to `config/local.paths.json` and fill in the
locations you use. The local file is ignored by Git. Environment variables with
the same names take priority, and explicit script arguments take priority over
both.

| Setting | What it points to |
|---|---|
| `NXDK_DIR` | Your pinned nxdk checkout |
| `WOWX_MSYS_BASH` | The `usr/bin/bash.exe` inside your MSYS2 installation |
| `WOWX_DATA_DIR` | The original game's `Data` directory |
| `WOWX_XEMU` | The xemu executable; otherwise `xemu.exe` is found on PATH |
| `WOWX_XISO` | Optional extract-xiso executable; otherwise found under nxdk or on PATH |
| `WOWX_MCPX` | Your MCPX boot ROM |
| `WOWX_BIOS` | Your Xbox BIOS image |
| `WOWX_HDD` | Your xemu hard disk image |
| `WOWX_EEPROM` | Your source EEPROM; otherwise the standard xemu application-data location |

`bootstrap.ps1` and `build-xbox.ps1` accept `-NxdkDir`. The two build wrappers
accept `-MsysBash`, and the emulator launchers accept `-Xemu`. To call `make`
directly in MSYS2, export `NXDK_DIR` as a Unix-style path first.

The xemu launcher writes its generated settings to ignored
`config/local.xemu.toml`, uses a separate EEPROM copy and enables disposable
hard disk writes. Firmware and disk images stay outside source control.

These notes include older feature snapshots. Check [the status log](STATUS.md)
and [feature coverage](PARITY.md) for newer work and its testing limits.
For a quick overview, go back to [the project README](../README.md).

## Build

Prerequisites: MSYS2 with MinGW GCC, CMake, Ninja, Clang and make; Python 3.12+;
the pinned nxdk checkout; WSL Ubuntu for the server; and the original game data.
[Dependency pins](../config/dependencies.json) record the revisions.

```powershell
.\scripts\bootstrap.ps1
.\scripts\build-tools.ps1
& .\build\host\wowx_assetc.exe 'game\Data' build\xbox\world.wxp
& .\build\host\wowx_assetc.exe --actors 'game\Data' build\xbox\actors.wxp config\creatures.txt
& .\build\host\wowx_assetc.exe --maps 'game\Data' build\xbox
& .\build\host\wowx_assetc.exe --spells 'game\Data' build\xbox\SPELLS.WXS
.\build\host\wowx_packcheck.exe build\xbox\world.wxp build\xbox\actors.wxp
python scripts/prepare-test-auth.py
.\scripts\build-xbox.ps1
.\scripts\run-xemu.ps1
```

The world converter discovers maps from the game's DBC/WDT data. Zones use the
same conversion path. To prepare the current four-tile test set, with xemu stopped:

```powershell
python scripts/prepare-world.py --tile 0:32:48 --tile 0:32:49 --tile 0:33:48 --tile 1:39:33 --jobs 2
python scripts/compress-world.py
python scripts/stage-world.py --source build/compressed-world
.\scripts\run-xemu.ps1
```

`--maps 0 1` selects both continents; `--all` selects all discovered terrain maps.
Do not interpret conversion as full gameplay or renderer coverage. WMO-only
instances need a separate root-scene export path. Batches resume after matching
archive/output hashes; changed verifiers recheck unchanged cooked assets.
Compression preserves source packs and validates geometry exactly. Select a new
`--output` directory after changing its optimizer recipe. Avatar/creature packs
remain uncompressed. `wowx_packopt input.wxp new-output.wxp` can also compress
the standalone boot scene; stage that output as `build/xbox/world.wxp` with xemu
stopped. The current five world packs shrink from 471.1 MB to 253.2 MB; this does
not establish a complete Vanilla installation size.

Prepare the local server below before generating `testauth.bin`. Outputs:

- `build/xbox/default.xbe`
- `build/wowx.iso` — xemu-compatible XISO
- `build/xbox/world.wxp` — private terrain/building/prop assets, approximately 13.9 MiB compressed (46.5 MiB before compression)
- `build/xbox/actors.wxp` — private creature templates, approximately 30.7 MiB

The launcher uses a dedicated configuration, copied EEPROM, `-snapshot`, `-m 64`,
native scale and QMP on `127.0.0.1:4444`. Firmware/HDD paths come from your local configuration.
Close this dedicated instance before rebuilding its mounted ISO:

```powershell
python tools/qmp.py quit
```

Each build/launch provisions fresh development entropy; consumption is flushed
to the guest disk before use. Release install/update/recovery and power-loss
behavior still need validation. Development images contain private assets and
local test credentials; they are not distribution packages.

## Local server

vMaNGOS runs under WSL Ubuntu. Packages include `build-essential cmake ninja-build
libboost-all-dev libssl-dev zlib1g-dev libmariadb-dev mariadb-server`.
Only dedicated `wowx_*` databases are used. Credentials are generated in ignored
`server/local-credentials.json`. Realm/world ports are loopback 3725/8086.

```powershell
$repoRoot=(Get-Location).Path.Replace('\','/')
$linuxRoot=(& wsl -d Ubuntu --exec wslpath -a $repoRoot).Trim()
wsl -d Ubuntu --exec bash "$linuxRoot/scripts/build-server.sh"
python scripts/prepare-server-assets.py
wsl -d Ubuntu -u root -- service mariadb start
wsl -d Ubuntu -u root --exec python3 "$linuxRoot/scripts/prepare-server.py"
python scripts/prepare-test-auth.py
```

In separate WSL shells, open the repository's `server` directory and run:

```sh
./bin/realmd -c etc/realmd.conf
./bin/mangosd -c etc/mangosd.conf
```

The realm advertises xemu's gateway `10.0.2.2:8086`. Physical hardware needs a
separate LAN configuration. Asset extraction stages use native Linux storage
and completion markers. All map/vmap data and the initial Northshire mmap tile
are prepared; other pathfinding tiles remain. Check installed outputs with
`python scripts/prepare-server-assets.py --verify`.

## Tests and replay

```powershell
.\build\host\wowx_runtime_tests.exe
.\build\host\wowx_entity_tests.exe
.\build\host\wowx_dbc_tests.exe
.\build\host\wowx_game_tests.exe
python tests/auth_protocol.py
python tests/world_protocol.py
.\build\host\wowx_auth_probe.exe build\xbox\testauth.bin --world
```

The last command needs the real server and an offline disposable character.
`--move` also moves that character 0.5 units before clean logout. Fake-peer
fixtures use independent protocol calculations and public test credentials.

Start telemetry in one shell, then `scripts/run-xemu.ps1 -Replay` in another:

```powershell
python tools/record_telemetry.py --seconds 240 --expect-replay --output build/evidence/replay.csv
```

The trace waits for world entry, exercises all 24 slots, moves/turns/jumps,
saves controls, logs out and reconnects. Acceptance requires zero load failures,
at least 8 MiB free, both world entries and a connected final session.
Guest timing and automated replay do not prove hardware performance or physical input.
Launch without `-Replay` for ordinary input.
`-SceneReplay` selects a separate camera/movement trace with diagnostics hidden;
it does not satisfy the full controls/logout/reconnect acceptance scenario.

`-QuestReplay`, `-TurninReplay` and `-CombatReplay` are separate gameplay scenarios.
Combat uses state-driven controller samples; the others use fixed traces. See
[STATUS.md](STATUS.md) for actual outcomes and limitations.
`-InventoryReplay` verifies shield storage/equip; `-KoboldReplay` exercises repeated
combat. `-BoundaryReplay` walks across the 48/49 tile boundary and reconnects.
For that scenario, stop xemu, set the disposable character with
`wsl -d Ubuntu -u root --exec python3 "$linuxRoot/scripts/reset-test-position.py" --fixture boundary`
(using `$linuxRoot` from the server setup above),
start a capture with `--expect-boundary`, then launch with `-BoundaryReplay`.
Use a 210-second capture to include compilation, boot and the reconnect hold.

## Local server startup

For a repeatable local start, run `scripts/start-server.ps1` before
`scripts/run-xemu.ps1`. The server command reuses existing workspace processes,
preserves database state and appends console logs under `server/logs`.

## Player asset preparation

The player cooker now composes Vanilla body skin, face, hair/scalp, underwear and
equipment regions, selects mesh variants, and attaches a held weapon/shield.
It shares the existing NPC and WoWee geometry rules. Example:

```powershell
.\build\host\wowx_assetc.exe --player '.\game\Data' build/player.wxp config/player.txt
python tests/player_assets.py --data '.\game\Data'
```

Profiles contain seven appearance numbers and twenty equipment display/type
pairs in character enumeration order. These are preparation inputs, not account
credentials or extracted game assets. The validation script cooks all sixteen
default race/sex combinations, the current test character's equipment and an
alternate Human appearance, then checks every batch through the runtime loader.
Missing optional Tauren facial/scalp overlays are reported in the local results.

The modular avatar path is now connected to the native renderer:

```powershell
.\build\host\wowx_assetc.exe --avatar '.\game\Data' build/avatar/AVATAR.WXP config/player.txt
.\build\host\wowx_avatar_tests.exe build/avatar/AVATAR.WXP build/avatar/AVATAR.WXA
python scripts/prepare-avatars.py --data '.\game\Data' --output build/avatar-profiles --defaults
# Stop this project's xemu before staging or rebuilding its mounted disc.
Get-ChildItem build/avatar-profiles/*.WXP,build/avatar-profiles/*.WXA | Copy-Item -Destination build/xbox
python tests/player_assets.py --modular --data '.\game\Data'
```

`--avatar` prepares all body mesh variants for one appearance, separate item
components, and clothing overlays. It accepts an optional final catalog file
containing `display slot inventory_type` rows. Native code composes the body
texture when server equipment changes and streams selected geometry in a 4 MiB
budget. It does not prepare every outfit combination. The profile helper prepares
all sixteen default race/sex looks with the supplied profile's five item displays;
`--items` extends that catalog. Omit `--defaults` and pass `--profile` to prepare
one specific appearance. Choose a new output directory on each preparation.

The client selects a short filename from all seven server appearance fields,
validates the companion metadata, and keeps only one profile resident. It releases
the previous model before loading a changed look. Missing/corrupt profiles remain
undrawn and are attempted once per appearance/connection, avoiding repeated disk
reads. These prepared defaults do not establish full race/class zone, cosmetic
or equipment coverage. Unknown equipment remains explicitly reported.
Weapon grips, sheathing, transformations and complete appearance coverage remain.

The native gear replay verifies actual shield draw removal, re-equip and saved
reconnect with the avatar visible. The third-person camera uses collision geometry
and falls back close to the player while collision is loading.

`scripts/catalog-creatures.py --output build/creatures-near.txt` reads the private
server's nearby spawn catalog and alternate genders around the saved character.
It observes current event eligibility, writes a proposed display list, and never
changes server data. Prepare and validate that list with `assetc --actors` and
`wowx_packcheck` before staging it. Scripted/summoned/seasonal model coverage is
still tracked separately; this is a reusable catalog path, not zone-specific code.

## Controller prototype

Left stick moves, right stick looks, A jumps, Start opens controls. Triggers select
three layers of eight face-button/D-pad slots. Y cycles nearby creatures, X interacts
(quest/loot/attack), B stops/clears targeting, LT+A starts auto attack. Black opens
the current bag/equipment interface. LT+B uses Heroic Strike on the default warrior
stance bar. Known spell and item action slots are wired; full class/form mappings,
spell names, cooldown UI and effects remain.
Pressing the right stick toggles development diagnostics.
In controls: up/down changes dead zone, trigger+button selects a slot, left/right
remaps it, X saves, Back logs out/reconnects, Start/B returns.
Bindings are stored in `E:\WOWX\controls.bin`; disposable xemu disk writes are
lost when that emulator run ends. Cursor/radial modes, on-screen keyboard,
full gameplay menus and enemy/party targeting categories remain.

## Source and assets

WoWee's noncommercial game restriction is preserved. Its separately restricted
original music is excluded. See [NOTICE.md](../NOTICE.md) and [licenses](../licenses/).
Game archives, converted packs, credentials, emulator firmware/disks, server
data and build outputs remain outside source control.


### Controller quest journal

In the world, press Start then A for the quest journal. Choose with D-pad and A;
use X for description/objectives, left/right for pages, and B to return. Quest
text comes from Vanilla server queries; counters and bag quantities update from
live state. The 20-entry metadata cache is fixed at 83,604 bytes. See
[playtest notes](PLAYTEST.md) for current coverage and replay evidence.
