# Global cooldowns and spell modifiers

This extends the [base cooldown candidate](COOLDOWNS.md). Full Vanilla class
behavior and interface parity remain open. The hardware kit is unchanged.

## Implemented behavior

- Validated `SMSG_SPELL_START` starts the player's matching global cooldown (GCD)
  category. Preparing-cast interruption through `SMSG_SPELL_FAILED_OTHER` clears
  that cast's GCD. Spell completion does not restart it, and a later failure does
  not cancel a completed cast's timer. Other caster GUIDs do not change it.
- Vanilla's zero-duration `SMSG_SPELL_COOLDOWN` entry is a GCD hint, including
  in-combat weapon changes. It no longer erases an existing school lockout.
- Server flat/percentage modifier packets replace signed totals for all **64**
  spell-family bits and 29 operations. Operations 11 (recovery) and 21 (GCD) apply
  to matching player families, with the ignore-caster-modifier attribute honored.
- Player cast-speed and ranged-attack-time fields update before the next packet
  in a received batch. Haste applies the pinned server's eligibility rules and
  1,000–1,500 ms clamp. Zero base ranged recovery still uses equipped attack time;
  category flags support the wand GCD. An existing longer timer remains visible.
- GCD-only icon shading uses the radial sweep without numeric countdown text.
  Normal longer recovery retains its countdown. Input and server cast acceptance
  are unchanged; an unshaded icon alone does not mean the action is usable.

The fixed cooldown state is **60,216 bytes**, including 16 GCD categories and
14,848 bytes of signed modifier totals. All updates/queries are allocation-free.
Full active storage increments overflow rather than evicting a running timer.
The supplied DBC only has categories 0 and 133 with nonzero start recovery.
The isolated native fixture has a separate fixed state; no additional GPU
allocation is required for this extension. The production state remains outside
the large transactional gameplay copy.

## Asset format and source references

WXCD v2 keeps the four-word header and expands each record from 20 to 56 bytes.
In order, its fourteen uint32 values are spell, category, recovery, category
recovery, attributes, start-recovery category, start-recovery time, family,
family mask low/high, attributesEx2, attributesEx3, damage class, category flags.
The Vanilla Spell.dbc indices are 0/2/19/20/6/157/158/160/161/162/8/9/164;
category flags join SpellCategory.dbc by ID. These are original 173-column DBC
indices, not the expanded server SQL spell structure.

The supplied assets produce **22,357 records / 1,251,992 resident bytes** and
1,252,008 bytes on disc. v1 assets remain readable, expanding missing fields to
zero; they do not provide GCD/family metadata. The runtime rejects invalid
dimensions, IDs, ordering, timing bounds, family/category bounds and damage
classes before publishing the catalog. Loading retains an 8 MiB memory guard.

