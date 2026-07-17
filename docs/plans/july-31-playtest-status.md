# July 31 Playtest Status

Companion tracker for `docs/plans/july-31-playtest-vfx-network-plan.md`.
Update this file at the end of every workday and whenever a hard gate changes state.
Manual runtime matrix: `docs/plans/july-31-manual-vfx-acceptance.md`.

Last updated: July 17, 2026.

## Current headline

- Current gate: manually retest Eluna Shift owner cooldown/refund feedback on the verified July 17 candidate, then continue the remaining presentation matrix
- Overall status: YELLOW
- Primary blocker: Eluna RMB latch stability and Q-started resurrection continuity are manually accepted.
  Fresh package automation proves the exact Shift cooldown is removed, but its owner HUD/refund pulse
  still needs human confirmation. Remaining presentation rows, Low/Medium, deliberate combat, and
  second-machine direct IP are also manual/external gates.
- PureLogic baseline: 47 passed, 0 failed on July 17.
- GameSystems baseline: 55 passed, 0 failed on July 17, including persistent-ASC match reset,
  stable cooldown specs, Crysta mark removal and Shift charges, and Void geometry, collision, swap,
  Singularity lifecycle contracts, Eluna Q carrier rules, Eluna R completion ownership, and isolated
  Eluna dash cooldown refund.
- Build baseline: the post-authoring Editor build, fresh packaged Game/Server targets, and complete
  freshness-aware package matrix succeeded on July 17.
- Latest network evidence: the full July 17 matrix completed at `20260717-042049` and the verifier
  passed at 04:21. It proves normal/impaired roster activation 6/6, LMB hits 6/6, non-LMB outcomes
  24/24 normally and under 100 ms/2% loss, match reset, reconnect, both death/wisp directions, wisp
  rules 12/12, four-client transport/mapping 4/4, and zero critical/cue-overflow findings. Visual
  acceptance remains pending.
- Wisp lifecycle: the fresh package passes 12/12 at `Saved/Logs/PackagedWispRules/20260717-040413`,
  including one-heal persistence after the source expires, enemy cancellation, exact Shift cooldown
  refund, passive pickup, CC drop, carried protection, and Eluna R revive/re-possession. The tester
  manually confirmed Q-started resurrection now completes; combined owner/observer bar rows remain.
- Ability range indicators: shared owner-only hover and active previews cover all six hunters and F/G powers; visual/network retest pending.
- GameplayCue networking: targeted cue scan paths configured; Hudson minigun cues use one cosmetic multicast per shot with local primitive rendering and no per-shot debug RPCs. Final normal, impaired, hit, death/revive, reconnect, four-client, and 24-contract packaged reviews report zero cue-overflow findings; manual ten-second Hudson firing/readability remains pending.
- VFX foundation: template enum, palette/parameter schema, numeric palettes, budgets, content roots, and primitive fallback contract locked; Niagara master assets remain unbuilt.
- VFX authoring handoff: `docs/plans/ludus-vfx-prompt-pack.md` contains directly forwardable project
  context, eight budgeted master-system prompts, six hunter variant prompts, and an audit prompt.
  LUDUS authoring is now active, but staged output does not advance the authored count from `0/8`
  until it matches a canonical master path and passes parameter, budget, cook, and live readability gates.
- VFX asset audit: `Scripts/Playtest/AuditVfxFoundation.ps1` distinguishes fallback readiness
  from authored completion and writes `Builds/VfxFoundationAudit.txt`. Its UE-backed structural
  commandlet now rejects wrong-class/corrupt packages, missing or mistyped required `User.*`
  parameters, empty/non-CPU emitter sets, invalid system fixed bounds, and enabled light renderers.
  The current expected result is fallback-ready with `0/8` Niagara masters, not a green
  authored-assets claim; budget, culling, lifecycle, geometry, and readability checks remain manual.
- VFX source inventory: LUDUS has staged `/Game/FX/NS_GhostManifestation`, but it is unaudited,
  unintegrated, and not one of the eight canonical Niagara masters. Six Starter Content and four
  village Cascade systems remain available as seeds; source UE 5.7.4 includes Epic's disabled beta
  Cascade-to-Niagara converter. Every route still requires exact parameters, budgets, canonical paths,
  audit, cooking, and live tests. The Jul 17 foundation audit reports fallback readiness, `0/8`
  canonical masters, and one separately identified non-master candidate.
- Ghost VFX: LMB, RMB, Q, R, and passive cooldown reset are manually accepted. Shift remains readable
  but fails the strict synchronized-movement row because unfocused local windows jitter.
- Eluna VFX: LMB and RMB are accepted. R full-channel revive and range cancellation work, while
  remaining cancellation cases are pending. Q following, carried-wisp smoothness, and Q-started
  resurrection continuity are accepted. Shift refund passes the exact packaged rule but needs one
  owner-HUD/manual presentation retest.
