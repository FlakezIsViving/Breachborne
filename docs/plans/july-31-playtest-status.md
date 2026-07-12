# July 31 Playtest Status

Companion tracker for `docs/plans/july-31-playtest-vfx-network-plan.md`.
Update this file at the end of every workday and whenever a hard gate changes state.
Manual runtime matrix: `docs/plans/july-31-manual-vfx-acceptance.md`.

Last updated: July 12, 2026.

## Current headline

- Current gate: July 13 Listen-Server Combat
- Overall status: YELLOW
- Primary blocker: execute the four-session owner/observer ability acceptance matrix.
- PureLogic baseline: 44 passed, 0 failed on July 12.
- GameSystems baseline: 28 passed, 0 failed on July 12, including six primitive-VFX geometry/lifecycle checks. The previous crash is fixed.
- Build baseline: Editor, Game, and source-engine Server targets succeeded on July 12.
- Latest network evidence: normal and 100 ms/2% loss packaged matches grant and activate all five inputs for hunters 1-6; separate close-range matches prove authoritative LMB health loss for all six. Core death/wisp/healing/revive also passes; non-LMB damage/CC, persistent-death cleanup, and visual acceptance remain pending.
- Wisp lifecycle: repeated multiplayer spawn/revive tests pass; invisible WidgetTree implementation replaced with direct Slate painting and first-paint diagnostics, visual retest pending.
- Ability range indicators: shared owner-only hover and active previews cover all six hunters and F/G powers; visual/network retest pending.
- GameplayCue networking: targeted cue scan paths configured; Hudson minigun cues use one cosmetic multicast per shot with local primitive rendering and no per-shot debug RPCs; live firing overflow retest pending.
- VFX foundation: template enum, palette/parameter schema, numeric palettes, budgets, content roots, and primitive fallback contract locked; Niagara master assets remain unbuilt.
- Ghost VFX: replicated primitive warning, projectile, trail, beam, impact, active-zone, and passive-pulse fallbacks compile; live owner/observer readability remains unverified.
- Eluna VFX: replicated primitive projectile, root/heal zone, dash, passive, and revive-channel fallbacks compile; live owner/observer readability remains unverified.
- Kingpin VFX: replicated primitive wedge, tether, trail, impact, chain, shell, and passive fallbacks compile; live owner/observer readability remains unverified.
- Latest main package summary: current source-toolchain client/server archives pass the freshness-aware candidate verifier: executables, TestMap, 17/17 Hudson cues, handshake 2/2, normal/impaired roster smoke 6/6, authoritative LMB hits 6/6, reconnect attempt, death/wisp/healing/revive, and zero critical/cue-overflow findings.
- Networking/performance cleanup: removed the final legacy DrawDebug multicast API and callers; disabled no-op observer ticks on server-only gameplay actors; made Grappling Hook collision/attachment/destruction server authoritative while replicating latch state for observer visuals; guarded replicated burst fallbacks against pre-initialization world-origin flashes.
- Manual-test tooling: packaged server plus 1-4 visible clients now launch as one recorded session with isolated logs, exact PIDs, safe cleanup, and automatic critical/cue-overflow review under `Saved/Logs/InteractivePlaytest`.
- Source control: latest green automated combat checkpoint is `f38164f`; baseline `d425cd9`, full-roster smoke `9fcec64`, impaired-network/reconnect `765f27a`, death/wisp `e09646a`, and healed-revive `9b92d0a` are also committed.

## Gate board

Use `RED`, `YELLOW`, or `GREEN`. A date passing does not make a gate green.