Primary references are the pinned vMaNGOS Player.cpp AddGCD/AddCooldown,
ApplySpellMod and SendSpellMod; Spell.cpp preparation/cancellation/completion;
Server/Packets/Spell.cpp; and UpdateFields_1_12_1.h. WoWee's classic DBC reader and
the [WoWDBDefs Vanilla Spell layout](https://github.com/wowdev/WoWDBDefs/blob/master/definitions/Spell.dbd)
confirm raw asset field positions. The pinned WoWee modifier handler accepts
only six groups, so it is not reused for Vanilla's 64-bit packet indexing.

## Explicit remaining gaps

The modifier packets contain per-bit totals, not the contributing aura masks.
If several nonzero matching bit totals are present, summing or deduplicating
them can miscount overlapping auras. The client increments `modifier_ambiguous`
and retains base timing. Resolving aura identity/charged modifier ordering and
verifying every talent/class remains required. This is partial support, not a
claim that all modified spells now match the PC client.

Timers begin at server packet receipt. Local cast prediction, latency correction,
server-customized DBC values and the server's configurable update subtraction
are not represented. Other controlled casters, pets and all form/weapon/proc
interactions still need implementation and live coverage. Resource/range checks,
charges/counts, complete bar/menu layout and remaining Vanilla UI remain open.

## Reproduction and evidence scope

```powershell
cmake --build build/host --target wowx_assetc wowx_cooldown_tests wowx_global_cooldown_tests wowx_icon_tests wowx_world_fixture
build/host/wowx_assetc --cooldowns 'game\Data' build/NEW-COOLDOWN.WCD
build/host/wowx_cooldown_tests build/NEW-COOLDOWN.WCD
build/host/wowx_global_cooldown_tests build/NEW-COOLDOWN.WCD build/candidates/20260916-cooldowns/COOLDOWN.WCD
build/host/wowx_icon_tests build/xbox/ICONS.WIC build/xbox/INTERFACE.WUI
python tests/world_protocol.py
python tests/preview_telemetry.py
```

The encrypted world fixture uses a public test key and synthetic spell metadata;
it exercises player field updates, cast lifecycle and published UI timer state.
It never connects to the shared test server or reads account credentials.
The first run's appearance assertion failed because the new fixture omitted
the roster's skin/hair fields. Supplying those fields fixed the fixture without
weakening the existing assertion; the full 77-scenario run then passed.

After staging the v2 asset, building and recording the source/output receipt:

```powershell
python scripts/prepare-portrait-test.py --global-cooldowns --output build/NEW-GCD-FIXTURE
scripts/run-portrait-test.ps1 -Fixture build/NEW-GCD-FIXTURE -Capture build/evidence/NEW-GCD.csv -Seconds 115
python tools/check-global-cooldowns.py build/evidence/NEW-GCD.csv
```

Marker `WXPF0004` selects eight phases: ordinary cast, preparing interruption,
percentage GCD modifier/completion/late failure, haste, GCD hint with school
lockout, modified Charge category recovery, modifier removal, and another
caster's interruption. It uses the production parser/query/UI and real converted
DBC. Context and packets are injected, with a deterministic clock of 33 ms per
frame. This is not controller replay, physical controls, live combat or a world
checkpoint. Actual frame intervals use the guest clock independently.

WXTZ telemetry is 368 words / 1,472 UDP payload bytes. It appends catalog version,
modifier totals/ambiguities, GCD starts/cancellations, visible GCD mask, cast-speed
factor and player family. Older captures remain decodable. This reaches a
1,500-byte IPv4 packet with UDP/IP headers; further metrics need a separate packet
or format redesign instead of growing this datagram.

## September 16 candidate results

- Host: 4,511 GCD/cast/modifier/catalog checks, 1,445 base cooldown checks,
  2,026 icon/UI checks and 12 telemetry cases passed. The 77 encrypted protocol
  scenarios passed; the five affected GCD scenarios also passed after the final
  zero-base ranged recovery correction. These use no shared account.
- Native capture `build/evidence/native-global-cooldowns.csv`: 1,781 received
  samples in one 115-second launch/capture allowance, all eight phases, exact
  timers/modifier/context/counter values and cancellation edges. No failures,
  missing metadata, ambiguity or overflow in this fixture.
- Minimum free memory **42,228 KiB**. Guest frame p50/p95/p99/max **33/34/34/35 ms**;
  maximum UI quads 850 of 2,048. QMP confirms 67,108,864 bytes RAM. This is an
  isolated offline scene; full-world headroom and 30 fps remain separate gates.
- Actual screenshot: `build/evidence/native-global-cooldowns.png`. Dedicated
  native-scale config, copied EEPROM and disposable HDD writes at
  `build/gcd-test-20260916`. Its 247 source hashes, source ZIP and 60 mounted
  outputs were verified after capture. Only owned PID 29860 was quit afterward.
- Normal candidate: `build/candidates/20260916-global-cooldowns`, XBE 1,904,640
  bytes, XISO 497,025,024 bytes, 247 source hashes and 131 output identities.
  Normal staging is MENU/live input with no offline marker. The native build
  has only pre-existing font indentation and .edata merge warnings.

No server, account, saved character, credential or hardware-network changes were
made. The stable hardware kit and accepted checkpoint remain preserved. No
`verification.json` or full-world acceptance is issued for this offline candidate.