- Kingpin VFX: replicated primitive wedge, tether, trail, impact, chain, shell, and passive fallbacks compile; live owner/observer readability remains unverified.
- Latest main package summary: the July 17 source-toolchain client/server package completed at 03:55;
  `Builds/PlaytestCandidateVerification.txt` passed at 04:21 against the same package. It verifies
  executables, TestMap, 17/17 Hudson cues, handshake 2/2, transport 4/4, exact 2v2 mappings/lifecycles
  4/4, normal/impaired roster smoke 6/6, LMB hits 6/6, non-LMB outcomes 24/24 normally and impaired,
  persistent match reset, same-hunter reconnect, both death/wisp directions, wisp rules 12/12, and
  zero critical/cue-overflow findings.
- Networking/performance cleanup: removed the final legacy DrawDebug multicast API and callers; disabled no-op observer ticks on server-only gameplay actors; made Grappling Hook collision/attachment/destruction server authoritative while replicating latch state for observer visuals; guarded replicated burst fallbacks against pre-initialization world-origin flashes.
- Manual-test tooling: packaged server plus 1-4 visible clients now launch as one recorded session with isolated logs, exact PIDs, safe cleanup, and automatic critical/cue-overflow review under `Saved/Logs/InteractivePlaytest`.
- Focused Eluna retest tooling: `StartElunaRepairRetest.ps1` launches only the required three-client
  topology and writes exact RMB, Shift-wisp, and Q-persistence/cancellation instructions plus a
  three-row result record. Its `-ValidateOnly` path opens no windows.
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
  provisional `Breachborne-Playtest-20260715-041036` client/server bundles were staged without
  archives; independent in-bundle verification passes client 1331/1331 and server 610/610 files
  with zero failures. Final archives and release-candidate naming remain deferred until manual
  acceptance and the freeze are complete.
- Source control: GitHub `main` is the publication target; generated builds/logs remain ignored and
  Unreal binary assets remain managed by Git LFS.
- Editor/toolchain consistency: the tracked VS Code workspace, source-editor/network launchers,
  packaging fallback, and plan commands now use the validated source-built UE 5.7.4 toolchain.
  Opening the generic `5.7` project association in Epic UE reproduces a Build-ID rejection for all
  LUDUS modules; launching the source editor mounts the project plugin without module-load errors.
- LUDUS compatibility: both project and source-engine plugin copies are now `1.0.0` (`Version 28`).
  Its first full project index exposed a null socket in Hudson's imported idle skeleton and rejected
  a valid Fab package path containing an en dash. The Fab folder was renamed through Unreal Asset
  Tools to an ASCII package path; the editor remained responsive beyond both former crash points
  while the 1.0 workspace index ran. The closed-editor repair removed three null sockets, moved the
  Hudson T-pose to its expected package, passed twice, and allowed the temporary source-engine guard
  to be removed. A stock-behavior LUDUS-enabled startup then remained responsive for two minutes with
  no null-socket, StateSync, assertion, or access-violation markers.
- Editor content warnings: LUDUS's full-project scan exposes optional PCG Biome Sample and old
  marketplace demo-map dependencies that are not mounted/present. They do not affect TestMap or the
  packaged candidate. The legacy Hudson T-pose is now stored below `Meshes` as expected by the visual
  set; its old unreferenced redirector package was removed.
- Repository asset policy: `Scripts/SourceControl/TestRepositoryAssetPolicy.ps1` checks that large
  tracked files use Git LFS and that rebuildable Unreal/IDE outputs remain ignored. Generated
  plugin `Binaries` remain ignored; LUDUS 1.0's required 32 MiB document-converter DLL is tracked
  explicitly through Git LFS. The same gate rejects non-ASCII project/plugin package paths so the
  LUDUS 1.0 StateSync assertion cannot be reintroduced by a later content import.

## Gate board

Use `RED`, `YELLOW`, or `GREEN`. A date passing does not make a gate green.

| Date | Gate | Status | Evidence | Blocker/next action |
| --- | --- | --- | --- | --- |
| Jul 12 | Green baseline | GREEN | Checkpoint `d425cd9`; Editor/Game/Server build; PureLogic 44/44; GameSystems 28/28; source and packaged handshakes 2/2 | Complete |
| Jul 13 | Listen-server combat | YELLOW | Packaged smoke selects/spawns all hunters, activates all five inputs, proves LMB damage 6/6, and proves RMB/Shift/Q/R outcomes 24/24 | Interactive movement/aim and owner/observer visual pass pending |
| Jul 14 | Dedicated server | YELLOW | Fresh package: roster activation, exact 2v2 mapping/lifecycle 4/4, LMB 6/6, non-LMB 24/24 normal and impaired, reconnect, both death/revive directions, and wisp rules 10/10 | Deliberate 2v2 movement/combat and ability-specific visual cleanup pending |
| Jul 15 | Packaged networking | YELLOW | 04:07 freshness-aware verifier PASS: executable/map/cue, 2/2 and 4/4 transport, exact 2v2 mapping/lifecycle, normal/impaired roster/outcomes 24/24, reconnect, death/revive, and wisp rules 12/12 | Focused Ghost/Eluna visual retest, deliberate 2v2 combat, and second-machine direct-IP pending |
| Jul 16 | VFX foundation lock | YELLOW | Cue roots, template enum, palettes, parameter schema, budgets, fallbacks, and narrow cue always-cook root locked; all 17 Hudson cue packages present in client IoStore | Eight Niagara master assets and live ability-level cue invocation pending |
| Jul 17 | Ghost VFX | YELLOW | Primitive Ghost Q manually accepted; fresh Ghost outcomes 4/4 with zero cue overflow; Eluna RMB and resurrection continuity accepted; wisp rules 12/12 | Eluna Shift owner-HUD retest, remaining Ghost/Eluna rows, staged animation build/audit, and Low/Medium pass required |
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
| Eluna | PASS | PASS | FAIL | FAIL | - | FAIL | FAIL | - |
| Kingpin | - | - | - | - | - | - | - | - |
| Hudson | - | - | - | - | - | - | - | - |
| Crysta | - | - | - | - | - | - | - | - |
| Void | - | - | - | - | - | - | - | - |

