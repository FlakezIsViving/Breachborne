# July 31 Playtest Status

Companion tracker for `docs/plans/july-31-playtest-vfx-network-plan.md`.
Update this file at the end of every workday and whenever a hard gate changes state.
Manual runtime matrix: `docs/plans/july-31-manual-vfx-acceptance.md`.

Last updated: July 15, 2026.

## Current headline

- Current gate: manually retest the three repaired Eluna behaviors on the July 15 03:07 package
- Overall status: YELLOW
- Primary blocker: focused automation is green. Human owner/observer confirmation remains for Eluna
  RMB latch stability, Shift wisp refund, and healing-started resurrection persistence.
- PureLogic baseline: 44 passed, 0 failed on July 15.
- GameSystems baseline: 53 passed, 0 failed on July 15, including persistent-ASC match reset,
  stable cooldown specs, Crysta mark removal and Shift charges, and Void geometry, collision, swap,
  Singularity lifecycle contracts, Eluna Q carrier rules, and Eluna R completion ownership.
- Build baseline: Editor target and packaged Game/Server targets succeeded on July 15.
- Latest network evidence: the July 15 01:26 package passes normal and 100 ms/2% loss activation for hunters 1-6, authoritative LMB health loss for all six, and 24/24 hunter-specific RMB/Shift/Q/R outcome contracts both normally and at 100 ms/2% loss. Core death/wisp/healing/revive and cleanup pass in both victim directions; wisp priority and Eluna R revive pass 10/10. Reconnect restores the same hunter/team/slot and exact recorded state. Four-client evidence proves 4/4 transport, exact 2v2 mapping/lifecycle, 20/20 primary activations, and 8/8 releases. Every fresh packaged review reports zero critical or GameplayCue-overflow findings. Visual acceptance remains pending.
- Wisp lifecycle: the final focused package passes 12/12 at `Saved/Logs/PackagedWispRules/20260715-030814`, including one-heal persistence after the source expires, enemy cancellation, Shift-path pickup, passive pickup, CC drop, carried protection, and Eluna R revive/re-possession. Manual healing continuity still requires confirmation.
- Ability range indicators: shared owner-only hover and active previews cover all six hunters and F/G powers; visual/network retest pending.
- GameplayCue networking: targeted cue scan paths configured; Hudson minigun cues use one cosmetic multicast per shot with local primitive rendering and no per-shot debug RPCs. Final normal, impaired, hit, death/revive, reconnect, four-client, and 24-contract packaged reviews report zero cue-overflow findings; manual ten-second Hudson firing/readability remains pending.
- VFX foundation: template enum, palette/parameter schema, numeric palettes, budgets, content roots, and primitive fallback contract locked; Niagara master assets remain unbuilt.
- VFX asset audit: `Scripts/Playtest/AuditVfxFoundation.ps1` distinguishes fallback readiness
  from authored completion and writes `Builds/VfxFoundationAudit.txt`; the current expected result
  is fallback-ready with `0/8` Niagara masters, not a green authored-assets claim.
- Ghost VFX: LMB, RMB, Q, R, and passive cooldown reset are manually accepted. Shift remains readable
  but fails the strict synchronized-movement row because unfocused local windows jitter.
- Eluna VFX: LMB is accepted. R full-channel revive and range cancellation work, while remaining
  cancellation cases are pending. Q following and carried-wisp smoothness passed the focused retest;
  RMB latch, Shift refund, and Q-driven resurrection continuity are repaired and await manual retest.
- Kingpin VFX: replicated primitive wedge, tether, trail, impact, chain, shell, and passive fallbacks compile; live owner/observer readability remains unverified.
- Latest main package summary: the July 15 01:26 source-toolchain client/server package passes the
  July 15 01:42 freshness-aware verifier: executables, TestMap, 17/17 Hudson cues, handshake 2/2,
  transport 4/4, exact 2v2 mappings/lifecycles 4/4, normal/impaired roster smoke 6/6, LMB hits
  6/6, non-LMB outcomes 24/24 normally and impaired, persistent match reset, same-hunter reconnect,
  both death/wisp directions, wisp rules 10/10, and zero critical/cue-overflow findings.
  The package includes the repairs from the July 15 manual session and the Hudson Q authoritative-
  aim correction found during impaired outcome regression.
- Focused repair package: client/server rebuilt at 03:00/03:07. Wisp rules pass 12/12 at
  `Saved/Logs/PackagedWispRules/20260715-030814`; Eluna RMB/Shift/Q/R authoritative outcomes pass
  4/4 at `Saved/Logs/PackagedOutcomeSmoke/20260715-030959`; both reviews have zero critical and cue-
  overflow findings. The complete six-hunter normal/impaired verifier has not been rerun on this
  newer focused package, so the 01:26 candidate remains the latest full-matrix evidence.
- Networking/performance cleanup: removed the final legacy DrawDebug multicast API and callers; disabled no-op observer ticks on server-only gameplay actors; made Grappling Hook collision/attachment/destruction server authoritative while replicating latch state for observer visuals; guarded replicated burst fallbacks against pre-initialization world-origin flashes.
- Manual-test tooling: packaged server plus 1-4 visible clients now launch as one recorded session with isolated logs, exact PIDs, safe cleanup, and automatic critical/cue-overflow review under `Saved/Logs/InteractivePlaytest`.
- Ghost/Eluna manual topology: a dedicated three-client wrapper assigns an Eluna owner, allied
  Hudson helper, and opposing Ghost so all 12 rows are executable; it records exact roles and
  stop/review instructions without opening windows until the user confirms readiness.
- Remaining pair topology: Kingpin/Hudson now has a three-client wrapper for chain/multi-target
  checks; Crysta/Void has a two-client wrapper that explicitly records the missing cooked marked-
  prop fixture instead of allowing deterministic coverage to be mistaken for visual acceptance.
- Direct-IP readiness: host/client launchers now preserve isolated logs, executable hashes, route
  arguments, and process metadata; `docs/plans/july-31-direct-ip-acceptance.md` defines the exact
  server plus host-client plus remote-client topology, safe remote stop/reconnect, combined log
  review, and six stable `DI` gates. The isolated reviewer self-test passes 4/4; execution still
  requires the user and a second machine.
- Distribution readiness: `Scripts/Packaging/PreparePlaytestDistribution.ps1` verifies the
  candidate before staging, preserves runnable relative paths and acceptance evidence, and emits
  per-file SHA-256 manifests for client/server bundles. The bundled
  `Scripts/Playtest/VerifyPlaytestDistribution.ps1` rejects missing, changed, duplicate,
  out-of-root, and unexpected files after transfer. Manifest generation and verification are
  Windows PowerShell 5.1-compatible and pass an isolated 8/8 behavioral self-test. July 15
  `-IncludeServer -ValidateOnly` preflight passes the current client/server inputs without copying
  or archiving files. Final staging remains deferred until manual repairs and release-candidate
  freeze are complete.
