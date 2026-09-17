# Vanilla action cooldowns

**Current extension:** [Global cooldowns and spell modifiers](GLOBAL-COOLDOWNS.md)
documents WXCD v2, cast lifecycle, haste/ranged timing and bounded family modifier
handling. The v1 sizes and September 16 base-candidate results below are historical;
its original remaining-work list is superseded by the extension's explicit gaps.

This candidate adds cooldown state and action-button feedback. It does not
complete spell usability, all class mechanics, or the PC interface.

## Runtime behavior

- Initial spell cooldowns restore spell, item and shared-category timers on login.
- `SMSG_SPELL_COOLDOWN` applies per-spell lockouts without erasing a longer normal
  recovery. Vanilla has an unpacked caster GUID followed by spell/duration pairs;
  there is **no flags byte**.
- `SMSG_CLEAR_COOLDOWN` clears the named spell's normal/category/lockout entry.
  Packets for other GUIDs never alter the player's timers.
- `SMSG_SPELL_GO` starts base spell/category timers using the supplied Spell.dbc.
  Cast failures and button presses do not start these timers. This starts on the
  server's spell-completion event, with receipt-time latency, not local prediction.
- Item caster GUIDs resolve through the inventory snapshot. The five item spell
  records retain server cooldown/category overrides, including -1 inheritance.
- Cooldown-on-event spells remain shaded until the server's cooldown event;
  initial packets' permanent marker is retained. No invented finite countdown
  is shown for a held timer. Missing metadata is counted without fabricating it.
- Existing controller layers and warrior stance binding resolution select the
  displayed timer. A clockwise radial shade and seconds/minutes/hours text use
  the original icon and the existing bounded UI command buffers. Expiry removes
  the shade. This does not claim that an otherwise unshaded spell is usable.

The production world worker publishes timers under its existing mailbox lock.
The UI clock is sampled inside that lock to avoid querying before a newly
received timer's timestamp. Logout/reconnect resets the character-specific state;
map transfers retain it. Queries and updates allocate no memory. Unsigned elapsed
time handles the 32-bit clock wrapping during an active timer.

## Bounds and assets

`COOLDOWN.WCD` is a private derived asset: WXCD v1, four little-endian uint32 header
words (magic 0x44435857, version, count, record size), then sorted 20-byte records:
spell ID, category, recovery milliseconds, category recovery milliseconds,
attributes. The converter requires the Vanilla 173-column table and fields
0/2/19/20/6. Runtime rejects duplicate IDs, invalid dimensions, out-of-range times,
truncation and trailing bytes before publishing the catalog.

The supplied installation yields **22,357 records / 447,140 resident bytes**,
447,156 bytes on disc. Spell/category metadata is immutable before networking
starts. Opening it retains the required 8 MiB allocation guard. A separate fixed
**45,076-byte** state holds 512 timer entries and 256 five-spell item records.
It is kept outside the already-large WxGame transactional packet copy. The
isolated fixture has its own additional fixed state; normal gameplay does not
use it. The radial lookup occupies 2 KiB; at most 64 shade rectangles per icon
fit within the existing 2,048-quad allocation. Active timer overflow is counted;
active entries are never silently evicted to make room.

Packets are completely validated before publishing their state. The largest
temporary list is 10 KiB, independent of WxGame's stack copy. Relevant primary
references in the pinned vMaNGOS tree are Player.cpp's SendInitialSpells,
AddCooldown and LockOutSpells, Server/Packets/Spell.cpp, and
Spells/SpellCastTargetsInfo.cpp. The pinned WoWee classic DBC layout supplies the
recovery/attribute field indices. The newer WoWee flags-byte packet parser was
not used for Vanilla's wire format.

## Reproduction and acceptance

```powershell
cmake --build build/host --target wowx_assetc wowx_cooldown_tests wowx_icon_tests wowx_world_fixture
build/host/wowx_assetc --cooldowns 'game\Data' build/NEW-COOLDOWN.WCD
build/host/wowx_cooldown_tests build/xbox/COOLDOWN.WCD
build/host/wowx_icon_tests build/xbox/ICONS.WIC build/xbox/INTERFACE.WUI
python tests/world_protocol.py
python tests/preview_telemetry.py
```

After staging the catalog, building, recording a source/output receipt and
archiving those sources, an isolated native fixture can be prepared with
`scripts/prepare-portrait-test.py --cooldowns --output build/NEW-FIXTURE` and run
with `scripts/run-portrait-test.ps1`. The exact marker is `WXPF0003`; normal build
staging rejects any PTTEST.BIN. The fixture starts only UDP telemetry, before the
auth worker, and contains no credentials. Eight deterministic steps inject
initial, lockout, clear, delayed-event and spell-go packets through the production
cooldown parser. The fixture's simulated timer clock advances 33 ms/frame;
reported frame timing still uses the guest clock. It is injected state, not an
automated controller replay. `tools/check-cooldowns.py` checks every received
timer against that sequence. Actual screenshots assess visible rendering.

WXTY telemetry is 360 words / 1,440 bytes and appends catalog/state metrics, eight
displayed timers, held mask and fixture phase. Older versions remain decodable.

## Remaining parity gates

Base DBC prediction does **not yet apply talent/aura spell modifiers**, global
cooldowns, ranged weapon attack speed, spell-category flags, cast-start GCD or
cancellation rules. Authoritative lockouts/initial timers are supported, but
baseline predictions can differ for modified spells until those systems are
implemented. Item-query cache eviction can leave a previously seen item without
metadata; this is diagnosed rather than guessed. Pets/other controlled casters,
all class/form rules, full original action-bar layout, range/resource/usability,
tooltips, charge/count feedback and live-world integration acceptance remain.
Server acceptance continues to decide whether a requested action actually casts.
The hardware kit and previously accepted world checkpoint remain unchanged.


## September 16 evidence

- Host cooldown/catalog checks: 1,429; icon/UI checks: 2,024; telemetry cases: 11;
  encrypted world protocol scenarios: 72. Logs use the `cooldowns-` prefix in
  build/evidence.
- One 115-second isolated native launch/capture allowance produced 1,850 samples;
  all eight packet phases and every received timer passed the exact checker.
- Measured free memory: minimum 43,048 KiB. Guest-clock frame p50/p95/p99/max:
  33/34/34/35 ms. Maximum UI quads: 585. No runtime failures or metadata overflow.
- Actual screenshot: build/evidence/native-cooldowns-charge.png. Fixture source
  ZIP and mounted outputs: build/cooldowns-test-20260916. Its project-owned xemu
  was quit after capture; shared server and saved characters were untouched.

These measurements exclude world streaming/crowds and do not establish physical
hardware performance or controller transport. No live-world checkpoint is issued.


Normal candidate archive: build/candidates/20260916-cooldowns. Final XBE is
1,896,448 bytes; XISO is 496,173,056 bytes. The final rebuild removes two new
compiler warnings (explicit zero initializer and indentation). Its four loaded
XBE sections are byte-identical to the captured executable; all six changed
bytes belong to XBE/PE/certificate timestamp fields. Both hashes and the section
comparison are retained in build/evidence/cooldowns-xbe-comparison.json. Final
incremental build has only the known .edata linker warning. These identities do
not turn the offline fixture into a live-world or hardware checkpoint.
