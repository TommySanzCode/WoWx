# Bounded world selection

The previous index-publication checkpoint still spent 7–8 ms scoring the world
table after a boundary update. Selection now keeps a private candidate list and
examines at most 2,048 entries per frame, in groups of 32, under the world queue's
cooperative clock. Repeated calls share the same entry quota. Existing read,
validation, index-copy and allocation quotas remain separate and unchanged.

The complete candidate list publishes only after every entry has been scored.
Required/optional ordering, render/collision capacities, placement deduplication
and residency hysteresis use the previous selection rules. Current residents
remain available while a new list is prepared. Pending payloads can continue
against a still-valid published list, within the shared world budget.

Index attach/detach/close invalidates both the candidate snapshot and published
indices. The existing cache remains subject to the pack's source/entry fixups.
Ordinary movement finishes its snapshot and then catches up on the next pass;
relocations of at least 32 units restart from the new position. This avoids
starvation from restarting a whole scan every two units. It introduces bounded
work per frame but not a hard completion-time guarantee: large indices, movement
and competing work can delay a fully current selection. Collision continues to
reject required geometry that is not loaded.

Each scene embeds a fixed 6,692-byte selection workspace. There is no extra heap
allocation or GPU target. Its static footprint is included in native free-memory
measurements. WXS5 appends shared entry use, selection phase/progress/count,
commit/restart counts and source revision identities; older telemetry decodes
with explicit unavailable fields.

Host checks compare the production selector with an independently sorted near
set, including collision/render limits, duplicate placements and retained assets.
They also cover atomic publication, inactive lanes, repeated calls, ordinary
movement, relocation, source detach and cooperative time expiry. Runtime,
index publication, environment, liquids, avatar, preview and streaming checks
pass. An older inactive-lane test expected a payload job immediately; it now
asserts deferred selection with zero scan/read/allocation work. The original
failure log remains. Ten telemetry decoder/identity test groups pass.

## Native result — September 17

Candidate: `build/candidates/20260917-selection`. The same private terrain-water
packs and ten camera cases completed three offline cycles in one native run.
There are 6,898 main / 6,897 exact companion samples, frames 240–7137, with zero
asset/region/fixture errors and all 30 original liquid texture frames submitted.

Across 425 frames of selection work, the maximum entry count is 2,048. The
checker identifies 35 complete multi-frame selections. For the 394 selection
frames without payload reads or a payload job, stream-time p50/p95/p99/max is
0/3/3/4 ms. This includes any remaining stream housekeeping; it is not a separate
CPU profiler sample. With payload work included, selection-frame stream maximum
is 12 ms. World queues exercised 603 cooperative time yields.

Minimum measured free memory is 29,936 KiB (29.23 MiB) in QMP-confirmed
67,108,864-byte RAM at native scale with disposable disk overlays. Guest frame
p50/p95/p99/max is 33/34/34/48 ms; one interval exceeded 40 ms. The first joined
index publication now has 0 ms world-stream work, but still 43 ms drawing and
47 ms total work, followed by the 48 ms interval. Later publications have
20–23 ms drawing. The earlier 7–8 ms whole-selection pause has been distributed;
the first draw/GPU-wait spike remains. This is not full-world or stock-hardware
30 fps acceptance, and emulator first-use costs still need to be distinguished
from work that affects the actual console.

Global-world, selection, shared clock and joined-index checks all pass. Source
ZIP and receipts match 339 sources, 14 fixture outputs and 132 normal outputs.
Only owned xemu PID 70524 was quit after exact executable/command, QMP owner,
mounted-disc and RAM checks. No emulator remains. No screenshots were added,
and no new visual-parity, physical controller or combined gameplay coverage is
claimed. All three delivered hardware kits and the shared server/account remain
unchanged and reserved for the user's physical test.

XBE: 1,998,848 bytes, SHA256
`3c51efc3f52e7e369ebf1b7f98407fac844b832b980f4bdb58a2ac51061c9cce`.
Normal XISO: 499,515,392 bytes, SHA256
`ab58658e9ddcd21a40704e2723b09d0f50f683bb457e700e2548b5b4259eb65d`.
Next: separate CPU submission from draw/GPU waiting, measure moving and combined
actor/player consumers, and retain the full gameplay/interface release gates.
