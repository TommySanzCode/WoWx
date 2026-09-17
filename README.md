# WoWx

I'm working on getting Vanilla WoW running on the original Xbox. The goal is to
have the client run on a stock 64 MB console, with controls and menus that make
sense on an Xbox controller.

There's still a lot left to do! The client can boot, connect to a local server
and play through parts of Northshire, but this is still a development project.
Getting a few quests working is a start. Getting the whole game working properly
on this hardware is going to take more work.

## How it works

I'm using [WoWee](https://github.com/Kelsidavis/WoWee) as the starting point. It
already provides code for reading WoW's files, handling animations and talking
to a server. The Xbox side uses [nxdk](https://github.com/XboxDev/nxdk) and pbkit
to build an XBE and draw everything using the console's own graphics hardware.

The biggest challenge is fitting things into 64 MB of RAM. Tools on the PC
prepare the original maps, models and textures into files the Xbox can load
efficiently. The client loads nearby parts of the world as you move, reuses
textures and clears out things it no longer needs.

The Xbox runs the client, including the graphics and controller input. A PC runs
the local [vMaNGOS](https://github.com/vmangos/core) server, which handles things
like enemies, quest progress and saving your character.

## What's working so far

These have been tested in xemu with the Xbox's stock memory limit:

- Login, character selection and basic character creation.
- Terrain, buildings, nearby creatures and an animated player with equipment.
- Movement, targeting, melee combat and some warrior abilities.
- Some quests, XP, loot, inventory and vendor interactions.
- Separate tests for the map, hearthstone, death and recovering your corpse.
- Logging out and reconnecting with saved progress.

Newer builds also include work on character customization, the original HUD,
ability icons, cooldowns, fog, lighting and water. Some of that has only passed
separate tests so far. Testing everything together, especially on a physical
Xbox, is still ongoing. The xemu results aren't a promise of the same performance
on the console.

## What still needs work

Full world coverage, all the classes and abilities, the rest of the menus,
social features, professions, audio and plenty of other systems. Performance
and loading also need more work, especially in busy areas.

The [roadmap](docs/ROADMAP.md) goes into more detail. The
[feature checklist](docs/PARITY.md) tracks what is implemented and what still
needs testing.

## Building it

You'll need your own legit copy of **Vanilla WoW 1.12.1, build 5875**. The build
tools read the original game data to prepare the files used by the client.
Game files, prepared assets, firmware, passwords and built XBE/ISO files aren't
included here.

The build uses Windows, MSYS2, Python, nxdk and WSL Ubuntu for the server.
Tool and game locations go in environment variables or an ignored local
configuration file. The build notes explain what to set up.

- [Build and development notes](docs/BUILDING.md)
- [Controls and local playtesting](docs/PLAYTEST.md)
- [Latest hardware test instructions](docs/HARDWARE-20260917-R2.md)
- [Technical notes](docs/README.md)

I don't plan on hosting a public server. The plan is for people to run their own
locally and build the client using their own copy of the game. This repo is
private for now while development continues.

## Credits

A lot of credit goes to WoWee, the XboxDev/nxdk community, vMaNGOS and
[xemu](https://xemu.app/). This project builds on their work, along with the
other libraries listed in [NOTICE.md](NOTICE.md).

Their licenses and notices are kept in [licenses](licenses/), including WoWee's
additional restriction on commercial game use. The original World of Warcraft
artwork and game data belong to Blizzard.