- Source control: latest committed planning checkpoint is `8e6a715`; the verified July 13 lifecycle/tooling batch is currently uncommitted. Preserve it when taking over.

## Gate board

Use `RED`, `YELLOW`, or `GREEN`. A date passing does not make a gate green.

| Date | Gate | Status | Evidence | Blocker/next action |
| --- | --- | --- | --- | --- |
| Jul 12 | Green baseline | GREEN | Checkpoint `d425cd9`; Editor/Game/Server build; PureLogic 44/44; GameSystems 28/28; source and packaged handshakes 2/2 | Complete |
| Jul 13 | Listen-server combat | YELLOW | Packaged smoke selects/spawns all hunters, activates all five inputs, proves LMB damage 6/6, and proves RMB/Shift/Q/R outcomes 24/24 | Interactive movement/aim and owner/observer visual pass pending |
| Jul 14 | Dedicated server | YELLOW | Fresh package: roster activation, exact 2v2 mapping/lifecycle 4/4, LMB 6/6, non-LMB 24/24 normal and impaired, reconnect, both death/revive directions, and wisp rules 10/10 | Deliberate 2v2 movement/combat and ability-specific visual cleanup pending |
| Jul 15 | Packaged networking | YELLOW | Jul 15 freshness-aware verifier PASS: executable/map/cue, 2/2 and 4/4 transport, exact 2v2 mapping/lifecycle, normal/impaired roster/outcomes 24/24, reconnect, death/revive, and wisp rules 10/10 | Focused Ghost/Eluna visual retest, deliberate 2v2 combat, and second-machine direct-IP pending |
| Jul 16 | VFX foundation lock | YELLOW | Cue roots, template enum, palettes, parameter schema, budgets, fallbacks, and narrow cue always-cook root locked; all 17 Hudson cue packages present in client IoStore | Eight Niagara master assets and live ability-level cue invocation pending |
| Jul 17 | Ghost VFX | YELLOW | Replicated primitive fallbacks cover LMB/RMB/Shift/Q/R/passive; Editor build and automation green | Recorded Ghost/Eluna three-client readability, timing, cleanup, and Low/Medium pass required |
| Jul 18 | Eluna VFX | YELLOW | Replicated primitive fallbacks cover LMB/RMB/Shift/Q/R/passive; exact zone geometry and server-only Q/R resolution compiled | Three-client owner/observer/helper readability, timing, cleanup, and Low/Medium pass required |
| Jul 19 | Kingpin VFX | YELLOW | Replicated primitive wedge/beam/burst fallbacks cover LMB/RMB/Shift/Q/R/passive; Editor build and automation green | Three-client chain/readability, moving-hook presentation, cleanup, and Low/Medium pass required |
| Jul 20 | Hudson VFX/internal test | YELLOW | Placeholder cue assets audited; code fallbacks cover LMB/RMB/Shift/Q/R/passive; Editor/Game/Server, automation, and 2/2 handshake pass | Recorded three-client firing/readability, release, cleanup, and overflow retest required |
| Jul 21 | Crysta VFX | YELLOW | Primitive fallbacks cover all abilities; empowered LMB observer state, true Q radius, impacts/detonations, X/V beams compile | Two-client readability, mark visibility, path, cleanup, and Low/Medium pass required |
| Jul 22 | Void VFX | YELLOW | Primitive fallbacks cover all abilities; charged/empowered observer state, true zone radii, swap endpoints, and bursts compile | Two-client wall-pierce/cone/swap/recast/readability and cleanup pass required |
| Jul 23 | Hard feature/VFX freeze | RED | Not reached | Full-roster integration required |
| Jul 24 | Playtest 1 | RED | Not run | Requires frozen packaged candidate |
| Jul 25 | Repair day 1 | RED | Not reached | Driven by Playtest 1 findings |
| Jul 26 | Redo gate | RED | Not reached | P0/P1 and readability repair |
| Jul 27 | Lag/loss playtest | YELLOW | Packaged activation 6/6 and authoritative RMB/Shift/Q/R outcomes 24/24 PASS under confirmed `PktLag=100` / `PktLoss=2` | Manual Hudson sustained fire, persistent-zone overlap, and impaired visual agreement remain |
| Jul 28 | Performance/cleanup | RED | Not run | Low/Medium and lifecycle pass required |
| Jul 29 | Dress rehearsal | RED | Not run | Exact playtest topology required |
| Jul 30 | Release candidate freeze | RED | Not built | Final package/distribution verification |
| Jul 31 | Playtest | RED | Not reached | Use verified July 30 RC |

## Ability verification matrix

Mark each cell `-`, `PASS`, or `FAIL`. `Network` means owner, server, and observer agree.

| Hunter | LMB | RMB | Shift | Q | R | Passive | Network | Cleanup |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Ghost | PASS | PASS | FAIL | PASS | PASS | PASS | FAIL | - |
| Eluna | PASS | FAIL | FAIL | FAIL | - | FAIL | FAIL | - |
| Kingpin | - | - | - | - | - | - | - | - |
| Hudson | - | - | - | - | - | - | - | - |
| Crysta | - | - | - | - | - | - | - | - |
| Void | - | - | - | - | - | - | - | - |

## Manual acceptance ledger

| Track | Accepted | Total | Status | Latest evidence/blocker |
| --- | ---: | ---: | --- | --- |
| Hunter abilities + cleanup/stress | 6 | 41 | PENDING | GE-01/02/04/05/06/07 accepted; three repaired Eluna behaviors await focused retest |
| Range indicators | 0 | 9 | PENDING | `docs/ABILITY_RANGE_INDICATORS.md`; hover and active-preview checks remain manual |
| Wisp UI | 2 | 5 | PENDING | Two-bar layout and natural decay accepted; healing continuity, healing contest, and carried smoothness need repaired-package retest |
| **Presentation total** | **8** | **55** | **PENDING** | Ghost/Eluna 6/12 and Wisp UI 2/5 accepted; range indicators remain 0/9 |
| Second-machine direct IP | 0 | 6 | PENDING | External machine/user required; separate from presentation total |

## Active issue log

