# Global world-model maps

This batch extends reusable preparation and native region routing to maps whose
WDT names a single global WMO. It does not implement complete instances, encounters,
transports, water, visibility portals or server entry workflows.

## Shared implementation

- Strict WDT chunk bounds, version, duplicates and tile indices remain validated.
  Global roots additionally require a bounded terminated path, one 64-byte MODF,
  valid name offset, finite placement components and no terrain/global mixture.
  Pinned WoWee interprets the validated placement and the WMO/M2/BLP dependencies.
- Zero-position global instances omit the terrain-origin offset. Rotation signs
  and order match pinned core/world_loader.cpp; nonzero positions use its ADT
  conversion. Host checks cover zero/nonzero origins and rotated axes.
- The cooker reuses terrain-building geometry/material/collision conversion and
  selected/default doodad sets. It writes short G###.WXP files, below 1 GiB,
  with the existing WXP v5 format and shared mip chains. Runtime caches remain
  bounded; preparing a whole pack does not load its entire contents into Xbox RAM.
- prepare-world.py --maps now handles terrain and global maps together;
  --world-models prepares all discovered global maps, including development
  content. Archive/cooker/verifier identities and per-pack hashes govern resuming.
  Global coverage reports are also hashed before reuse.
- Each global report lists loaded archive member paths/sizes and known omitted
  features: liquid groups, authored lighting/fog, portals, complex materials and
  effect-only doodad models. Geometry-free particle emitters are recorded explicitly
  as unsupported effects; ordinary malformed geometry still fails conversion.
- WXI1 version 2 uses the existing 32-byte cell's reserved field as a region kind:
  zero is terrain; one is a global WMO, with x=y=0 and G###.WXP. Version 1 is still
  accepted and requires zero. Mixed global/terrain records for one map, duplicates,
  unknown kinds, unsafe names, invalid sizes and unsorted identities are rejected.
- Native global selection follows map identity independently of terrain grid
  coordinates. Map changes cancel pending work, detach old packs and pump the
  existing index/payload stages under their shared frame quotas. Collision and
  rendered geometry use authoritative world coordinates; no player position is
  fabricated for gameplay.
- Python preparation/staging share one strict index encoder/decoder. Only verified
  packs enter the index. An entirely failed batch writes an explicitly invalid
  empty index, so an earlier index cannot masquerade as the new result.

## Evidence and observed failures

The supplied archives contain 44 valid WDTs: 23 terrain maps with 2,429 tiles and
21 global-model maps. This enumeration includes developer/unreleased entries;
it is not a claim that all 44 are required playable Vanilla maps.

build/global-worldpacks-v2-20260917 contains 20 verified global packs totalling
415,761,298 bytes. Each passes the production pack loader and all payload checks.
The largest file is BlackrockDepths, 59,448,740 bytes; the largest index is DireMaul,
8,343 entries. HordePVPBarracks (450) fails finite-vertex validation and remains an
explicit failure in report.json. It is excluded from the runtime index.

The first conversion attempt is preserved in build/global-worldpacks-20260917
and is rejected: zero-origin global maps incorrectly received the terrain offset,
and four effect-only doodads were initially rejected as invalid meshes. Reading
the pinned WoWee world loader resolved the coordinate convention. The second
attempt records the four unsupported effect-only models instead of treating their
absence of triangles as corruption. These missing effects remain parity work.

Host checks: 153 DBC/WDT boundaries, 405 runtime/collision/allocation checks,
16 scene/transform checks, the mixed-index malformed-input suite and 12 telemetry
tests. The map-wide route tests include terrain/global/map-unavailable transitions
and crossing terrain-grid coordinates while remaining in the same global map.

The initial generic regioncheck invocation looked only for terrain via wx_ground,
so it reported no floor for all global maps. This was an inappropriate probe, not
a passed test. Its log is preserved. A dedicated --point mode now uses production
wx_floor around a supplied height and the frame-budgeted streamer. It reaches
resident geometry and a valid floor without errors for Stockades (34), Wailing
Caverns (43), Deeprun Tram (369), Ragefire Chasm (389) and Molten Core (409), after
141/189/164/37/41 preparation frames respectively. Host simulated memory figures
are not Xbox headroom measurements.

WXPF0013 is a credential-free native fixture cycling those five map identities at
inspection points selected from original horizontal collision triangles. It checks
the selected global identity, requires 120 frames of complete streaming plus floor
and visible geometry per map, and reports the existing WXS2 frame quotas. The
checker requires complete nonterminal main/companion identities, all five map
holds, no errors, and measured headroom.

Native acceptance passed: 6,866 main /6,865 companion samples, complete identities,
six cycles, all five map holds, zero unexpected failures. Minimum free 36,184 KiB
(35.34 MiB); isolated frame p50/p95/p99/max 33/34/34/35 ms. Index maximum 10 ms,
payload streaming maximum 19 ms. All 309 source identities and both normal/fixture
output sets matched after capture. Candidate: build/candidates/20260917-global-world.
The detailed checkpoint in STATUS.md records hashes, actual screenshots and limits.

## Scope still open

Inspection points are diagnostic surfaces, not verified instance entrances.
The Tram point lies in the underwater scene; its first screenshot exposes missing
water and partial loading. A floor hit and some rendered batches do not prove
complete geometry or original visual fidelity. Current 256-render/64-collision
residency caps can omit content in dense scenes; coverage must be improved without
silently declaring the omitted content complete.

Remaining work includes automatic indoor/water classification, spatial splitting,
visibility portals, all authored materials/lights/vertex colours/fog, animated
doodads/effects/liquids, missing-map diagnostics and map 450's rejected geometry.
Instance entry, NPC/object population, navigation, encounters, transports, server
persistence and controller UI are separate live acceptance gates. Only four
terrain tiles remain prepared; global-map preparation does not complete either
continent or the terrain-based instances/battlegrounds.

Normal development images contain the new routing code but retain their existing
terrain index. The five private global packs are isolated in the native test disc.
Stable hardware packages and reserved accounts remain untouched.


## Material follow-up

The later WORLD-MATERIALS.md checkpoint adds WXP v6 material semantics and
regenerates the same twenty maps with unchanged geometry/mip pixels. Its combined
native capture covers the five maps plus a synthetic blend chart. The earlier
v5 assets/captures above remain preserved. Map 450 now has a precise source NaN-UV
diagnosis; it is still excluded. This follow-up does not complete instance gameplay
or the visual/streaming limitations listed above.