## Manual acceptance ledger

| Track | Accepted | Total | Status | Latest evidence/blocker |
| --- | ---: | ---: | --- | --- |
| Hunter abilities + cleanup/stress | 7 | 41 | PENDING | GE-01/02/04/05/06/07/08 accepted; Shift package repair needs owner-HUD confirmation and broader Q/R/passive rows remain incomplete |
| Range indicators | 0 | 9 | PENDING | `docs/ABILITY_RANGE_INDICATORS.md`; hover and active-preview checks remain manual |
| Wisp UI | 2 | 5 | PENDING | Two-bar layout and natural decay accepted; healing continuity, healing contest, and carried smoothness need repaired-package retest |
| **Presentation total** | **9** | **55** | **PENDING** | Ghost/Eluna 7/12 and Wisp UI 2/5 accepted; range indicators remain 0/9 |
| Second-machine direct IP | 0 | 6 | PENDING | External machine/user required; separate from presentation total |

## Remaining execution lanes

- **Automated Codex lane:** complete. `CompletePostAuthoringValidation.ps1` passes Editor build,
  PureLogic 47/47, GameSystems 55/55, all six discovered `A_Ghost_*` sequence audits, launcher
  contracts, and repository policy. BB-PT-048/049 repair and stock-engine reopen verification pass,
  and the fresh client/server package plus complete evidence refresh pass.
- **Manual local lane:** 46/55 presentation rows remain. The immediate regression is `GE-09` owner
  cooldown/refund-pulse confirmation, followed by the remaining Ghost/Eluna rows, Kingpin/Hudson,
  Crysta/Void, cleanup/stress, range indicators, and wisp UI. Automation can prepare and review these
  sessions but cannot honestly mark subjective owner/observer readability without tester evidence.
- **External lane:** `DI-01` through `DI-06` require a second machine and remote operator. Launch,
  reconnect, evidence capture, and combined review tooling are ready.
- **Optional authored-content lane:** canonical Niagara master completion remains `0/8`; staged LUDUS
  output is reviewed and counted only after path/parameter/budget/cook/live acceptance. Six generated
  Ghost animation sequences pass class/skeleton audits and remain deliberately unbound until the user
  finishes the set and chooses phase mappings.
- **Source-control lane:** after authoring and closed-editor validation, run
  `Scripts/SourceControl/ReviewPendingCommitScope.ps1 -Strict`, refresh every stale staged LFS entry,
  exclude rebuildable/generated noise, review the Ludus plugin update separately, and only then commit
  and push the intentional scope to `github/main`.
- The goal remains active across all lanes. There is no turn-count cap; progress continues wherever
  the required editor state, tester input, or external machine is available.

## Active issue log