| ID | Severity | Area | Description | Owner | Status | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| BB-PT-001 | P0 | Automation | GameSystems access violation at test line 449 | Codex | Resolved | GameSystems 28/28; no crash/ensure on Jul 12 |
| BB-PT-002 | P1 | Packaging | Packaged candidate must match current source | Codex | Resolved | `Scripts/Playtest/VerifyPlaytestCandidate.ps1` PASS; evidence in `Builds/PlaytestCandidateVerification.txt` |
| BB-PT-003 | P1 | Networking | Full ability/death/dedicated/package matrix untested | User + Codex | Automated gameplay/package portion resolved; deliberate combat and presentation cleanup remain open | July 15 candidate: 2/2 and 4/4 transport, exact 2v2 mapping/lifecycle 4/4, normal/impaired roster, LMB 6/6, non-LMB 24/24, reconnect, both death/revive directions, and wisp rules 10/10 PASS; total manual presentation is 8/55 |
| BB-PT-004 | P2 | GameplayCues | No targeted GameplayCueNotifyPaths configured | Codex | Resolved | `/Game/GameplayCues` configured; clean Jul 12 editor startup initialization |
| BB-PT-005 | P2 | Environment | UDP Messaging cannot bind 25.18.80.222 | Unassigned | Open | Jul 12 log; GameNetDriver still worked |
| BB-PT-006 | P0 | Wisp | PvP death did not spawn or possess a wisp because GameMode lifecycle wiring was absent | Codex | Resolved | User retest plus `Saved/Logs/Breachborne-backup-2026.07.12-00.26.01.log`: three successful spawn/possession/revive cycles |
| BB-PT-007 | P1 | VFX/Networking | Rapid ability cues exceeded the per-net-update GameplayCue multicast limit | Codex | Automated regression resolved; manual Hudson sustained-fire/readability retest pending | Fire/Impact/Heal use one batched Hudson multicast; redundant Kingpin passive and Crysta R impact cue RPCs removed; every final packaged review reports zero cue-overflow findings |
| BB-PT-008 | P1 | Wisp UI | Wisp widgets constructed and bound on every client but rendered no bars because the tree was created after `RebuildWidget` | Codex | Fix compiled; visual retest pending | Jul 12 repeated-session log; no paint path existed |
| BB-PT-009 | P1 | Ability UI | Primitive range previews need a six-hunter owner/observer readability pass | Codex | Fix compiled; visual retest pending | `docs/ABILITY_RANGE_INDICATORS.md` manual matrix |
| BB-PT-010 | P1 | Networking | Launcher Game client and source-built Server had incompatible network version hashes | Codex | Resolved | Same-toolchain Game/Server plus `TestHeadlessDedicatedHandshake.ps1`: 2/2 joins PASS |
| BB-PT-011 | P1 | Ghost VFX | Primitive fallbacks require complete owner and remote-client acceptance | Codex | Ghost RMB accepted; remaining visual retest pending | Ghost RMB passed in `InteractivePlaytest/20260714-123853`; Editor/PureLogic/GameSystems and current packaged Ghost outcomes pass, while the other Ghost presentation rows remain unchecked |
| BB-PT-012 | P1 | Eluna VFX | Root/heal zone observer state and Q/R authority leaks required repair; complete visual acceptance remains open | Codex | Gameplay/network fix verified; visual retest pending | Eluna R has positive presentation/full-channel revive evidence; replicated radii/toss movement, authoritative Q routing and R resolution, root-on-pop, and packaged RMB/Shift/Q/R outcomes pass; cancellation and Q presentation remain manual |
| BB-PT-013 | P1 | Kingpin VFX | Cone gameplay was owner-debug-only or invisible to observers; full visual acceptance remains unrun | Codex | Fix compiled; visual retest pending | Replicated wedge actor plus per-ability fallbacks; Editor PASS; PureLogic 44/44; GameSystems 27/27 |
| BB-PT-014 | P1 | Crysta/Void VFX | Empowered/charged mesh state and several zone radii did not match on observers | Codex | Fix compiled; visual retest pending | Replicated visual flags, corrected radius scaling, bounded resolution effects; full source build and handshake PASS |
| BB-PT-015 | P1 | Roster | Ghost and Eluna lacked native hunter definitions, so missing/unloadable DataAssets made their IDs unselectable on dedicated runtime | Codex | Resolved | Native definitions now cover IDs 1-6; source and packaged full-roster ability smoke PASS |
| BB-PT-016 | P1 | Networking | A client can reconnect to the server during Playing but is not restored to the disconnected hunter | Codex | Resolved for alive hunters within the 60-second grace period | Persisted direct-IP token plus snapshot restores team, slot, hunter, transform, vitals, and inventory; packaged PASS at `Saved/Logs/PackagedReconnect/20260713-173922` with 3 joins and 0 critical/cue-overflow findings |
| BB-PT-017 | P1 | Death/Wisp | Packaged lethal lifecycle and wisp-priority behavior lacked deterministic evidence | Codex | Gameplay lifecycle and priority resolved; bar visibility/readability and per-ability visual cleanup remain manual | Current-package death directions PASS at `PackagedDeathWisp/20260715-013904` and `013956`; 10/10 priority/lifecycle including Eluna pickup, CC drop, and R revive PASS at `Saved/Logs/PackagedWispRules/20260715-013904`; 0 critical/cue overflow |
| BB-PT-018 | P1 | Automation/Drop | Impaired smoke waited for replicated ability specs before resetting drop movement, allowing one client to remain stuck gliding and never prepare | Codex | Resolved | Server preparation now occurs as soon as the pawn exists; exact source regression PASS and packaged impaired roster PASS 6/6 at 100 ms/2% loss |
| BB-PT-019 | P1 | Networking | Four-client packaged transport and 2v2 lobby/ability flow lacked repeatable evidence | Codex | Automated transport, lobby, selection, mapping, spawn, and ability lifecycle resolved; deliberate combat/readability remains manual | Transport `Saved/Logs/PackagedFourClientHandshake/20260715-013904` PASS 4/4; 2v2 `Saved/Logs/PackagedFourClientAbilitySmoke/20260715-014122` PASS with mappings/lifecycles 4/4, activations 20/20, releases 8/8, and 0 findings |
| BB-PT-020 | P1 | Death/Passive | `CancelAllAbilities` stopped the hunter passive permanently because revive did not reactivate it; owned actors and loose empowered states could survive death | Codex | Resolved | Central death cleanup destroys hunter-owned actors, resets pull/glider/mantle state, removes transient effects/tags, and revive restarts one server passive; both custom passive teardown paths pass packaged smoke |
| BB-PT-021 | P2 | Networking | Reconnect reconstructs a clean pawn/ASC instead of serializing active casts, temporary effects, or cooldown timers | Unassigned | Accepted playtest limitation | Identity, position, vitals, progress, and inventory restore; transient combat state is deliberately cleaned and abilities are freshly granted |
| BB-PT-022 | P1 | Networking/Abilities | RMB/Shift/Q/R had activation-only coverage and predicted-client lifetimes could cancel authoritative resolution | Codex | Resolved by deterministic source/package outcome gates | Current package proves 24/24 normally at `PackagedOutcomeSmoke/20260715-013021` through `013518` and impaired at `PackagedOutcomeNetworkImpairment/20260715-013713`, `013126` through `013543`; fixes include active-Q authority, Eluna root/R ownership, Kingpin R/Shift, Hudson reel/Q aim placement, stable cooldown specs, Crysta mark removal, and Crysta R ownership |
| BB-PT-023 | P1 | Void Gameplay/VFX | Void Snap's cone tip faced opposite travel and marked swap props were not made movable/replicated | Codex | Resolved in source and deterministic tests; live readability remains manual | Cone local pitch corrected; `UBBVoidSwappableComponent` prepares owner mobility/replication; GameSystems 49/49 covers orientation, hit volume, empowered scaling, prop mobility, eligibility, category swap, and Singularity lifecycle; packaged Void outcomes PASS 4/4 normally and impaired |
| BB-PT-024 | P1 | QA Tracking | The original 0/41 headline omitted the existing nine range checks and had no explicit wisp-bar visual matrix | Codex | Resolved in tracking; execution pending | Manual presentation is explicitly 8/55: hunter/cleanup 6/41, range 0/9, wisp UI 2/5; direct-IP remains separate |
| BB-PT-025 | P1 | Void VFX Fixture | The frozen TestMap package has no placed prop with `UBBVoidSwappableComponent`, so marked-prop visual swapping cannot be observed | Codex | Setup blocked; do not infer PASS from deterministic tests | Source/map audit finds no cooked fixture; Crysta/Void launcher records the limitation; adding one requires a later content/package change |
| BB-PT-026 | P1 | Manual Tooling | The documented two-client pair sessions could not exercise Eluna's ally mechanics or Kingpin's two-target chain | Codex | Resolved in tooling; execution pending | Three-client Ghost/Eluna and Kingpin/Hudson wrappers define exact teams, slots, hunters, evidence, candidate preflight, and safe stop/review |
| BB-PT-027 | P1 | Distribution Tooling | Manifest tooling used `.NET Path.GetRelativePath`, which is unavailable in Windows PowerShell 5.1 | Codex | Resolved | Shared relative-path implementation plus `TestDistributionVerifier.ps1` PASS 8/8 for generated-valid, tampered, missing, unexpected, duplicate, traversal, malformed, and restored-valid scenarios |
| BB-PT-028 | P1 | Direct-IP Tooling | The external-machine runbook launched only the remote client even though acceptance required host/remote observation | Codex | Resolved in tooling; external execution pending | Runbook now launches server, host client, initial remote, and remote reconnect; safe client stop plus combined reviewer PASS 4/4 synthetic scenarios and validates DI-01 through DI-06 |
| BB-PT-029 | P0 | Match Reset/Abilities | A player ending game 1 dead retained `State.Dead` on the persistent PlayerState ASC, so GAS granted Ghost's abilities in game 2 but rejected every activation | Codex | Automated fix verified; exact manual two-game retest pending | Failure: `Saved/Logs/InteractivePlaytest/20260713-225601`; `ResetForNewMatch` clears stale effects/input/state before grants. Source GameSystems 52/52; current packaged regression PASS at `Saved/Logs/PackagedMatchReset/20260715-013904` with clean resets 2/2 and Ghost activations 5/5 |
| BB-PT-030 | P1 | Four-client Automation | Owner could start Hunter Select immediately when client 4 connected, before client 4's requested team-slot RPC reached the server | Codex | Resolved | Failed mapping 3/4 at `PackagedFourClientAbilitySmoke/20260713-235319`; smoke-only 1.5-second full-lobby settle window added; rebuilt-package rerun PASS mappings/lifecycles 4/4 and activations 20/20 at `PackagedFourClientAbilitySmoke/20260713-235955` |
| BB-PT-031 | P1 | Eluna R | The owning predicted client completed its 3.0s timer milliseconds before the server and replicated an ability end, cancelling the authoritative revive at 2.8s | Codex | Completion fix packaged and automated; manual range/stun cancellation pending | Failure at `Saved/Logs/InteractivePlaytest/20260714-121437`; current wisp smoke proves full-channel revive, passive restart, and hunter repossession at `Saved/Logs/PackagedWispRules/20260714-141535`. Completion, range break, and CC interruption are server-owned. |
| BB-PT-032 | P2 | Gameplay HUD | In-match hunter label helper only mapped IDs 1-2, displaying `Hunter 3` and `Hunter 4` for Eluna and Hudson | Codex | Source fix compiled; packaged visual retest pending | `BBDebugHUD` now maps IDs 1-6 to Ghost, Kingpin, Eluna, Hudson, Crysta, and Void. |
| BB-PT-033 | P2 | Eluna Passive VFX | The recurring heal fallback was an obstructive floor disc and its delayed first visible tick appeared tied to using RMB | Codex | Source fix compiled; packaged visual retest pending | Passive was active from grant (`count=1`); real-heal feedback is now an immediate-eligible, small floating green plus instead of a floor disc. |
| BB-PT-034 | P1 | Eluna Q/Wisp | Initial Q stayed on the floor; tossed Q ignored allied wisps/dead-body filtering and appeared unbounded instead of following the first eligible target | Codex | Source/package rules verified; visual behavior pending | Failure reproduced in `Saved/Logs/InteractivePlaytest/20260714-123853`. Initial Q follows Eluna; toss sweeps for the first eligible allied hunter/wisp, follows it, applies wisp healing, and drops when an attached hunter dies. GameSystems 51/51 and current Eluna package outcomes PASS. |
| BB-PT-035 | P1 | Ghost Passive/Networking | Ghost passive assumed `WaitGameplayEvent` was one-shot, then registered another persistent listener after every kill; later kills emitted increasing duplicate pulse RPCs | Codex | Package regression clean; exact multi-kill visual retest pending | Manual failure had 13 cue overflows, including 10 passive pulses. Listener is one-shot before re-registering; every fresh current-package review reports zero cue-overflow findings. |
| BB-PT-036 | P2 | Ghost Q/R Readability | Q laser and R landing telegraphs resolve too quickly for a useful reaction window | Codex | Source tuning implemented; visual retest pending | Tester requested +0.25 to +0.4 seconds. Q now has an explicit 0.3-second thin telegraph before trace/damage; R warning increases from 0.45 to 0.75 seconds. |
| BB-PT-037 | P1 | Movement/Networking | Remote characters and abilities are smooth in the focused local window and equally jittery in both unfocused packaged windows | Codex | Local focus behavior isolated; separate-machine confirmation required | July 15 tester confirmed the pattern across all abilities. UE 5.7 packaged `t.IdleWhenNotForeground` defaults to 0, but OS/Slate rendering of background windows still differs. Do not retune gameplay replication from this same-machine observation; compare focused clients on separate machines during the weekend smoke. Eluna Q/carried jitter reproduced in focused windows and is tracked separately in BB-PT-044. |
| BB-PT-038 | P1 | Automation/Readiness | One pre-candidate impaired Void activation run prepared the server pawn but did not replicate owner ability specs before the harness deadline | Codex | Not reproduced; monitor | Failure `Saved/Logs/PackagedNetworkImpairment/20260714-133518`; immediate retry passed, and the rebuilt 14:01 package passes all six hunters at 100 ms/2% loss in `140857`, `141001`, and `141105`. |
| BB-PT-039 | P1 | Crysta R | A predicted client could end Crysta R before the server's first delayed salvo; the zero-delay first timer also never fired | Codex | Resolved and packaged | Non-authority now waits for server-owned completion and salvo zero fires immediately. Three focused reruns passed, then current normal and impaired 24/24 outcome suites passed at `PackagedOutcomeSmoke/20260714-142409` and `PackagedOutcomeNetworkImpairment/20260714-143034`. |
| BB-PT-040 | P1 | Ghost Cooldowns/Passive | Ghost RMB/Shift/Q/R used obsolete transient GameplayEffects, allowing Shift to activate twice inside its six-second cooldown and leaving the kill passive nothing to reset | Codex | Resolved and manually accepted | Live failure at `InteractivePlaytest/20260715-001027`; focused retest `20260715-014500` accepted cooldowns/reset and logged one eligible removal with zero cue overflow. |
| BB-PT-041 | P1 | Wisp Healing | Healing-started revival stopped when the heal source expired instead of continuing until enemy contest | Codex | Packaged regression pass; manual retest pending | Failure reached 76-80% then stopped in `20260715-014500`. Persistent latch plus enemy cancellation now pass exact packaged scenarios in `PackagedWispRules/20260715-030814`. |
| BB-PT-042 | P1 | Eluna RMB | Sticky projectile passed through or repeatedly rubberbanded around its first enemy before the delayed root expanded | Codex | Packaged outcome pass; manual visual retest pending | Proxy simulation is disabled and stuck target/offset replicate explicitly. Final Eluna outcome contracts pass 4/4 at `PackagedOutcomeSmoke/20260715-030959`. |
| BB-PT-043 | P1 | Eluna Shift | Dash refund failed to detect and collect an allied wisp along the projected path | Codex | Packaged path regression pass; manual retest pending | Authoritative point-to-segment detection and granted-tag cooldown removal compile; exact Shift-path pickup passes in `PackagedWispRules/20260715-030814`. |
| BB-PT-044 | P1 | Eluna Q/Carry Networking | Q and carried wisps were server-teleported at 20 Hz/5 Hz, producing focused-client jitter and model/camera bouncing | Codex | Resolved and manually accepted | Q uses replicated target attachment and carried wisps attach once. Focused retest `20260715-014500` accepted smooth Q following and substantially improved carried-wisp movement. |
| BB-PT-045 | P1 | Ghost LMB Networking | Three held-fire ticks exceeded GAS's two-GameplayCue-RPC per-net-update budget | Codex | Packaged regression resolved; manual readability retest optional | Owner predicts the muzzle cue locally and observers use the replicated projectile. Every current-package review reports zero GameplayCue-overflow findings. |
| BB-PT-046 | P1 | Hudson Q/Networking | Hudson Q preferred actor facing over replicated aim, allowing late movement correction under 100 ms/2% loss to place barbed wire away from the target | Codex | Resolved and packaged | Q now uses `GetAimDirection()` with actor facing only as fallback. Focused normal and impaired reruns pass 4/4; full current-package outcome matrices pass 24/24 in both profiles. |

