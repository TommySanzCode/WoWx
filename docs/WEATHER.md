# Vanilla weather state and lighting

## Wire and session contract

The Xbox client accepts Vanilla build 5875 SMSG_WEATHER (0x02F4) as exactly
13 bytes: little-endian u32 type, f32 grade, u32 sound ID and u8 change flag.
Type is 0..3 (fine/rain/snow/storm), grade is finite in [0,1], and change is 0/1.
Invalid packets leave the snapshot unchanged and fail the session rather than
publishing partial state. The sound ID is retained for future audio; it is not
the change flag. The existing desktop WoWee handler comments describe 8 bytes,
which is inconsistent with the pinned Vanilla server and protocol reference:

- upstream/vmangos/src/game/Server/Packets/Misc.cpp, WeatherUpdate::AppendBodyTo
- upstream/vmangos/src/game/Weather.cpp, SendWeatherForPlayersInZone
- https://gtker.com/wow_messages/docs/smsg_weather.html (Client Version 1.12)

A locked read-only snapshot is published by the world packet reader, including
updates before LOGIN_VERIFY_WORLD. Logout, disconnect and accepted map-transfer
start clear prior weather with a new revision. Weather arriving between transfer
start and NEW_WORLD is retained. A direct NEW_WORLD without a pending transfer
clears stale weather. Near teleports retain current weather until the server
publishes the new zone. Pinned vMaNGOS Player::Update recomputes area/zone itself;
this work does not fabricate client-side weather or change server settings.

## Presentation and limits

The server's grade blends clear and overcast/rain DBC lighting profiles. Rain,
snow and storm retain their individual type and sound ID; the shared overcast
profile is the current inclement-lighting approximation, not full weather parity.
Smooth updates approach the target with an exponential response; instant updates
apply the target weight and lighting on the same frame. Fine weather forces zero
inclement weight even if its raw packet grade is nonzero. A small convergence
threshold finishes clearing exactly and avoids resampling an invisible profile
forever. No per-frame allocation, I/O, texture or framebuffer is added.

Explicit underwater/indoor requests override weather lighting. The normal world
still lacks automatic water/interior classification, so this interface does not
claim correct weather rendering through every interior. Precipitation particles,
cloud geometry, storm/snow-specific presentation and audio remain unfinished.
No visual or sound system has been waived by implementing packet handling first.

## Validation

Targeted weather tests cover packet sizes/values, atomic rejection, finite grade,
revision wrap, smooth/instant transitions, clearing/reset and real-catalog grade
samples. Twelve encrypted localhost scenarios use only a public fixture key:
early/late weather, rain/snow/storm/clear changes, transfer reset, weather during
transfer, logout cleanup, truncated/overlong packets, TBC-shaped packets and
invalid enums/floats/flags. The default protocol suite includes these scenarios.

WXPF0010 is an isolated credential-free native fixture with ten phases. It sends
synthetic 13-byte packets through the production decoder/presentation and draws
the real resident Northshire world, three scaled wolves and an independent
portrait. Native phases include explicit environment overrides and malformed
packet rejection; they are not a live shared-account or physical-input test.
WXW1 is a separate strict 68-byte telemetry companion; WXTZ stays 1,472 bytes.
Exact candidate identities and native measurements are recorded in STATUS.md
once the batch is accepted.

## Accepted native checkpoint

September 17: build/candidates/20260916-weather-state. One combined native run
passed all ten phases (3,681 main frames; 3,680 weather/lighting companions).
Minimum free 39,180 KiB; guest frame p50/p95/p99/max 33/34/34/35 ms. The recorded
resident world excludes startup/preload and does not establish streaming, combat,
physical controller or hardware acceptance. No shared account was used.

Actual clear and instant-snow screenshots show the overcast lighting change;
there are no precipitation particles in this batch. Numerical checks cover rain,
storm, clearing, instant changes and explicit environment priority. Source/disc
identities, screenshot phase JSON, raw telemetry, host checks and limits are
preserved in the fixture/candidate and detailed in STATUS.md.