| ID | Severity | Area | Description | Owner | Status | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| BB-PT-001 | P0 | Automation | GameSystems access violation at test line 449 | Codex | Resolved | GameSystems 28/28; no crash/ensure on Jul 12 |
| BB-PT-002 | P1 | Packaging | Packaged candidate must match current source | Codex | Resolved | `Scripts/Playtest/VerifyPlaytestCandidate.ps1` PASS; evidence in `Builds/PlaytestCandidateVerification.txt` |
| BB-PT-003 | P1 | Networking | Full ability/death/dedicated/package matrix untested | User + Codex | Automated gameplay/package portion resolved; deliberate combat and presentation cleanup remain open | July 15 candidate: 2/2 and 4/4 transport, exact 2v2 mapping/lifecycle 4/4, normal/impaired roster, LMB 6/6, non-LMB 24/24, reconnect, both death/revive directions, and wisp rules 12/12 PASS; total manual presentation is 8/55 |
| BB-PT-004 | P2 | GameplayCues | No targeted GameplayCueNotifyPaths configured | Codex | Resolved | `/Game/GameplayCues` configured; clean Jul 12 editor startup initialization |
| BB-PT-005 | P2 | Environment | UDP Messaging cannot bind stale adapter 25.18.80.222 | Codex | Resolved | Current machine owns no 25.x adapter. `UnicastEndpoint=0.0.0.0:0` follows UE 5.7.4's documented default-adapter behavior, stale static endpoints are cleared, and the Jul 15 source-editor restart bound successfully on an ephemeral port; GameNetDriver remains separate. |
| BB-PT-006 | P0 | Wisp | PvP death did not spawn or possess a wisp because GameMode lifecycle wiring was absent | Codex | Resolved | User retest plus `Saved/Logs/Breachborne-backup-2026.07.12-00.26.01.log`: three successful spawn/possession/revive cycles |
| BB-PT-007 | P1 | VFX/Networking | Rapid ability cues exceeded the per-net-update GameplayCue multicast limit | Codex | Automated regression resolved; manual Hudson sustained-fire/readability retest pending | Fire/Impact/Heal use one batched Hudson multicast; redundant Kingpin passive and Crysta R impact cue RPCs removed; every final packaged review reports zero cue-overflow findings |
| BB-PT-008 | P1 | Wisp UI | Wisp widgets constructed and bound on every client but rendered no bars because the tree was created after `RebuildWidget` | Codex | Fix compiled; visual retest pending | Jul 12 repeated-session log; no paint path existed |
| BB-PT-009 | P1 | Ability UI | Primitive range previews need a six-hunter owner/observer readability pass | Codex | Fix compiled; visual retest pending with launcher ready | `StartRangeIndicatorAcceptance.ps1` validates a four-client, two-game six-hunter topology, creates the exact nine-row record, and documents all five built-in F/G power commands; `-ValidateOnly` passes without launching processes. A real run remains package-freshness gated and no RI row is promoted from tooling alone. |
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
| BB-PT-025 | P1 | Void VFX Fixture | The frozen TestMap package had no placed prop with `UBBVoidSwappableComponent`, so marked-prop visual swapping could not be observed | Codex | Resolved; manual visual execution pending | `ABBTargetDummy` owns a default marker and replicates actor movement from its class defaults. The focused fixture test and full GameSystems 54/54 pass, and the Jul 17 client/server package cooked the updated `TestMap`; the marked-prop subcase is no longer setup-blocked. |
| BB-PT-026 | P1 | Manual Tooling | The documented two-client pair sessions could not exercise Eluna's ally mechanics or Kingpin's two-target chain | Codex | Resolved in tooling; execution pending | Three-client Ghost/Eluna and Kingpin/Hudson wrappers define exact teams, slots, hunters, evidence, candidate preflight, and safe stop/review. Ghost/Eluna now creates separate 12-row hunter and five-row wisp ledgers in the shared topology. |
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
| BB-PT-037 | P1 | Movement/Networking | Remote characters and abilities are smooth in the focused local window and equally jittery in both unfocused packaged windows | Codex | Engine/editor focus behavior isolated; separate-machine confirmation required | UE 5.7.4 source sets editor `bThrottleCPUWhenNotForeground=true` and disables background viewport rendering, directly explaining multi-PIE behavior. Packaged defaults are `t.IdleWhenNotForeground=0` and `r.VSync=0`, so packaged clients are not intentionally idled; three windows on one GPU/desktop remain a presentation-contention test, not valid replication evidence. Compare focused clients on separate machines during the weekend smoke. Eluna Q/carried jitter reproduced in focused windows and is tracked separately in BB-PT-044. |
| BB-PT-038 | P1 | Networking/Readiness | Under packet loss, a client could bind its PlayerState ASC but never receive the initial owner ability-spec burst | Codex | Resolved and packaged | Recurrences: Void `20260714-133518`, Crysta `20260715-032515`. Completed grants are now re-published immediately and after owner-channel settling, with smoke preparation as a recovery point. Three focused impaired Crysta/Void runs (`033643`, `033732`, `033822`) and the full impaired roster (`034252`, `034341`, `034430`) pass. |
| BB-PT-039 | P1 | Crysta R | A predicted client could end Crysta R before the server's first delayed salvo; the zero-delay first timer also never fired | Codex | Resolved and packaged | Non-authority now waits for server-owned completion and salvo zero fires immediately. Three focused reruns passed, then current normal and impaired 24/24 outcome suites passed at `PackagedOutcomeSmoke/20260714-142409` and `PackagedOutcomeNetworkImpairment/20260714-143034`. |
| BB-PT-040 | P1 | Ghost Cooldowns/Passive | Ghost RMB/Shift/Q/R used obsolete transient GameplayEffects, allowing Shift to activate twice inside its six-second cooldown and leaving the kill passive nothing to reset | Codex | Resolved and manually accepted | Live failure at `InteractivePlaytest/20260715-001027`; focused retest `20260715-014500` accepted cooldowns/reset and logged one eligible removal with zero cue overflow. |
| BB-PT-041 | P1 | Wisp Healing | Healing-started revival stopped when the heal source expired instead of continuing until enemy contest | Codex | Gameplay continuity manually accepted; combined UI row still pending | Failure reached 76-80% then stopped in `20260715-014500`. Persistent latch plus enemy cancellation pass packaged scenarios, and the Jul 17 tester confirmed Q-started resurrection now completes. `WI-03` remains unchecked until its ally/healing owner-observer bar coverage is complete. |
| BB-PT-042 | P1 | Eluna RMB | Sticky projectile passed through or repeatedly rubberbanded around its first enemy before the delayed root expanded | Codex | Resolved and manually accepted | Proxy simulation is disabled and stuck target/offset replicate explicitly. Packaged outcome contracts pass 4/4, and the Jul 17 focused tester accepted the one-time latch with no forward/back loop. |
| BB-PT-043 | P1 | Eluna Shift | Dash refund failed to detect and collect an allied wisp along the projected path | Codex | Packaged repair passed; owner-HUD manual retest pending | Jul 17 authority logged collection while the owner retained its predicted cooldown and reported ground Shift blocked/aerial not refunded. Refund now removes the matching authoritative effect and explicitly clears the owning client's predicted copy. GameSystems 55/55 proves one charge is refunded without clearing the other. Fresh packaged wisp evidence `20260717-040413` passes 12/12 and records `GA_Eluna_AerialDash` removing exactly one seeded cooldown effect. |
| BB-PT-044 | P1 | Eluna Q/Carry Networking | Q and carried wisps were server-teleported at 20 Hz/5 Hz, producing focused-client jitter and model/camera bouncing | Codex | Resolved and manually accepted | Q uses replicated target attachment and carried wisps attach once. Focused retest `20260715-014500` accepted smooth Q following and substantially improved carried-wisp movement. |
| BB-PT-045 | P1 | Ghost LMB Networking | Three held-fire ticks exceeded GAS's two-GameplayCue-RPC per-net-update budget | Codex | Packaged regression resolved; manual readability retest optional | Owner predicts the muzzle cue locally and observers use the replicated projectile. Every current-package review reports zero GameplayCue-overflow findings. |
| BB-PT-046 | P1 | Hudson Q/Networking | Hudson Q preferred actor facing over replicated aim, allowing late movement correction under 100 ms/2% loss to place barbed wire away from the target | Codex | Resolved and packaged | Q now uses `GetAimDirection()` with actor facing only as fallback. Focused normal and impaired reruns pass 4/4; full current-package outcome matrices pass 24/24 in both profiles. |
| BB-PT-047 | P1 | Editor/Toolchain | VS Code launched Epic UE 5.7 against project and LUDUS DLLs built by source UE 5.7.4, causing a missing/different-engine module dialog | Codex | Resolved in tracked launch configuration | Build IDs proved the mismatch (`47537391` versus `6fb450cd...`); source UE 5.7.4 now launches successfully and mounts the project-local LUDUS plugin. |
| BB-PT-048 | P1 | Editor/LUDUS | LUDUS 1.0 full-project indexing crashed on a Hudson skeleton null socket and asserted on a Unicode Fab package path | Codex | Resolved in portable project content | The guarded runner renamed the U+2013 Fab folder to `Stylized_Medieval_Well_Low_Poly_3D_Asset`, removed three null sockets from `/Game/Hunters/Hudson/Hudson_Idle/SkeletalMeshes/Hudson_Idle_Skeleton`, and passed two idempotency passes. The temporary engine guard was removed and rebuilt; stock-behavior LUDUS startup remained responsive for two minutes with no former crash markers. |
| BB-PT-049 | P2 | Editor/Content | Full-project indexing reports stale optional sample dependencies and a legacy Hudson T-pose package path | Codex | Hudson path resolved; optional demo warnings triaged | PCG Biome Sample is disabled, VRS demo maps reference removed engine sky content, and neither is in the playtest map/candidate path. The compatibility runner moved the real T-pose to `/Game/Hunters/Hudson/Meshes/Hudson_TPOSE`, confirmed no old-package referencers, removed the redirector, and passed a second no-op pass. |
| BB-PT-050 | P1 | Ghost Q VFX | LUDUS described railgun telegraph/fire systems but did not save either named Niagara asset | Codex + User | Primitive integration manually accepted; authored Niagara assets pending | Ghost Q sends exact start/end geometry through cue `Location`, `Normal`, and `RawMagnitude`; native cue parents write `User.BeamStart`, `User.BeamEnd`, direction, length, and lifetime. Both cue Blueprints are generated and the Jul 17 tester accepted warning timing, origin, and shot behavior with the primitive fallback. The bootstrap still refuses final `READY` until both named Niagara systems exist. |
| BB-PT-051 | P1 | Manual Tooling | A relative interactive-playtest output root made packaged Unreal resolve `-abslog` below each executable, leaving the central session without logs to review | Codex | Resolved and regression-tested | The launcher canonicalizes and creates the output root before process launch. The reviewer recovered the stopped Jul 17 session's 4/4 legacy split logs and records their sources; `TestInteractivePlaytestReviewer.ps1` passes a two-tree synthetic recovery 2/2. |
| BB-PT-052 | P1 | VFX Fixture/Networking | A stale placed target-dummy instance has no root or native components but inherited replication, flooding server relevancy warnings after clients join | Codex | Resolved in fresh package | The active Jul 17 session emitted over 33,000 `BBTargetDummy_0 has no root component` warnings with zero critical findings, while valid `BBTargetDummy_1` initialized normally. Malformed legacy instances are now rejected before replication; Editor build and GameSystems 55/55 pass, and fresh packaged wisp evidence `20260717-024548` has no root-component warning. |
| BB-PT-053 | P1 | Ghost Q/GameplayCues | Q Fire, per-hit Impact, and lethal passive pulse competed for GAS's two cue multicasts per net update | Codex | Resolved in focused package | Jul 17 review recovered 4/4 logs and found five overflows: one passive pulse and four Q impacts. Fire already carries the authoritative beam endpoint and the replicated burst supplies fallback impact, so the redundant Impact multicast was removed. Fresh Ghost-versus-Eluna packaged outcome evidence `20260717-024716` passes 4/4 with zero critical and zero GameplayCue-overflow findings. |
| BB-PT-054 | P1 | Editor/Toolchain | Source-editor startup walked into optional plugins whose modules were absent from the partial 5.7.4 engine build | Codex | Resolved in launch tooling | `PlatformCrypto` and `ChaosCloth` dialogs were unrelated to LUDUS and both plugins were already disabled in the project. `StartSourceEditor.ps1` now quotes the project path, derives every disabled plugin from the `.uproject`, guards against duplicate editors, and supports dry runs; VS Code launch profiles carry the same explicit list. PowerShell/JSON/diff checks pass and PID 21892 remains responsive with LUDUS enabled. |
| BB-PT-055 | P1 | Animation/Content Safety | Generated animations could be assigned without enforcing the code-owned skeleton, slot, root-motion, phase, or loop contract | Codex | Code-owned safety and asset audits resolved; phase assignment deferred | Ghost routes LMB/RMB/Shift/Q/R/passive phases through C++ and replicated `UBBAbilityVisualSet`. Runtime rejects skeleton/root-motion mismatches and validation enforces slot, phase, loop, and play-rate contracts. PureLogic 47/47 passes, and all six `A_Ghost_*` assets audit as `AnimSequence` on `/Game/Characters/SK_GhostSpecOps_Skeleton`. They remain deliberately unbound until the user finishes the animation set and chooses mappings. |
| BB-PT-056 | P1 | Source Control | The repaired VS Code source-editor launch profile was hidden by the blanket `.vscode/` ignore rule | Codex | Resolved and policy-tested | Only `.vscode/launch.json` is now eligible for source control; generated compile databases and IntelliSense files remain ignored. Repository policy now includes untracked/pending files and passes with zero large files outside LFS, rebuildable ignores PASS, LFS attributes PASS, and printable ASCII package paths PASS. All current Ghost animation, VFX, and reference-image paths resolve to LFS without staging. |
| BB-PT-057 | P1 | Editor/Automation | Closed-editor build, test, animation-audit, and repository gates were separate manual commands with no aggregate result | Codex | Resolved and executed | `CompletePostAuthoringValidation.ps1` passed at `Saved/Logs/PostAuthoringValidation/20260717-035018`: launcher contracts 7/7, ledgers 6/6, Editor build, PureLogic 47/47, GameSystems 55/55, all six discovered Ghost animation audits, and repository policy. It stops on first failure and writes one durable summary. |
| BB-PT-058 | P1 | Source Control | Assets staged before later LUDUS edits could leave the commit index pointing at older LFS objects, while plugin-update churn and authoring references need separate scope decisions | Codex | Review tooling resolved; final staging pending authoring close | `ReviewPendingCommitScope.ps1` parses NUL-delimited Git status safely, groups source/tooling, project content, Ludus plugin files, and authoring references, verifies pending binary LFS attributes, and flags staged-then-modified paths. The latest Jul 17 live review still finds eight stale index entries and zero pending binary paths without LFS attributes; strict mode intentionally remains red until final staging. |
| BB-PT-059 | P1 | Manual Tooling | Range, cleanup/stress, and wisp UI batches had checklist rows but no complete launcher-backed record topology | Codex | Resolved in tooling; manual execution pending | `StartRangeIndicatorAcceptance.ps1` validates a four-client two-game six-hunter/power run at UDP 7914. `StartCleanupStressAcceptance.ps1` validates normal and 100 ms/2% loss four-client modes at UDP 7915, including Low/Medium and Grappling Hook instructions. Ghost/Eluna creates both hunter and WispUI records. Launcher contracts pass 7/7 and ledger generation 6/6 against the fresh verified package. No visual row is promoted by validation. |