## Daily updates

### 2026-07-15

- Gate due: tester-controlled Ghost/Eluna owner-observer acceptance.
- Status: YELLOW.
- Session launched: `Saved/Logs/InteractivePlaytest/20260715-001027`; frozen candidate preflight
  PASS before launch. Client 1 is Eluna on Team 0/Slot 0, client 2 is allied Hudson on Team 0/Slot
  1, and client 3 is opposing Ghost on Team 1/Slot 0. Storm must be disabled before match start.
- Optional authoring route: LUDUS AI 1.0 early access may be evaluated after the technical/manual
  gates for shared animations and ability VFX. The user operates its Unreal Editor window while
  Codex prepares prompts/specifications and reviews outputs. It is not a deadline dependency;
  existing Niagara/primitive effects and licensed sourced assets remain fallbacks. Ludus MCP can
  expose knowledge search to Codex after authentication, but official documentation says it lacks
  plugin project context/actions and therefore does not submit generation prompts to the UE window.
- Live findings: Ghost's old transient cooldown effects did not enforce RMB/Shift/Q/R cooldowns;
  source now uses the stable shared cooldown spec. Eluna Q healing pulses repeatedly dropped wisp
  revive state between 0.5-second pulses. The completed repair batch also covers Eluna RMB first-
  target latch, Shift wisp collection/full refund, Q replicated attachments, carried-wisp movement,
  and Ghost LMB cue budgeting. Editor PASS, PureLogic 44/44, and GameSystems 52/52.