| Date | Gate | Status | Evidence | Blocker/next action |
| --- | --- | --- | --- | --- |
| Jul 12 | Green baseline | GREEN | Checkpoint `d425cd9`; Editor/Game/Server build; PureLogic 44/44; GameSystems 28/28; source and packaged handshakes 2/2 | Complete |
| Jul 13 | Listen-server combat | YELLOW | Packaged smoke selects/spawns all hunters, activates all five inputs, and proves LMB damage 6/6 | Non-LMB damage/CC plus interactive movement/aim pass pending |
| Jul 14 | Dedicated server | YELLOW | Roster activation, authoritative LMB damage 6/6, replacement transport reconnect, and death/wisp/revive pass in packaged dedicated runs | Non-LMB damage/CC agreement, persistent-ability death cleanup, and disconnected-player restoration pending |
| Jul 15 | Packaged networking | YELLOW | Freshness-aware verifier PASS: executable/map/cue, handshake, normal/impaired roster, LMB hits 6/6, reconnect-attempt, and death/wisp/revive | Deliberate visual acceptance and second-machine direct-IP pending |
| Jul 16 | VFX foundation lock | YELLOW | Cue roots, template enum, palettes, parameter schema, budgets, fallbacks, and narrow cue always-cook root locked; all 17 Hudson cue packages present in client IoStore | Eight Niagara master assets and live ability-level cue invocation pending |
| Jul 17 | Ghost VFX | YELLOW | Replicated primitive fallbacks cover LMB/RMB/Shift/Q/R/passive; Editor build and automation green | Two-client readability, timing, cleanup, and Low/Medium pass required |
| Jul 18 | Eluna VFX | YELLOW | Replicated primitive fallbacks cover LMB/RMB/Shift/Q/R/passive; exact zone geometry and server-only Q/R resolution compiled | Two-client readability, timing, cleanup, and Low/Medium pass required |
| Jul 19 | Kingpin VFX | YELLOW | Replicated primitive wedge/beam/burst fallbacks cover LMB/RMB/Shift/Q/R/passive; Editor build and automation green | Two-client readability, moving-hook presentation, cleanup, and Low/Medium pass required |
| Jul 20 | Hudson VFX/internal test | YELLOW | Placeholder cue assets audited; code fallbacks cover LMB/RMB/Shift/Q/R/passive; Editor/Game/Server, automation, and 2/2 handshake pass | Two-client firing/readability, release, cleanup, and overflow retest required |
| Jul 21 | Crysta VFX | YELLOW | Primitive fallbacks cover all abilities; empowered LMB observer state, true Q radius, impacts/detonations, X/V beams compile | Two-client readability, mark visibility, path, cleanup, and Low/Medium pass required |
| Jul 22 | Void VFX | YELLOW | Primitive fallbacks cover all abilities; charged/empowered observer state, true zone radii, swap endpoints, and bursts compile | Two-client wall-pierce/cone/swap/recast/readability and cleanup pass required |
| Jul 23 | Hard feature/VFX freeze | RED | Not reached | Full-roster integration required |
| Jul 24 | Playtest 1 | RED | Not run | Requires frozen packaged candidate |
| Jul 25 | Repair day 1 | RED | Not reached | Driven by Playtest 1 findings |
| Jul 26 | Redo gate | RED | Not reached | P0/P1 and readability repair |
| Jul 27 | Lag/loss playtest | YELLOW | Automated packaged activation PASS for hunters 1-6 under confirmed `PktLag=100` / `PktLoss=2` | Manual Hudson sustained fire, persistent-zone overlap, and visual agreement remain |
| Jul 28 | Performance/cleanup | RED | Not run | Low/Medium and lifecycle pass required |
| Jul 29 | Dress rehearsal | RED | Not run | Exact playtest topology required |
| Jul 30 | Release candidate freeze | RED | Not built | Final package/distribution verification |
| Jul 31 | Playtest | RED | Not reached | Use verified July 30 RC |

## Ability verification matrix

Mark each cell `-`, `PASS`, or `FAIL`. `Network` means owner, server, and observer agree.

| Hunter | LMB | RMB | Shift | Q | R | Passive | Network | Cleanup |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Ghost | - | - | - | - | - | - | - | - |
| Eluna | - | - | - | - | - | - | - | - |
| Kingpin | - | - | - | - | - | - | - | - |
| Hudson | - | - | - | - | - | - | - | - |
| Crysta | - | - | - | - | - | - | - | - |
| Void | - | - | - | - | - | - | - | - |