## Daily updates

### 2026-07-17

- Closed-editor post-authoring validation PASS: Editor build, PureLogic 47/47, GameSystems 55/55,
  six Ghost animation audits, launcher/ledger contracts, and repository policy.
- BB-PT-048/049 repair PASS twice. Three Hudson skeleton null sockets were removed, the Unicode Fab
  package path was replaced with ASCII, and Hudson T-pose moved below `Meshes` without referencers at
  the old path. The temporary engine guard was removed; LUDUS-enabled stock startup survived two
  minutes with no former assertion/crash markers.
- Fresh client/server package completed at 03:55 and the complete package evidence refresh/verification
  passed at 04:21 with zero critical or GameplayCue-overflow findings.
- Gate due: focused Ghost Q cue/origin and Eluna RMB/Shift/Q owner-observer retest.
- Status: YELLOW; the recorded three-client session is active at
  `Saved/Logs/InteractivePlaytest/20260717-022200`.
- Source verification: BreachborneEditor build PASS; PureLogic 44/44 PASS; initial GameSystems run
  exposed that the new target-dummy fixture deferred replication until BeginPlay. Replication is
  now a class default; the focused regression and full GameSystems 54/54 pass with no crash/ensure.
- Ghost Q integration: generated and saved `GC_Hunter_Ghost_Q_Telegraph` and
  `GC_Hunter_Ghost_Q_Fire`; cue bootstrap emitted `BB_GHOST_Q_VFX|CUES_READY`. Final `READY` remains
  correctly blocked because LUDUS has not saved `NS_GhostRailGunTelegraph` or
  `NS_GhostRailGunLaser`.