- Live positive evidence: Ghost R authoritative warning/zone lifecycle completed; Eluna R completed
  a full hunter heal channel; wisp widgets reached `FirstPaint` on all three clients; ordinary ally
  revive and Eluna carried revive completed. These are gameplay/log facts, not visual acceptance.
- Manual result: original 26-item report mapped and recorded in
  `Saved/Logs/InteractivePlaytest/20260715-001027/GhostEluna-ManualAcceptance.md`. GE-01/02/04/05/07
  and WI-01/02 pass. Ghost Shift/passive, Eluna RMB/Shift/Q/passive carry, remaining R cancellation,
  and three wisp priority/smoothness rows remain failed or incomplete. Session review found zero
  critical errors and three Ghost LMB fire-cue budget overflows.
- Package result: July 15 01:26 client/server package and 01:42 freshness-aware candidate verifier
  PASS. Current evidence covers roster activation 6/6 normally and impaired, LMB hits 6/6,
  RMB/Shift/Q/R outcomes 24/24 normally and impaired, match reset, reconnect, both death/wisp
  directions, wisp rules 10/10, transport 4/4, exact 2v2 lifecycle 4/4, 20/20 activations, 8/8
  releases, and zero critical/cue-overflow findings.
- Regression repair: an impaired Hudson Q run exposed placement from actor facing instead of
  replicated aim. Q now prefers `GetAimDirection()`; focused normal/impaired runs pass 4/4 and the
  complete refreshed matrices are green.
- Tooling revalidation: VFX audit is `FALLBACK_READY_AUTHORING_INCOMPLETE` with cue roots, template
  contract, primitive files, and Low/Medium scalability passing and authored Niagara masters at
  0/8. Manual acceptance definition passes 55/55 exact rows, direct-IP reviewer self-test passes
  4/4 with DI-01 through DI-06 unique, and distribution verifier self-test passes 8/8.
- Post-package orchestration: `RefreshPackagedCandidateEvidence.ps1` now runs every verifier-required
  gate sequentially with isolated ports and serialized death directions, then runs candidate
  verification. Its `-VerifyOnly` path passes against the current candidate.