## Active issue log

| ID | Severity | Area | Description | Owner | Status | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| BB-PT-001 | P0 | Automation | GameSystems access violation at test line 449 | Codex | Resolved | GameSystems 28/28; no crash/ensure on Jul 12 |
| BB-PT-002 | P1 | Packaging | Packaged candidate must match current source | Codex | Resolved | `Scripts/Playtest/VerifyPlaytestCandidate.ps1` PASS; evidence in `Builds/PlaytestCandidateVerification.txt` |
| BB-PT-003 | P1 | Networking | Full ability/death/dedicated/package matrix untested | User + Codex | Partially automated; non-LMB damage/CC, death cleanup, visual, and reconnect-restoration matrix open | Normal/impaired roster plus authoritative LMB damage PASS 6/6; death/wisp/revive PASS; four-session manual matrix ready |
| BB-PT-004 | P2 | GameplayCues | No targeted GameplayCueNotifyPaths configured | Codex | Resolved | `/Game/GameplayCues` configured; clean Jul 12 editor startup initialization |
| BB-PT-005 | P2 | Environment | UDP Messaging cannot bind 25.18.80.222 | Unassigned | Open | Jul 12 log; GameNetDriver still worked |
| BB-PT-006 | P0 | Wisp | PvP death did not spawn or possess a wisp because GameMode lifecycle wiring was absent | Codex | Resolved | User retest plus `Saved/Logs/Breachborne-backup-2026.07.12-00.26.01.log`: three successful spawn/possession/revive cycles |
| BB-PT-007 | P1 | VFX/Networking | Rapid Hudson LMB exceeded the per-net-update GameplayCue multicast limit; an impact cue was dropped | Codex | Fix compiled; live retest pending | Fire/Impact/Heal use one batched multicast per shot; fallback rendering is local; legacy DrawDebug multicast API removed; Editor/Game/Server PASS |
| BB-PT-008 | P1 | Wisp UI | Wisp widgets constructed and bound on every client but rendered no bars because the tree was created after `RebuildWidget` | Codex | Fix compiled; visual retest pending | Jul 12 repeated-session log; no paint path existed |
| BB-PT-009 | P1 | Ability UI | Primitive range previews need a six-hunter owner/observer readability pass | Codex | Fix compiled; visual retest pending | `docs/ABILITY_RANGE_INDICATORS.md` manual matrix |
| BB-PT-010 | P1 | Networking | Launcher Game client and source-built Server had incompatible network version hashes | Codex | Resolved | Same-toolchain Game/Server plus `TestHeadlessDedicatedHandshake.ps1`: 2/2 joins PASS |
| BB-PT-011 | P1 | Ghost VFX | Primitive fallbacks are implemented but have not been observed from owner and remote-client views | Codex | Fix compiled; visual retest pending | Editor PASS; PureLogic 44/44; GameSystems 27/27 on Jul 12 |
| BB-PT-012 | P1 | Eluna VFX | Root/heal zone observer state and Q/R authority leaks required repair; visual acceptance remains unrun | Codex | Fix compiled; visual retest pending | Replicated radii/toss movement, server-only Q spawn and R resolution; Editor PASS; PureLogic 44/44; GameSystems 27/27 |
| BB-PT-013 | P1 | Kingpin VFX | Cone gameplay was owner-debug-only or invisible to observers; full visual acceptance remains unrun | Codex | Fix compiled; visual retest pending | Replicated wedge actor plus per-ability fallbacks; Editor PASS; PureLogic 44/44; GameSystems 27/27 |
| BB-PT-014 | P1 | Crysta/Void VFX | Empowered/charged mesh state and several zone radii did not match on observers | Codex | Fix compiled; visual retest pending | Replicated visual flags, corrected radius scaling, bounded resolution effects; full source build and handshake PASS |
| BB-PT-015 | P1 | Roster | Ghost and Eluna lacked native hunter definitions, so missing/unloadable DataAssets made their IDs unselectable on dedicated runtime | Codex | Resolved | Native definitions now cover IDs 1-6; source and packaged full-roster ability smoke PASS |
| BB-PT-016 | P1 | Networking | A client can reconnect to the server during Playing but is not restored to the disconnected hunter | Codex | Open; restart-match fallback documented | `Saved/Logs/PackagedReconnect/20260712-201340`: initial gameplay PASS, transport reconnect PASS, replacement assigned late-join spectator, 0 critical/cue-overflow findings |
| BB-PT-017 | P1 | Death/Wisp | Packaged lethal-damage-to-wisp lifecycle lacked deterministic evidence | Codex | Resolved for core lifecycle; visual bars/proximity priority and persistent-ability death cleanup remain manual | `Saved/Logs/PackagedDeathWisp/20260712-212520`: GAS depletion, wisp possession, capped healing revive, server hunter restore, victim-client repossession PASS; 0 critical/cue overflow |
| BB-PT-018 | P1 | Automation/Drop | Impaired smoke waited for replicated ability specs before resetting drop movement, allowing one client to remain stuck gliding and never prepare | Codex | Resolved | Server preparation now occurs as soon as the pawn exists; exact source regression PASS and packaged impaired roster PASS 6/6 at 100 ms/2% loss |

