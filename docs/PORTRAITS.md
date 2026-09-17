# Native unit portraits

## Implementation

Player and creature HUD portraits reuse the live resident world model, animation,
body/hair/clothing textures and equipment. There is no second avatar, offscreen
texture, added GPU allocation or frame heap allocation. The regular actor cache
remains bounded to 32 nearby instances; the selected creature keeps one slot and
is skinned even when outside the world camera. Missing models stay unavailable.

The private PORTRAIT.WPT catalog has 108 records: 16 race/sex model cameras and
92 prepared creature displays. All creatures have an authored M2 portrait camera;
all players have head key-bone 6. WoWee supplies the parser and camera/head-bone
approach. Creature cameras use WoWee's 1.35 distance multiplier. Player focus uses
the head pivot transformed by the same standing-animation sampler used for WXP;
the initial bind-pivot version framed hunched races too high. Player distance is
max(1.0, model height * 0.50), with a 45-degree field of view. Exact PC framing is
still a visual acceptance requirement, especially during non-idle poses.

WXPT v1 is a 16-byte header (magic, version, count, record size) followed by sorted
44-byte entries: key, source, camera XYZ, target XYZ, FOV, near and far planes.
Creature keys are display IDs; players use 0x80000000 | (race << 1) | sex.
The fixed 512-record catalog occupies 22,540 bytes of CPU storage. Loader checks
reject duplicate/unsorted keys, count overflow, truncation, trailing bytes,
non-finite or invalid camera/projection values. The 108-record file is 4,768 bytes.

The 64x64 portrait is an exact pixel-centre circle. It needs 37 grouped clear
rectangles, which fill a dark background and reset depth/stencil only inside the
circle. Stencil references 1 and 2 isolate the two portraits; geometry depth tests
remain active, and stencil testing/writes are restored afterward. World geometry
is submitted first and UI artwork/text last. No terrain depth is reused inside a
portrait. The player and target positions match the supplied FrameXML frame crop.

## Validation scope

The offline fixture uses the production renderer and real prepared model assets,
but synthetic unit health/names. It initializes networking solely for localhost
UDP telemetry. It does not start the authentication worker, load credentials,
connect to a realm or issue character/gameplay commands. Its disc contains no
credentials. PTTEST.BIN = WXPF0001 is required; normal builds reject this marker.

It cycles 16 default player models and all 92 prepared creature displays. This
covers GPU geometry, textures, clipping and switching; it does not prove live
HUD target/equipment synchronization, all appearance variants, death/attack
framing, full-world memory, PC visual parity or physical controller operation.
The stock 64 MiB and 8 MiB measured-headroom requirements still apply. Emulator
timing is separate from later physical Xbox performance.

WXTW adds 10 words to WXTV (1,328 bytes total): catalog ready/count/failures,
player key/draws, target key/draws, mask rectangle count, CPU submission time and
fixture flag. All historical telemetry formats remain readable. Fixture captures
must never pass check-hud.py's world acceptance. check-portrait.py reports their
own model coverage, memory and timing. check-hud.py --portraits --targets remains
the later live-world gate, together with screenshots and a saved-state baseline.

## Reproduction

Run from the repository root, with the pinned toolchains available:

```powershell
cmake --build build/host --target wowx_assetc wowx_portrait_tests -j6
build/host/wowx_assetc.exe --portraits 'game\Data' build/portrait-focused-20260916/PORTRAIT.WPT config/creatures.txt
build/host/wowx_portrait_tests.exe build/portrait-focused-20260916/PORTRAIT.WPT
```

Stage the catalog only into an unmounted development build, build-xbox.ps1, then
write-build-receipt.py. Use a **new** directory for each fixture/capture:

```powershell
python scripts/prepare-portrait-test.py --output build/portrait-test-NEW
scripts/run-portrait-test.ps1 -Fixture build/portrait-test-NEW -Capture build/evidence/native-portrait-NEW.csv -Seconds 300
python tools/check-portrait.py build/evidence/native-portrait-NEW.csv
```

Inspect source/disc receipts and preserve screenshots. After capture completion,
stop only the fixture PID through its QMP endpoint before changing its disc.
Do not replace the stable physical-test package with this candidate.

## Recorded result

The corrected 300-second isolated capture `native-portrait-focused` passes its
coverage/memory checks: 7,190 samples, all 16 player models and 92 prepared
creature displays, zero failures, minimum 42,912 KiB free. Frame p50/p95/p99/max
33/34/76/250 ms; stable-identity subset 33/34/45/242 ms. Submission p95/p99/max
1/1/2 ms. This is not a sustained 30 fps result or full-world/hardware acceptance.
`portrait-focused-dwarf.png` is an actual native screenshot. The initial run and
screenshots remain as evidence of the framing defect that prompted correction.
The final normal XBE also adds invalid-display-ID guards after the fixture's
archived source snapshot; it still requires live-world integration acceptance.