- Distribution preflight: `PreparePlaytestDistribution.ps1 -IncludeServer -ValidateOnly` passes the
  candidate, manual definition 55/55, manifest self-test 8/8, and direct-IP reviewer self-test 4/4;
  no files were copied or archived while manual repair acceptance remains open.
- Focused retest launched at `Saved/Logs/InteractivePlaytest/20260715-014500` using the verified
  package. Ghost cooldown/reset passed; Eluna RMB latch, Shift refund, and healing persistence failed;
  Q following and carried-wisp smoothness passed. Review found zero critical/cue-overflow findings.
- Repair result: final 03:07 package passes Wisp Rules 12/12 and focused Eluna outcomes 4/4. The
  persistent-heal regression also caught and fixed a contested-heal edge where a late pulse could
  remain latched after the enemy left.
- Tomorrow's manual retest is only: Eluna RMB latches once without forward/back looping; Shift through
  an allied wisp collects it and fully refunds; one Q heal starts resurrection that continues after Q
  expires, while enemy overlap cancels it. Do not repeat Ghost, Q-follow, or carry-smoothness checks.

### 2026-07-14

- Gate due: manual two-game Ghost/Eluna rematch
- Status: YELLOW
- Ghost rematch diagnosis: in `Saved/Logs/InteractivePlaytest/20260713-225601`, game 2 granted
  Ghost's full ability set, but every activation was rejected with `State.Dead`. The PlayerState ASC
  persists between matches and retained dead/wisp state from game 1.
- Fix: added `UBBAbilitySystemComponent::ResetForNewMatch` and call it before hunter ability grants.
  It cancels active abilities, removes active effects, clears pressed spec inputs, and removes loose
  tags under `State` and `Cooldown`. Added source and packaged injected-stale-state regressions.
- Source result: Editor PASS; PureLogic 44/44; GameSystems 49/49 with no failure or ensure.
- Package result: client/server package PASS at July 13 23:59; final freshness-aware verifier PASS
  at July 14 00:27 with zero critical/cue-overflow findings.
- Exact reset regression: `Saved/Logs/PackagedMatchReset/20260714-000342` injects Dead/Wisp on
  both players plus Ghost Shift cooldown, then proves clean server resets 2/2 and Ghost post-reset
  LMB/RMB/Shift/Q/R activations 5/5.
- Full package regression: normal/impaired activation 6/6; reconnect PASS; death/wisp/revive both
  directions PASS; wisp rules 9/9; authoritative LMB hits 6/6; non-LMB outcomes 24/24 normally and
  24/24 at 100 ms/2% loss; transport 4/4; exact 2v2 mappings/lifecycles 4/4 and activations 20/20.
- Four-client harness repair: the first rebuilt run exposed a 33 ms start/team-slot race for client
  4. Added a smoke-only 1.5-second full-lobby settling window and reran PASS at
  `Saved/Logs/PackagedFourClientAbilitySmoke/20260713-235955`.
- Manual topology: bottom-left = client 1 = Eluna/lobby owner; top-left = client 2 = allied Hudson;
  top-right = client 3 = opposing Ghost.
- Manual next action: switch Storm off on bottom-left before starting, deliberately end game 1 with
  Ghost dead, start game 2, and test Ghost LMB/RMB/Shift/Q/R. Do not infer a manual PASS from the
  automated fixture.
- Paused session: `Saved/Logs/InteractivePlaytest/20260714-002907` was launched, then stopped before
  manual testing for an overnight break. Stop/review PASS with 3/3 joins and zero critical or
  GameplayCue-overflow findings; it is not acceptance evidence. Launch a fresh session tomorrow.
- Partial manual session: `Saved/Logs/InteractivePlaytest/20260714-121437` ran with Eluna owner,
  allied Hudson helper, and opposing Ghost, then stopped/reviewed with 3/3 joins and zero critical
  or GameplayCue-overflow findings. `GE-11` and `GE-12` are recorded FAIL; remaining rows stay
  pending. It also exposed the missing Eluna/Hudson HUD names.
- Eluna repair: server logs proved R received fourteen 0.2s heal ticks but was ended by the owning
  predicted client's slightly earlier completion timer. R completion/range/CC decisions are now
  server-only, including a final boundary check. The passive floor disc is replaced by a small
  floating green heal plus, shown only for real healing and evaluated once immediately on grant.
  HUD names now cover roster IDs 1-6.
- Repair verification: Editor build PASS; PureLogic 44/44; GameSystems 51/51 with zero failed
  tests. The only environment warning is the known inactive UDP Messaging adapter bind; gameplay
  networking uses GameNetDriver and all packaged network gates pass.
- Focused repair result: `Saved/Logs/InteractivePlaytest/20260714-123853` proves Eluna R
  full-channel resurrection twice and the tester accepted its revised presentation. Ghost RMB is
  the first fully accepted GE row. Q/R telegraphs need +0.3 seconds; Eluna Q failed carrier/path/wisp
  behavior; and review found 13 cue overflows caused by accumulating Ghost passive listeners.
- Current source repairs: initial Eluna Q follows her, toss detaches and links to the first living
  allied hunter/wisp, wisp healing enters `BeingRevived`, enemy contest still overrides healing,
  and hunter death drops Q in place. Ghost Q now telegraphs for 0.3 seconds, R warns for 0.75
  seconds, and the passive kill listener is explicitly one-shot. Compile and current-package
  automation pass; the visual behavior remains manual.
- Crysta R repair: a normal packaged outcome run exposed a predicted-client/server ownership race,
  and source inspection found that the zero-delay first salvo timer never executed. Non-authority
  now waits for the server-owned end, salvo zero fires immediately, and salvos 1-4 use timers.
  Three focused package reruns passed before the complete normal and impaired outcome matrices.
- Frozen candidate: client/server package completed at 14:01. Fresh post-package evidence passes
  handshake 2/2, roster activation 6/6, match reset, normal impairment activation 6/6, reconnect,
  both death/wisp directions, wisp rules 10/10 including Eluna R revive, LMB hits 6/6, non-LMB
  outcomes 24/24 normally and 24/24 at 100 ms/2% loss, four-client transport 4/4, and exact 2v2
  mapping/lifecycle 4/4 with 20/20 activations and 8/8 releases. All reviews have zero critical and
  zero GameplayCue-overflow findings.
- Candidate evidence: `Builds/PlaytestCandidateVerification.txt`; wisp rules at
  `Saved/Logs/PackagedWispRules/20260714-141535`; normal outcomes begin at
  `Saved/Logs/PackagedOutcomeSmoke/20260714-142012`; impaired outcomes begin at
  `Saved/Logs/PackagedOutcomeNetworkImpairment/20260714-142617`; four-client transport at
  `Saved/Logs/PackagedFourClientHandshake/20260714-143259`; exact 2v2 ability smoke at
  `Saved/Logs/PackagedFourClientAbilitySmoke/20260714-143334`.
