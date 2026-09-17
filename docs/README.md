# Project notes

The main [README](../README.md) is the quick version. These files go into how
things work, how to build the client and what still needs attention.

## Getting started

- [Building](BUILDING.md): tools, asset preparation and the local server.
- [Playtesting](PLAYTEST.md): controller mappings and development test modes.
- [Hardware testing](HARDWARE-20260917-R2.md): the latest prepared test package.
- [Roadmap](ROADMAP.md): where I want to take the project.
- [Feature checklist](PARITY.md): working pieces and unfinished systems.

The hardware guides describe local development packages. Those packages contain
game data and aren't part of this repository.

## How the client works

- [Architecture](ARCHITECTURE.md): graphics, networking, memory and data flow.
- [Streaming](STREAMING.md) and [stream timing](STREAM-CLOCK.md).
- [Character appearance](CHARACTER-APPEARANCE.md) and [profile loading](PROFILE-LOADING.md).
- [HUD](HUD.md), [portraits](PORTRAITS.md) and [action icons](ACTION-ICONS.md).
- [Cooldowns](COOLDOWNS.md) and [global cooldowns](GLOBAL-COOLDOWNS.md).
- [Fog](FOG.md), [lighting](VERTEX-LIGHTING.md) and [environments](ENVIRONMENTS.md).
- [World materials](WORLD-MATERIALS.md), [terrain water](TERRAIN-LIQUIDS.md)
  and [building water](LIQUIDS.md).

## Development records

[STATUS.md](STATUS.md) and [OVERNIGHT.md](OVERNIGHT.md) keep the detailed build
and test history. They're working notes, so older entries describe older builds.
An isolated graphics test or emulator run doesn't mean that feature has passed
a full gameplay test on an actual Xbox.