- Package verification: the earlier focused Development package completed at 02:20 and was superseded
  by the fully refreshed 03:55 client/server candidate and 04:21 complete verifier PASS.
- Fixture closure: BB-PT-025 is no longer setup-blocked. The updated native target dummy is tested,
  replicated, movable, and cooked with the current `TestMap`; only the corresponding manual Void
  Shift presentation row remains to execute.
- Evidence tooling: BB-PT-051 is fixed. Future sessions pass absolute log paths to packaged Unreal;
  the reviewer recovered the stopped session's four executable-relative logs. The synthetic reviewer
  recovery passes 2/2 with zero critical or cue-overflow findings.
- Fixture warning flood: BB-PT-052 was discovered from the live server log. Correctness/visibility
  observations remain useful, but that run is not accepted for fine-grained smoothness because the
  stale rootless dummy wrote repeated relevancy warnings. The source guard passes the clean editor
  build and GameSystems fixture; fresh packaged evidence `20260717-040413` proves the warning absent.
- Manual result: Ghost Q's primitive warning/origin/shot behavior looked correct; Eluna RMB is accepted
  as GE-08; Eluna Q-started resurrection now completes; Eluna Shift collection occurred but its owner
  retained the predicted cooldown. Review recovered 4/4 split logs, recorded 3/3 joins, found zero
  critical errors, and found five Ghost Q/passive cue overflows.