## Daily updates

### 2026-07-12

- Gate due: Green baseline
- Status: YELLOW
- Completed: fixed dangling test-lambda state; implemented replicated Day/Night state;
  compiled Editor, Game, and Server targets; restored server-authoritative wisp spawn,
  possession, revive, execute, decay-to-deathbox, and deathbox revive wiring
- Build result: Editor PASS; Game PASS; Server PASS using source-built UE 5.7.4
- PureLogic result: 44 passed, 0 failed, no crash/ensure
- GameSystems result: 28 passed, 0 failed, no crash/ensure; six primitive-VFX geometry/state/lifecycle checks included
- Wisp fix validation: Editor/Game/Server PASS; PureLogic 44/44; GameSystems 27/27;
  live 2v2 spawn/possession/revive test PASS; two-bar UI/contest visual retest pending
- Eluna R visual: corrected approximately 50x cylinder length scaling error; replaced with replicated narrow translucent tether, subtle core, endpoint halos, and restrained lights; visual retest pending
- Ability range indicators: added LMB HUD slot, hover max-range circles, active aim/landing/area previews, Void Q center clamping behavior, Tactical Nuke targeting integration, and metadata for every active hunter ability and F/G power
- Plan audit: Crysta/Void implementation checklists now distinguish source-complete work from unverified owner/server/observer acceptance tests
- GameplayCue reliability: configured `/Game/GameplayCues`; batched high-frequency Hudson Fire/Impact/Heal cues into one multicast per shot; Editor/Game/Server PASS, PureLogic 44/44, GameSystems 27/27; live overflow retest pending
- Dedicated networking: initial mixed-toolchain smoke correctly failed as `OutdatedClient`; rebuilt Game with the source-engine toolchain, then completed two clean welcomes and joins; added repeatable `Scripts/Networking/TestHeadlessDedicatedHandshake.ps1`
- Packaged networking: produced fresh Development client/server archives with source UAT; packaged server welcomed two packaged clients; added `Scripts/Playtest/TestPackagedHeadlessHandshake.ps1` to capture and validate repeatable evidence without orphaning launcher-child processes
- VFX foundation: added `EBBVFXTemplateType`, palette and parameter-default data to `UBBAbilityVisualSet`; locked exact colors, `User.*` semantics, template budgets, paths, and primitive fallbacks in `docs/VFX_FOUNDATION.md`; retained `/Game/Hunters` scan compatibility for Hudson cues
- Ghost VFX source pass: added replicated primitive beam/burst actors and fallbacks for projectile impacts, grenade detonation, dash trail, laser line/hits, napalm warning/active zone, and passive reset pulse; delayed napalm damage until its warning ends; Editor PASS, PureLogic 44/44, GameSystems 27/27; two-client visual acceptance remains pending
- Eluna VFX source pass: added crescent/sticky projectile styling and impacts, replicated exact root/heal zone radii, healing pulses/final burst, toss path and replicated movement, dash/refund pulses, passive heal/carry/drop pulses, and R completion burst; removed duplicate predicted Q zone spawning and made R heal/revive server-authoritative; Editor PASS, PureLogic 44/44, GameSystems 27/27; two-client visual acceptance remains pending
- Kingpin VFX source pass: added reusable replicated sector-outline actor; covered LMB swipe/impact/chain, RMB hook/latch, charge and dash paths, Shift collisions, Q geometry/impacts, both R shell cones/impacts, and passive response; Editor PASS, PureLogic 44/44, GameSystems 27/27; two-client visual acceptance remains pending
- Hudson VFX source pass: confirmed existing cue Blueprints contain no referenced visual assets; rendered minigun primitives locally inside the single batched cue multicast, removed high-frequency debug RPCs from LMB/Shift/R, added replicated RMB progress outline, made barbed-wire meshes visible and colored, and added bounded Shift/R/passive fallbacks; Editor/Game/Server PASS, PureLogic 44/44, GameSystems 27/27, source dedicated handshake 2/2 PASS at `Saved/Logs/DedicatedHandshake/20260712-055506`; live firing/release/overflow acceptance remains pending
- Crysta/Void VFX source pass: replicated empowered Crysta LMB and charged/empowered Void projectile appearance; corrected Crysta Q, Void Q, Void Shift, and Void R primitive cylinders from half-size to exact gameplay radius; added Crysta impacts/detonations and Void source/destination swap zones, impacts, puddle burst, swap burst, and singularity implosion; Editor/Game/Server PASS, PureLogic 44/44, GameSystems 27/27, current-source handshake 2/2 PASS at `Saved/Logs/DedicatedHandshake/20260712-060313`; live ability acceptance remains pending
- Cooked cue discovery: archive audit initially found no Hudson cue packages; added `/Game/Hunters/Hudson/Cues` to `DirectoriesToAlwaysCook`, rebuilt the client, confirmed all 17 cue `.uasset` files in `Breachborne-Windows.utoc`, and reran packaged handshake 2/2 PASS at `Saved/Logs/PackagedHandshake/20260712-184420`
- Networking/performance cleanup: removed the remaining DrawDebug multicast calls and dead RPC API; disabled server-only actor ticks on observer clients for Flux Snare, Moonlight Zone, Barbed Wire, Void Snap, Void Swap, Singularity, Glide Bot, Test Ally, and Train Carriage; made Grappling Hook collision/range/latch/destruction server authoritative and replicated latch state for observers; prevented uninitialized replicated burst actors from flashing at world origin; Editor/Game/Server PASS, PureLogic 44/44, GameSystems 28/28, source handshake 2/2 PASS at `Saved/Logs/DedicatedHandshake/20260712-191425`
- Current package: source-toolchain client and server archives rebuilt at 21:18 after LMB hit and impaired-readiness work; packaged handshake 2/2 PASS at `Saved/Logs/PackagedHandshake/20260712-211850`; interactive visual combat and second-machine evidence remain pending
- Candidate verifier: PASS for source freshness, executables, cooked TestMap, Hudson cues 17/17, handshake 2/2, normal/impaired roster 6/6, authoritative LMB hits 6/6, reconnect attempt, death/wisp/revive, and zero critical packaged-log findings; summary at `Builds/PlaytestCandidateVerification.txt`
- Interactive acceptance tooling: upgraded `StartPackagedLocalSmoke.ps1` to launch a hidden packaged server and side-by-side clients with isolated timestamped logs and exact PID metadata; added safe stop plus automatic review scripts; parser checks PASS and reviewer PASS against `Saved/Logs/PackagedHandshake/20260712-191726` with 2 joins, zero critical matches, and zero cue-overflow matches
- Source-control checkpoint: committed the green six-hunter VFX/network/wisp/range/tooling baseline as `d425cd9`; Jul 12 gate is GREEN
- Automated roster gameplay: added opt-in `-BBAbilitySmoke` flow plus packaged single-pair/full-roster scripts; fixed nondeterministic lobby-owner startup, isolated activation checks from enemy CC, and added native Ghost/Eluna definitions; source and packaged runs PASS for hunter pairs 1/2, 3/4, and 5/6 with all initial inputs successful and cleanup complete
- Candidate freshness: verifier rejects archives older than source/config/content/plugin inputs; refreshed main package and all required post-build evidence now PASS
- Network impairment: server/client `PktLag=100` plus `PktLoss=2` confirmed; activation lifecycle PASS for hunters 1-6 at `Saved/Logs/PackagedNetworkImpairment/20260712-212122`, `20260712-212221`, and `20260712-212320`; manual sustained-fire/zone-overlap visual checks remain open
- Death/wisp automation: shared GAS lethal damage, HealthSet depletion, authoritative/client wisp possession, server-timed capped healing, revive at 50% health, and victim-client hunter repossession PASS at `Saved/Logs/PackagedDeathWisp/20260712-212520`
- LMB hit automation: `TestPackagedFullRosterHitSmoke.ps1` repositions opponents after lifecycle smoke and requires positive authoritative health loss from every hunter; PASS 6/6 at `Saved/Logs/PackagedHitSmoke/20260712-212609`, `20260712-212703`, and `20260712-212757`
- Exploratory Ghost/Eluna session: `Saved/Logs/InteractivePlaytest/20260712-200148` stopped and reviewed PASS with 2 joins and zero critical/cue-overflow findings; no visual checklist rows accepted because owner/observer results were not deliberately recorded
- Two-client result: roster activation, authoritative LMB damage, and core death/wisp/revive are automated; replacement reconnect reaches the server but match-state restoration is unsupported; manual visuals, aim/movement, non-LMB damage/CC, and death cleanup remain
- Packaged/dedicated result: Editor/Game/Server build PASS; final PureLogic 44/44 and GameSystems 28/28; refreshed main candidate and all automated post-build gates PASS; second-machine direct-IP remains pending
- Evidence and log paths: `Saved/Logs/Breachborne.log` and
  `Saved/Logs/Breachborne-backup-2026.07.11-23.47.36.log`
