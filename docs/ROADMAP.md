# Roadmap to a finished Vanilla Xbox port

Approved implementation scope, September 16, 2026. Full original Vanilla 1.12.1
build 5875 gameplay and interface workflows, with controller adaptations and
optimizations for a stock 64 MiB Xbox. The port remains incomplete.

## Working rules

- Reuse pinned WoWee parsers, protocol behavior and data interpretation. Validate
  Vanilla differences against the supplied assets and pinned vMaNGOS source.
- Keep gameplay independent of graphics, input, audio and storage. Native UI
  consumes read-only snapshots and validated commands.
- Preserve checkpoints, source/disc identities, characters, private assets and
  credentials. Do not replace the hardware kit with an unaccepted candidate.
- The shared account/server remains reserved for hardware testing until released.
  Independent host and credential-free native development can continue.
- Work in substantial feature batches: targeted parser/allocation checks while
  developing, one combined native acceptance per batch. Endurance is reserved
  for major integration and release gates.
- No upgraded RAM or reduced gameplay scope without a user decision. Native core
  UI workflows are the first-release target; third-party addons are deferred.

## 1. Consolidate the current client (active)

Integrate character management/customization/outfits, original title and HUD,
portraits, icons, action cooldowns and feedback. Complete resource/range/usability,
quantities, charges and casting/channel feedback. Resolve remaining aura and
cooldown modifier ambiguities.

Acceptance: combined login, customization, world entry, combat, inventory,
logout/reconnect against authoritative server state. Preserve progress and at
least 8 MiB measured free through transitions. Isolated candidates do not satisfy
this combined gate. Keep it pending while the hardware account is reserved.

## 2. World rendering and stock-hardware foundation (active; hardware pending)

- Add shared world fog parameters for terrain, buildings, characters and effects.
  Initial outdoor fallback: 90 to 145 units, before the current 160-unit cull.
  NV2A fog coordinate/final combiner must preserve texture alpha. Match the
  horizon, restore UI/portrait/preview state, and separate indoor/underwater rules.
- Reuse WoWee lighting data interpretation for zone colors/time; interpolate
  environmental changes. No screen-sized render target in the initial design.
- Break synchronous world reads/conversion/uploads into bounded stages; prefetch,
  use residency hysteresis and distance-based model/texture detail. Measure each
  stage. Share geometry, improve visibility and schedule animation work.
- Keep gameplay entity tracking independent from visual model limits.
- Test boot, physical controls, networking, world entry, memory and timing on the
  stock console early. A feasibility failure is an explicit engineering blocker.

Acceptance: repeatable terrain boundaries and indoor/outdoor routes; improved
cutoff appearance; measured streaming costs; initial physical-hardware report.
Fog does not substitute for missing geometry, dependency coverage or timely loads.

September16 checkpoint: staged world/NPC/avatar streaming has two scoped native
route cycles. Authored fog and server clock pass the offline native batch in
FOG.md, with source fog data for26maps and explicit empty-profile fallbacks.
Full sky/material/environment classification, profile opening/composition,
general content and physical hardware remain pending.

## 3. Expand reusable content preparation

Batch both continents, instances and battlegrounds into indexed packs. Complete
terrain, buildings, doodads, collision and model dependencies. Cover every player
appearance/equipment display, other players and creature/object state. Implement
swimming, falling, doors, transports and map transfers. Emit automated missing and
unsupported-content reports. No manual implementation per zone.

Acceptance: every required map prepares and enters; representative routes across
all environment types have complete dependencies and respect memory limits.

## 4. Complete combat and progression

Cover all nine Vanilla classes: all resources, unit/ground targeting, channels,
interrupts and combat feedback; auras, dispels, forms, pets, summons, resurrection
and class bars; talents/trainers/skills; every quest objective/reward and inventory
operation; professions, gathering, recipes, crafting and advancement.

Acceptance: class-by-class evidence for learning, combat, resources, talents,
equipment, death and saved reconnect. Warrior-only success is insufficient.

## 5. Finish original UI and remaining systems

Functional original menus/settings, bags/equipment/tooltips, spellbook, talents,
quests, minimap and unit/group frames. Complete vendors/repair/buyback, bank,
mail, auction and confirmed trade; chat/social/guild/party/raid/group loot;
mounts/taxis/boats/zeppelins/portals/instances/encounters/PvP. Controller focus,
cursor, text entry, remapping and ground targeting must cover every workflow.
Preserve original artwork, information and behavior at readable 640x480 layouts.

Acceptance: every PARITY.md workflow, cancellation/error path and authoritative
persistence works end to end. Visible controls alone do not count.

## 6. Presentation and release qualification

Stream music/ambience/positional effects/voices/UI audio. Finish water, weather,
day/night, world spell effects, animation and attachments. Bound audio/effects
with gameplay-critical feedback prioritized. Optimize crowded/mixed workloads.
Deliver reproducible builds, private asset preparation, XBE/XISO installation,
notices and a final compatibility report without bundled credentials.

## Release gates (all required)

1. Complete original-game coverage in PARITY.md, implemented and verified.
2. At least 8 MiB measured free on stock 64 MiB hardware, including transitions.
3. 640x480/30 fps target evaluated with distributions and stalls over travel,
   combat, cities and group content. xemu timings are separate evidence.
4. Repeated hour-long mixed combat/travel/UI/disconnect/world-entry sessions.
5. Authoritative recovery after logout, disconnect and restart without lost progress.
6. Physical controller verification, separate from injected replay/fixtures.
7. Reproducible builds/assets/install and preserved notices/no bundled credentials.

No completion percentage or reliable release date is asserted before the
rendering, streaming and stock-hardware feasibility gates establish it.