- Repair verification: the owning client now receives an explicit same-tag cooldown refund, the packaged
  wisp scenario seeds and requires removal of that cooldown, and redundant Ghost Q Impact cue traffic is
  folded into the endpoint-aware Fire effect plus replicated fallback burst. Editor build, PureLogic
  47/47, and GameSystems 55/55 pass. Fresh wisp evidence `20260717-040413` passes 12/12 with one exact
  aerial-dash cooldown removed; focused Ghost outcome evidence `20260717-024716` passes 4/4 with zero
  critical or cue-overflow findings. GE-09 remains open only for visible owner cooldown/refund feedback.
- Source-editor launcher: BB-PT-054 is closed. Optional source-engine plugins with no built module no
  longer block LUDUS startup; the canonical launcher derives the disabled list from `Breachborne.uproject`,
  and both VS Code launch profiles include the same defensive override.
- Code-owned animation safety: BB-PT-055 build, automation, and six asset audits pass. Ghost phase
  routing is covered by lookup/validation tests, bad montage skeleton/root-motion data is rejected before
  playback, and all generated Ghost sequences remain staged rather than integrated pending user mapping.
- Source-control policy: BB-PT-056 is closed. The source-editor launch profile is now the only tracked
  VS Code file; generated IDE data remains ignored. The full repository asset-policy gate passes and
  checks pending/untracked files as well as tracked files, confirming current Ghost binary/source-media
  additions use Git LFS rather than normal Git blobs.
- Post-authoring automation: BB-PT-057 is executed and green. The aggregate command passed launcher
  contracts, isolated automation, automatic `A_Ghost_*` discovery/audits, repository policy, and one
  final summary.
- VFX candidate accounting: `AuditVfxFoundation.ps1` remains fallback-ready with `0/8` canonical
  masters and inventories `/Game/FX/NS_GhostManifestation` separately. Non-master `NS_*` files
  cannot advance authored-master completion by filename coincidence, and canonical files no longer
  count as structurally valid until the UE commandlet loads and inspects them.
- Pending commit safety: BB-PT-058 adds a read-only scope review. The live tree currently has eight
  staged-then-modified Ghost weapon/texture assets that must be refreshed after authoring, while all
  pending binary paths resolve to LFS. Plugin-update files and authoring references are reported in
  separate groups instead of being swept into a blanket commit.
- Range-indicator setup: BB-PT-009 now has a dedicated four-client launcher and generated nine-row
  record. Validation passes for UDP 7914 and the packaged client/server paths without starting a
  process; the real session is now freshness-ready against the verified 03:55 package.
- Manual batch closure: BB-PT-059 provides the missing cleanup/stress launcher in normal and
  100 ms/2% loss modes, while Ghost/Eluna now creates a separate five-row WispUI ledger. Together
  with the pair and range wrappers, all six definition batches have an exact record-generation path.
- Plan/tooling contract refresh: manual acceptance definition PASS with 55/55 unique rows and exact
  documentation agreement; direct-IP reviewer PASS 4/4 with DI-01 through DI-06 unique; distribution
  verifier PASS 8/8. These prove the runbooks/evidence contracts, not the still-unexecuted manual rows.
- Manual launcher regression: `TestManualAcceptanceLaunchers.ps1` now runs every acceptance wrapper in
  a child PowerShell, covers normal and impaired cleanup modes, detects any new Unreal/game process or
  session artifact, and validates all six ledger generators in a disposable folder. This keeps the
  complete manual topology testable while the live editor remains untouched.