- Movement investigation: UE 5.7 packaged idle-on-unfocus defaults off in engine source; therefore
  the report may be local multi-window GPU/Slate behavior or remote movement replication rather
  than the network impairment simulation. Retest must compare focused owner/observer FPS and then
  use separate focused machines at the weekend smoke test.
- Plan reconciliation: audited every `docs/plans` checklist against current source and packaged
  evidence. Crysta and Void implementation items remain fully checked; their current deterministic
  tests and evidence paths now reference the July 14 candidate. Manual readability, cleanup/stress,
  range, wisp UI, and second-machine rows remain unchecked because they require direct observation
  or external hardware.
- Distribution preflight: `PreparePlaytestDistribution.ps1 -IncludeServer -ValidateOnly` PASS at
  14:35 against the frozen candidate; manual definition 55/55, manifest verifier 8/8, and direct-IP
  reviewer 4/4. No files were staged, copied, or archived.
- Unattended boundary: automated technical/package verification is complete. Do not launch visible
  clients or infer visual PASS rows while the tester is away. Resume with the Ghost/Eluna manual
  checklist: Eluna Q follow/link/wisp/death-drop, Eluna R range/stun cancellation, Ghost Q/R timing,
  repeated-kill passive cue, hunter names, restrained Eluna passive, wisp bars, and focus-controlled
  movement comparison. Manual presentation remains 1/55; second-machine direct IP remains 0/6.

### 2026-07-13

- Gate due: Listen-server combat
- Status: YELLOW
- Completed: centralized authoritative death cleanup; added character pull/glider/mantle reset;
  added clean Ghost and Kingpin passive teardown; restarted server passives after wisp revive and
  dev respawn; strengthened and parameterized packaged death/wisp smoke; made normal ability and
  reconnect scripts always produce critical/cue-overflow reviews; added a persisted random direct-IP
  reconnect identity and 60-second Playing-phase same-hunter restoration; added deterministic
  hunter-specific RMB/Shift/Q/R outcome smoke; repaired active-Q authority races, Eluna RMB root,
  Kingpin R/Shift authority, Hudson R reel lifetime, and redundant cue overflow; hardened every
  sequential smoke wrapper against stale evidence selection; added and passed the same 24
  authoritative contracts under confirmed 100 ms lag and 2% loss on every process; extracted the
  live wisp tick resolver and added source plus packaged coverage for all nine priority scenarios;
  added a four-client 2v2 smoke path and packaged gate for exact team/slot/hunter mapping, match
  start, 20/20 primary activations, 8/8 releases, and four completed client lifecycles; repaired
  the harness launch-order race by waiting for each indexed client join; corrected Void Snap cone
  orientation and made marked swap props movable/replicated; added deterministic Crysta R X/V,
  Void charged-LMB, cone, swap-category, prop-mobility, and Q-recast coverage; repaired shared
  dynamic cooldown construction so tags and duration survive spec creation; verified Crysta's two
  Shift charges, dual cooldown reduction, empowered LMB front/back ordering, and Reverberation
  removal; grounded Hudson Q placement and retained facing under simulated loss
- Build result: Editor PASS; Game PASS; source-engine Server PASS
- PureLogic result: 44 passed, 0 failed, 0 critical
- GameSystems result: 48 passed, 0 failed, 0 critical; storm default and manager suspension checks included
- Packaged/dedicated result: freshness-aware candidate verifier PASS at 22:54 after a 22:27 rebuild;
  17/17 Hudson cues, handshake 2/2, transport 4/4, exact 2v2 mapping/lifecycle 4/4,
  normal/impaired activation 6/6,
  authoritative LMB hits 6/6, non-LMB outcomes 24/24 normally and at 100 ms/2% loss,
  same-hunter reconnect restoration, both strengthened death/wisp cleanup/revive directions, and
  the nine-scenario wisp priority matrix
- Reconnect regression: PASS at `Saved/Logs/PackagedReconnect/20260713-194652`; the replacement
  process reclaimed Eluna on team 1/slot 0 with exact recorded transform, health/max health,
  shield/max shield, and inventory values; 3 joins and zero critical/cue-overflow findings
- Death regression: Kingpin victim PASS at `Saved/Logs/PackagedDeathWisp/20260713-194839`;
  Ghost victim PASS at `Saved/Logs/PackagedDeathWisp/20260713-195032`; both report cleanup=1,
  transient states cleared, passives=1 after revive, and zero critical/cue-overflow findings
- Wisp-priority regression: PASS 9/9 at `Saved/Logs/PackagedWispRules/20260713-195128` for natural,
  ally, enemy, ally/enemy, healing, healing/enemy, carried/enemy, Eluna pickup, and Eluna CC drop
- Candidate evidence: `Builds/PlaytestCandidateVerification.txt`; fresh normal roster runs begin
  at `Saved/Logs/PackagedAbilitySmoke/20260713-193953`, impaired runs at
  `Saved/Logs/PackagedNetworkImpairment/20260713-194547`, hit runs at
  `Saved/Logs/PackagedHitSmoke/20260713-195245`, non-LMB outcome runs at
  `Saved/Logs/PackagedOutcomeSmoke/20260713-195535`, impaired outcome runs at
  `Saved/Logs/PackagedOutcomeNetworkImpairment/20260713-200147` plus Hudson `193801`, four-client
  transport at `Saved/Logs/PackagedFourClientHandshake/20260713-200715`, and exact 2v2
  lobby/ability evidence at `Saved/Logs/PackagedFourClientAbilitySmoke/20260713-200747`
- Manual acceptance: 0/55 presentation checks accepted: hunter/cleanup VFX 0/41, range indicators
  0/9, and wisp UI 0/5. Ghost/Eluna is still 0/12; no visible windows were launched during this
  run and no visual row was inferred from automation. Second-machine direct-IP is separate.
- Manual-session readiness: `Scripts/Playtest/StartGhostElunaAcceptance.ps1` now launches the
  required three-client topology and writes exact Eluna/helper/Ghost roles into the evidence
  directory. All three hunter-pair launchers create a session-local 12-row acceptance record with
  separate owner, observer, gameplay, cleanup, result, and notes fields. This fixes the earlier
  two-client checklist gap for ally-dependent Eluna checks and prevents clean logs from being
  mistaken for visual acceptance.
- Acceptance-definition gate: `ManualAcceptanceDefinition.psd1` is the structured source for all
  six batches; `TestManualAcceptanceDefinition.ps1` proves 55/55 unique IDs and exact agreement
  with the manual and range documents. Record generation validates 12 GE, 12 KH, 12 CV, 5 CS,
  9 RI, and 5 WI rows without launching processes.