- Open P0/P1 issues: disconnected-player restoration, unexecuted manual network/visual matrix, plus Hudson, wisp UI, and range-indicator live retests
- Scope cut triggered: none
- Tomorrow's single target: run deliberate Ghost/Eluna owner-observer acceptance and record all 12 rows

## Daily update template

Copy this section for each day.

### YYYY-MM-DD

- Gate due:
- Status: RED / YELLOW / GREEN
- Completed:
- Build result:
- PureLogic result:
- GameSystems result:
- Two-client result:
- Packaged/dedicated result:
- Evidence and log paths:
- Open P0/P1 issues:
- Scope cut triggered, if any:
- Tomorrow's single target:

## Decision log

| Date | Decision | Reason | Consequence |
| --- | --- | --- | --- |
| Jul 12 | Direct IP; no matchmaking | Protect deadline | Dedicated preferred, listen accepted |
| Jul 12 | Feature/VFX freeze Jul 23 | Preserve one-week repair window | Generic/primitive fallback after freeze |
| Jul 12 | Shared animations accepted | Animation workload is not viable | VFX carries ability readability |
| Jul 12 | Low/Medium first | Playtest reliability and broad hardware | No High/Epic polish before deadline |

## Calendar configuration

- Calendar: primary Google Calendar
- Events: July 12 through July 31, one hard gate per day
- Event time: 20:00-20:30 Europe/Bucharest
- Reminders: popup 12 hours and 1 hour before each gate
- Events are private and transparent; they are deadline reviews rather than busy blocks
