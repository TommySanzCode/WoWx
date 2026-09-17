# Vanilla scene particle flags

This implementation uses the supplied build-5875 assets and a bounded native C
simulation. It does not use the modern M2 flag names as an authoritative map.
Several public descriptions disagree, including what they call world space.

## Evidence and implemented subset

The user's local `WoW.exe` has SHA-256
`b4756d38ef207c02ed651f4952bd89a70b4857b73a33413339e1b285b28d2dc7`.
The function locations documented by
[WoW 1.12.1 Client Internals](https://github.com/samwhosung/wow-1121-client-internals/blob/main/docs/models.md)
match that executable. Local inspection of the loader and particle branches
established the following mapping. File flags and runtime flags are different.
The generated disassembly stays in private `build/evidence`; no original PC
instructions or executable are incorporated into the Xbox program.

| M2 file bit | Observed build-5875 behavior | Native implementation |
|---|---|---|
| `0x1` | Loader clears material bit 0; render omits the normal stream (`0x70fcf2`, `0x7b4349`) | Existing unlit scene particles |
| `0x8` | Loader clears material bit 1 (`0x70fd01`) | Accepted only together with `0x1`, as in all newly converted Orc/NightElf emitters; retains the unlit path. Its independent normal-bearing material case remains rejected |
| `0x10` | Loader sets runtime `0x100`; plane/sphere spawn skips position/velocity transformation, and draw applies the emitter transform (`0x70faf8`, `0x7b8a9a`, `0x7b8ff0`, `0x7b3e6f`) | Keep particles in emitter-local coordinates and transform their centers at draw preparation; unset particles retain their spawn transform |
| `0x20` | Runtime `0x200`; quad size multiplied by the length of the first bone basis vector (`0x70fb32`, `0x7b51f9`, `0x7b2b9d`) | Opt-in bone scale on size, computed once per emitter each frame |
| `0x100` | Converted to runtime `0x4000` only for sphere type 2 (`0x70fb8e`); selects Z-up velocity in the non-Z-source branch (`0x7b8f79`) | Sphere-only Z-up direction; ignored for planes exactly as the loader does |
| `0x1000` | Runtime `0x2000`; uses transformed XY corners instead of the camera-facing quad (`0x70fb70`, `0x7b3fe1`) | Bone-oriented XY quads; ordinary particles remain camera-facing |

For emitted world-space velocity, the bone's vector transform is retained;
normalizing it afterward would discard the scale present in the original
spawn branch. Local and released particles share bounded gravity integration.
Quad bases use a fixed 64-entry stack array. Particle capacity, GPU vertex
capacity and the WXB file layout are unchanged. There are no frame allocations.

## Validation and limits

Synthetic geometry checks cover a stationary particle following an animated
bone versus remaining at its birth position, opt-in scale, XY versus billboard
orientation, and the sphere-only up flag. The converter rejects unknown bits
and standalone `0x8`. These are independent behavior checks, not native GPU
or pixel comparisons.

Both remaining source scenes export without dropping emitters: Orc has 11,
NightElf has 12. Their skeletal geometry agrees with pinned WoWee/GLM at nine
timestamps. A 30-second host simulation exercises every emitter in each scene,
with finite positions/colors/UVs and no pool drops. All seven prepared scenes
(title plus six racial scenes) pass the same host simulation; races 7 and 8
share Dwarf and Orc respectively.

Native visual acceptance remains pending. Emission shape/distribution, precise
spawn timing, sprite repeat/decay, sorting, fog, camera animation and complete
particle lighting are still fidelity gaps. None of the above establishes
pixel-identical PC effects or physical Xbox timing. Do not extend the mask to
other cases without tracing their behavior and implementing it.