- Unattended test-support pass: preserved the 19:37 candidate; added pair-specific launchers,
  candidate preflight, active-session refusal, session labels, server/client extra-argument
  capture, expected-join review, direct-IP evidence launchers, and exact 55-check/manual plus
  second-machine acceptance documents. Added release-bundle staging, SHA-256 manifests, and a
  destination-side verifier without staging or recompressing the candidate. No gameplay C++,
  content, package, or visible process was changed or launched.
- Unattended tooling verification: all pair wrappers pass no-launch preflight with an unchanged
  session pointer; distribution manifest/verifier behavior passes 8/8; direct-IP combined review
  passes 4/4 synthetic scenarios; all temporary fixtures are removed. Manual presentation remains
  0/55 and second-machine direct IP remains 0/6.
- Distribution validation: `PreparePlaytestDistribution.ps1 -IncludeServer -ValidateOnly` PASS at
  20:55 for the candidate, 55-row definition, manifest verifier 8/8, and direct-IP reviewer 4/4;
  no files were staged or archived.
- Eluna passive/storm retest fix: traced the recurring Q-sized blue bubble to Soul Pack's five-second
  600-unit pulse and replaced it with a 120-unit pale-green foot pulse. Added a replicated,
  owner-only `Storm` lobby checkbox that defaults on; when off, the server keeps storm progression,
  damage, and debug visualization suspended and logs `BB_STORM_SETTING|SERVER|MatchStart enabled=0`.
- Fresh fix verification: Editor/Game/Server package PASS; PureLogic 44/44 and GameSystems 48/48;
  normal/impaired activation 6/6, reconnect restoration, both death/wisp directions, wisp rules 9/9,
  authoritative LMB 6/6, normal/impaired non-LMB outcomes 24/24, four-client transport 4/4, and
  four-client ability lifecycle 4/4 all PASS with zero critical/cue-overflow findings. Candidate
  summary: `Builds/PlaytestCandidateVerification.txt`.
- Open P0/P1 issues: Ghost/Eluna then remaining owner/observer visual matrix; deliberate 2v2
  movement/combat/readability; wisp bars/range indicators/Hudson
  sustained-fire live retests; Low/Medium and second-machine direct-IP
- Scope cut triggered: none
- Next single target: launch a fresh Ghost/Eluna session, switch Storm off before match start, verify
  the five-second Eluna passive pulse, then deliberately record the 12 owner/observer checks
- Goal continuation boundary: the refreshed candidate is green; manual visual acceptance remains the
  active boundary and must not be inferred from automation.
- Live Ghost/Eluna session: `Saved/Logs/InteractivePlaytest/20260713-220140` reached gameplay with
  Ghost on Team 1 and Eluna/Hudson on Team 0; 3/3 clients joined. It was stopped and reviewed PASS
  with zero critical or GameplayCue-overflow findings. Ghost RMB and Shift were exercised, but no
  tester visual PASS/FAIL results were supplied, so GE acceptance remains 0/12. The session exposed
  the oversized recurring Eluna passive pulse fixed in the 22:27 candidate.

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
- Candidate verifier: PASS for source freshness, executables, cooked TestMap, Hudson cues 17/17, handshake 2/2 plus four-client transport 4/4, normal/impaired roster 6/6, authoritative LMB hits 6/6, reconnect attempt, death/wisp/revive, and zero critical packaged-log findings; summary at `Builds/PlaytestCandidateVerification.txt`
- Interactive acceptance tooling: upgraded `StartPackagedLocalSmoke.ps1` to launch a hidden packaged server and side-by-side clients with isolated timestamped logs and exact PID metadata; added safe stop plus automatic review scripts; parser checks PASS and reviewer PASS against `Saved/Logs/PackagedHandshake/20260712-191726` with 2 joins, zero critical matches, and zero cue-overflow matches
- Source-control checkpoint: committed the green six-hunter VFX/network/wisp/range/tooling baseline as `d425cd9`; Jul 12 gate is GREEN
- Automated roster gameplay: added opt-in `-BBAbilitySmoke` flow plus packaged single-pair/full-roster scripts; fixed nondeterministic lobby-owner startup, isolated activation checks from enemy CC, and added native Ghost/Eluna definitions; source and packaged runs PASS for hunter pairs 1/2, 3/4, and 5/6 with all initial inputs successful and cleanup complete
- Candidate freshness: verifier rejects archives older than source/config/content/plugin inputs; refreshed main package and all required post-build evidence now PASS
- Network impairment: server/client `PktLag=100` plus `PktLoss=2` confirmed; activation lifecycle PASS for hunters 1-6 at `Saved/Logs/PackagedNetworkImpairment/20260712-212122`, `20260712-212221`, and `20260712-212320`; manual sustained-fire/zone-overlap visual checks remain open
- Death/wisp automation: shared GAS lethal damage, HealthSet depletion, authoritative/client wisp possession, server-timed capped healing, revive at 50% health, and victim-client hunter repossession PASS at `Saved/Logs/PackagedDeathWisp/20260712-212520`
- LMB hit automation: `TestPackagedFullRosterHitSmoke.ps1` repositions opponents after lifecycle smoke and requires positive authoritative health loss from every hunter; PASS 6/6 at `Saved/Logs/PackagedHitSmoke/20260712-212609`, `20260712-212703`, and `20260712-212757`
- Four-client transport: `TestPackagedFourClientHandshake.ps1` PASS 4/4 with clean five-log review at `Saved/Logs/PackagedFourClientHandshake/20260712-213546`; this does not replace manual 2v2 lobby/team/combat acceptance
- Exploratory Ghost/Eluna session: `Saved/Logs/InteractivePlaytest/20260712-200148` stopped and reviewed PASS with 2 joins and zero critical/cue-overflow findings; no visual checklist rows accepted because owner/observer results were not deliberately recorded
- Two-client result: roster activation, authoritative LMB damage, and core death/wisp/revive are automated; replacement reconnect reaches the server but match-state restoration is unsupported; manual visuals, aim/movement, non-LMB damage/CC, and death cleanup remain
- Packaged/dedicated result: Editor/Game/Server build PASS; final PureLogic 44/44 and GameSystems 28/28; refreshed candidate and all automated post-build gates including 4/4 transport PASS; second-machine direct-IP remains pending
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
| Jul 15 | LUDUS AI 1.0 is an optional post-gate authoring experiment | Early access may accelerate shared animations or skill VFX, but output quality and integration are unproven | User relays prompts unless a documented integration exists; generated output must pass existing gameplay/network/readability gates, otherwise use Niagara/primitive or licensed sourced fallbacks |
| Jul 12 | Low/Medium first | Playtest reliability and broad hardware | No High/Epic polish before deadline |

## Calendar configuration

- Calendar: primary Google Calendar
- Events: July 12 through July 31, one hard gate per day
- Event time: 20:00-20:30 Europe/Bucharest
- Reminders: popup 12 hours and 1 hour before each gate
- Events are private and transparent; they are deadline reviews rather than busy blocks