- Crysta/Void fixture instructions: the pair launcher no longer labels marked-prop swapping setup
  blocked. It directs the tester to the placed target dummy added by BB-PT-025 and still requires
  matching owner/observer endpoints before the visual row can pass.
- Focused-retest provenance: future Eluna repair records now link the candidate-verification evidence
  used by their launch instead of stamping the obsolete July 15 package timestamp.

### 2026-07-15

- Gate due: tester-controlled Ghost/Eluna owner-observer acceptance.
- Status: YELLOW.
- Session launched: `Saved/Logs/InteractivePlaytest/20260715-001027`; frozen candidate preflight
  PASS before launch. Client 1 is Eluna on Team 0/Slot 0, client 2 is allied Hudson on Team 0/Slot
  1, and client 3 is opposing Ghost on Team 1/Slot 0. Storm must be disabled before match start.
- Optional authoring route: LUDUS AI 1.0 early access is installed and may be evaluated after the technical/manual
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
- Final candidate refresh: client/server rebuilt at 03:36; Editor, PureLogic 44/44, and GameSystems
  53/53 pass. The 04:07 verifier passes every packaged gate: normal/impaired roster 6/6, reset,
  reconnect, both death/wisp directions, wisp rules 12/12, LMB hits 6/6, normal/impaired outcomes
  24/24, four-client transport/mapping/lifecycle, and zero critical/cue-overflow findings.
- Ability readiness repair: an impaired Crysta run reproduced BB-PT-038. The authoritative ASC now
  re-publishes its completed granted-spec list after owner-channel settling. Three focused impaired
  Crysta/Void runs and the complete impaired roster pass on the rebuilt package.
- Provisional distribution: staged the verified client and server under
  `Builds/Distribution/Breachborne-Playtest-20260715-041036` without archives. Manifest verification
  passes all 1331 client files and 610 server files with zero failures. This proves the transfer
  layout and hashes; it is not the final release candidate while manual acceptance remains open.
- VFX prompt handoff: locked LUDUS/manual Niagara prompts for all eight masters and six hunters in
  `docs/plans/ludus-vfx-prompt-pack.md`, including exact paths, `User.*` parameters, budgets,
  cleanup, and review evidence. No generated asset is claimed; the authored audit remains `0/8`.
- Manual retest handoff: added `StartElunaRepairRetest.ps1` so tomorrow's three repaired behaviors
  have exact roles, ordered actions, PASS conditions, and a dedicated result record without asking
  the tester to repeat the already accepted Ghost/Eluna rows.
- Editor/toolchain repair: corrected all tracked VS Code engine paths plus source editor, headless,
  local-network, packaging, and plan defaults to source UE 5.7.4. Workspace JSON and six affected
  PowerShell scripts parse cleanly. The already-running source editor stayed responsive; no build,
  package, PIE session, or content asset was touched during the repair.
- Source-control audit: all `1,630` tracked binary assets are in Git LFS; every tracked local file
  at or above `5 MiB` is LFS-backed. Rebuildable output roots remain ignored. Added a repeatable
  policy script so later LUDUS/Niagara imports fail review if a large asset bypasses LFS.
- VFX content inventory: a binary/name scan found zero Niagara systems and ten existing Cascade
  systems. Verified UE 5.7.4's converter plugin and Content Browser conversion action in engine
  source, then documented exact seed mappings and post-conversion requirements in the LUDUS prompt
  pack. The plugin remains disabled to avoid restarting the user's active editor session.
- UDP Messaging repair: replaced stale, unavailable `25.x` adapter/static endpoints with UE's
  documented `0.0.0.0:0` default-adapter bind and an explicit empty static-endpoint array. The Jul
  15 source-editor restart bound successfully on an ephemeral port with no stale-adapter warning;
  this does not affect gameplay direct-IP on UDP 7777. BB-PT-005 is closed.
- LUDUS 1.0 startup repair: updated both plugin copies to `1.0.0`, rebuilt against source UE 5.7.4,
  and repaired the Unicode Fab package path that caused `StateSyncPath.cpp:94` to assert. A local
  engine guard identified and bypasses a null socket in Hudson's idle skeleton; the editor remains
  responsive and LUDUS writes its 1.0 workspace database. Resave a repaired skeleton before calling
  this portable or removing the guard.
- LUDUS warning triage: the full index reports disabled PCG sample content and old VRS demo-map sky
  references; neither is used by the playtest candidate. A separate Hudson warning is a real but
  simple path mismatch: the mesh exists at `/Game/Hunters/Hudson/Hudson_TPOSE`, while the legacy
  visual set expects `/Game/Hunters/Hudson/Meshes/Hudson_TPOSE`. The closed-editor compatibility
  pass now performs that Asset Tools rename before the unguarded launch verification.
- Focus-jitter research: source UE 5.7.4 defaults editor background CPU throttling on and explicitly
  disables background viewport rendering, which explains multi-PIE windows becoming uneven. The
  packaged runtime defaults `t.IdleWhenNotForeground=0` and `r.VSync=0`, so same-machine packaged
  degradation is not an intentional game-loop sleep. Treat three local windows as a transport and
  rough visual test; judge movement replication only with each client focused on its own machine.
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
