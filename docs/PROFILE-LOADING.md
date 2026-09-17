# Staged character-profile loading

## Implementation

Production character selection now pumps a cancellable profile job. The previous
profile is closed immediately on a race/sex/profile change. Neither its geometry
nor its texture set can render while the new profile is incomplete. Same-profile
appearance changes retain the previous atomic-composition path.

- Pack header, entry table, animated-header validation, WXA metadata, WXLK looks
  catalog, character bindings and item references are prepared in stages.
- Reads are capped at 32 KiB per call and 64 KiB/eight calls per pump, charged to
  the shared AVATAR lane. Scan work is capped at 64 KiB per pump and charged to
  the same frame budget. Look catalog validation handles at most 32 rows per pump;
  duplicate-key comparisons are included as four-byte validation work units.
- Index and texture allocation batches reserve shared allocation grants and check
  the measured 8 MiB free-memory guard. The empty scene takes ownership of the
  validated entry table; it receives the full original header and source identity.
- Contiguous animation headers use a bounded bulk buffer up to 64 KiB, read in
  chunks. Noncontiguous headers retain the validated small-read fallback. The
  entire index remains unpublished until both entries and headers validate.
- A 544-byte idle-family membership table replaces repeated full-index scans for
  geoset and equipment family lookup. It is rebuilt per profile and cleared on
  close. Invalid family references remain rejected.
- New-profile textures are built by the existing staged compositor before the
  first complete model can draw. Production no longer performs the redundant
  synchronous base-atlas read and mip build while opening a profile.
- Synchronous pack/avatar helpers drain the same validators outside a frame scope;
  avatar tooling retains its original initial-atlas contract. These helpers refuse
  to run inside a frame budget without overwriting an existing scene/avatar.
- Failure, cancellation, logout and missing-profile recovery release pending pack,
  catalog and GPU resources. A failed key is retried only when the selection/revision
  changes. Same-race/sex edits during base-profile loading update the desired look
  without restarting the file job; exact legacy-profile requests restart as needed.
- Frontend avatar selection/composition has its own AVATAR budget scope when no
  world scope is active. Other frontend backdrop/title/icon work remains separate.
  This is not yet one global quota across every frontend resource.

## Evidence

Host checks cover missing/invalid profiles, allocation rollback, partial/cancelled
opening, zero budgets, complete header identity, wrong-character suppression,
profile recovery and the existing atomic appearance behavior. Original prepared
appearance hashes and all 80 legal starter outfits remain unchanged.

WXPF0012 is an isolated native fixture using production selection, staged loading,
composition and full-body rendering. It walks through 16 default race/sex profiles
with warrior starter outfits, rapid pending-profile changes, a missing directory,
and recovery. WXQ1 reports phase, read/scan work, pending bytes, selection identity,
completion/cancellation/failure counters and the total/AVATAR frame budget.

The checker requires all 16 profiles to render, a complete scenario cycle, rapid
cancellations, expected missing-profile failure, successful recovery, no stale
identity or partial-profile draws, at least 8 MiB free, and complete nonterminal
main/companion time-frame identities. It does not accept dropped/rejected records
as evidence of a passing quota.

Accepted candidate: `build/candidates/20260917-profile-loading`. Native evidence:
6,832 main /6,831 companions, complete identity coverage, all sixteen profiles,
four cycles, 96 pending cancellations and four expected failures with recovery.
No unexpected failures or stale/partial draws. Minimum free 41,660 KiB; isolated
frame p50/p95/p99/max 33/34/34/35 ms; loading/composition work 1/9/10/17 ms.
Synthetic selection over real assets, without server, replay or physical input.
See the final STATUS.md checkpoint for artifact hashes and remaining gates.

The original companion CSV has an erroneous ten-column performance header, while
all 52 values in each data row are correct. Original files remain unchanged.
Derived `native-profile-loading-recovered` evidence changes only the companion
header and passes strict packet decoding plus full acceptance. Transformation and
body hashes are in `profile-loading-header-recovery.json`. The recorder alias and
socket-to-CSV regression were fixed after capture; only those two Python files
changed, and all XBE/disc/output hashes stayed identical. Candidate source.zip is
final tooling; capture-source.zip and the as-run receipts preserve actual capture
sources. All 11 telemetry tests pass after the fix. This correction is not hidden
as a second native run.

## Remaining scope

File open/seek latency, allocation/free latency, GPU work and rendering are not
fixed-time operations. World startup still uses synchronous wrappers for its
initial world/NPC packs. Other frontend resources need coordinated budgeting.
All-race isolated renders do not complete the original frontend workflow, saved
nondefault creation, live equipment changes, transformed models, missing content,
physical controls, full gameplay or physical hardware release gates.
