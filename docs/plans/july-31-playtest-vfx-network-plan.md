# July 31 Playtest: VFX, Networking, and Test Plan

## Document role

This is the source of truth for preparing the Breachborne July 31, 2026 playtest.
It is intentionally self-contained so a new Codex conversation or another agent can
continue without relying on chat history.

Use `docs/plans/july-31-playtest-status.md` for daily progress, failures, and evidence.
Do not silently change a locked decision in this document. Record the proposed change,
reason, cost, and deadline impact in the status tracker first.

Last audited: July 12, 2026, Europe/Bucharest.

## Objective

Deliver a multiplayer Windows playtest on July 31 with six selectable hunters:

- Ghost
- Eluna
- Kingpin
- Hudson
- Crysta (hunter ID 5)
- Void (hunter ID 6)

The playtest is primarily for validating combat mechanics, multiplayer behavior, and
ability readability. It is not a production-art milestone.

Success means:

- Two to four Windows clients can join by direct IP and enter a combat session.
- All enabled hunters can activate their abilities without crashes or corrupt state.
- Damage, healing, crowd control, movement, projectiles, zones, and cooldowns agree
  between the server, owning client, and observing clients.
- Every dangerous or important ability has a readable warning, active state, and
  resolution using Niagara or a primitive fallback.
- A fresh packaged release candidate is tested using the exact July 31 topology.
- The team has at least one week between feature freeze and the playtest for testing,
  repair, and one genuine redo cycle.

## Locked scope decisions

- Deadline: July 31, 2026. This does not move to preserve optional polish.
- Feature and bespoke VFX freeze: end of July 23.
- Release candidate freeze: July 30 after verification.
- Shared character animations are acceptable for all hunters and skills.
- Character models may be the same base model with clearly different recolors.
- Ability readability comes from shape, timing, motion, hitbox representation, sound
  stubs, and color. Color alone is not sufficient.
- Use reusable Niagara templates plus primitive mesh fallbacks. Do not build 30 fully
  independent effects.
- Author and test at 1080p Low/Medium first. High/Epic polish is out of scope.
- Direct IP is the required connection flow. Matchmaking, accounts, and production EOS
  integration are out of scope.
- Dedicated server is preferred, but a listen server is an accepted deadline fallback.
- If the full match loop is unstable, the playtest may use a controlled TestMap combat
  session with manual resets.
- A broken hunter may be disabled after July 27 rather than threatening the entire build.
- No animation expansion, progression redesign, major UI overhaul, final audio pass,
  anti-cheat project, or unrelated refactor before the playtest.

## Current evidence and risks

### What is working

- UE version is pinned to 5.7.
- Client, Editor, and Server targets exist.
- Niagara, GameplayAbilities, GameplayTags, GameplayTasks, NetCore, and UMG are enabled.
- ASC lives on PlayerState and uses mixed replication.
- Hunter selection, player data, aim location, ability state, world gameplay actors,
  and several movement systems contain replication support.
- Local networking scripts exist under `Scripts/Networking/`.
- Packaging scripts exist under `Scripts/Packaging/`.
- The editor settings currently use a two-player listen-server PIE session on port 17777.
- A July 12 listen-server log proves two clients connected, selected Crysta and Void,
  readied, entered countdown, and spawned into gameplay.
- The latest C++ build after the Crysta/Void fixes succeeded.
- `Breachborne.PureLogic` ran July 12: 44 passed, 0 failed.

### Remaining risks

- An opt-in packaged multiplayer smoke now proves lobby flow, hunter selection/grants, initial
  LMB/RMB/Shift/Q/R activations, held-input releases, Q recasts, and cleanup for IDs 1-6. It does
  not prove damage/CC outcomes, visual replication/readability, or hitbox alignment.
- The current packaged candidate is fresh and passes normal plus 100 ms/2% loss automated
  activation gates. An isolated current-source package also passes the lethal GAS damage through
  wisp possession lifecycle. Interactive ability damage/CC, persistent-ability death cleanup,
  second-machine, visual, and revive evidence remains outstanding. A reconnect attempt reaches the
  server, but the replacement is a spectator instead of regaining the disconnected hunter.
- The green hunter/shared-VFX baseline and full-roster smoke harness have named checkpoints; keep
  subsequent green network batches checkpointed before broad manual testing.
- The eight reusable Niagara master assets remain editor work. Compiled primitive fallbacks
  cover the roster and are the accepted deadline fallback.
- A UDP Messaging plugin socket bind error exists for `25.18.80.222`. GameNetDriver still
  connects successfully, so treat it as a separate environment warning unless it blocks
  the direct-IP test.
- Wisp bars, range indicators, Hudson sustained fire, and every hunter's owner/observer
  readability still require the manual acceptance matrix.

## Relevant architecture

### Gameplay and cosmetic separation

- Gameplay remains server authoritative.
- LocalPredicted abilities may provide immediate owner feedback, but the server owns
  damage, healing, CC, movement corrections, spawn decisions, and persistent state.
- GameplayCues own client-side one-shot and looping cosmetics.
- Do not replicate Niagara simulations or send a multicast every tick.
- A moving projectile/zone that already replicates may own an attached Niagara or mesh
  component. Its gameplay collision and state remain authoritative.
- Persistent GameplayCues must have explicit add/remove paths and cleanup on ability end,
  actor destruction, death, cancellation, and timeout.

### Existing visual infrastructure

- `Source/Breachborne/Abilities/BBGameplayTags.h/.cpp`
  declares GameplayCue tags for every hunter.
- `Source/Breachborne/Abilities/BBGameplayAbility.h/.cpp`
  provides `ExecuteVisualCue`, `AddVisualCue`, and `RemoveVisualCue`.
- `Source/Breachborne/Abilities/BBAbilityVisualSet.h/.cpp`
  defines per-hunter ability visuals, Niagara/Cascade/mesh/sound references, sockets,
  montage phases, and low-spec budgets.
- `Source/Breachborne/Characters/HunterCharacter.h/.cpp`
  replicates the hunter visual set and resolves configured montage phases.
- Hudson has the current complete GameplayCue content pattern under
  `Content/Hunters/Hudson/Cues/` plus `DA_Hudson_VisualSet`.
- Crysta and Void currently use replicated primitive gameplay actors and can retain them
  as low-quality and emergency fallbacks.
- `Source/Breachborne/Combat/BBPrimitiveVisuals.*` centralizes primitive mesh coloring.
- `Source/Breachborne/Combat/BBPrimitiveBeamActor.*` supplies a replicated beam fallback.
- Legacy DrawDebug multicast RPCs were removed after bounded primitive fallbacks replaced them.
- Server-only gameplay actors disable their ticks on observer clients where replication already
  carries movement and visual state; locally animated visual actors retain client ticks.
- Grappling Hook power collision, latch, range, pull, and destruction resolve only on the server;
  attachment state replicates so owner and observer chain visuals use the authoritative endpoint.
- Replicated primitive bursts remain hidden until initialized state arrives, avoiding a transient
  white fallback sphere at world origin.
- Native hunter definitions cover all six roster IDs, so a missing or unloadable optional
  DataAsset cannot make Ghost or Eluna unselectable on dedicated/package test paths.

### GameplayCue content path task

By July 16, configure targeted cue scanning in `Config/DefaultGame.ini` under
`[/Script/GameplayAbilities.AbilitySystemGlobals]`, using the actual final content roots,
for example `/Game/Hunters` and `/Game/Powers`. Verify cue discovery after cooking.

Status on July 12: engineering configuration is complete for `/Game/GameplayCues`,
`/Game/Hunters`, and `/Game/Powers`. The compatibility roots preserve existing Hudson assets.
`/Game/Hunters/Hudson/Cues` is also a narrow `DirectoriesToAlwaysCook` root. Archive inspection
confirmed all 17 Hudson cue packages in the current client IoStore; live packaged ability-level
cue invocation still needs the manual combat pass.

## Ability readability contract

Every important ability communicates three stages:

1. Warning: location, direction, area, and enough timing to react when gameplay permits.
2. Active: the actual projectile, beam, tether, cone, path, or zone.
3. Resolution: impact, detonation, heal, stun, root, pull, swap, expire, or cancel.

Rules:

- Visible width/radius/length must match gameplay dimensions closely.
- Enemy outer rims use a red warning treatment; friendly outer rims use blue/cyan.
- Preserve the hunter identity color inside the effect.
- Do not rely only on red versus green. Enemy telegraphs should also pulse/dash differently
  from friendly solid rings.
- Empowered effects change at least two properties: size, secondary shell, brightness,
  density, pulse rate, or shape. A color swap alone is not enough.
- LMB and passive polish are lower priority than ultimates, persistent zones, CC, hooks,
  pulls, swaps, and revive effects.
- Persistent effects require bounded lifetime, explicit stop, distance culling, and an
  active-instance budget.

## Hunter identity directions

Lock final numeric colors on July 16. Current direction:

- Ghost: electric green/cyan with white ballistic flashes.
- Eluna: moon-white and sky blue.
- Kingpin: red and amber with heavy dark silhouettes.
- Hudson: industrial orange and steel blue.
- Crysta: cyan and magenta; empowered state uses gold/white.
- Void: violet with acid green/teal; empowered state uses hot pink/white.

Final numeric colors, team-relation treatments, and authoring rules are locked in
`docs/VFX_FOUNDATION.md`.

## Reusable VFX templates

Build approximately eight low-cost master systems, then configure per-skill instances:

1. Projectile body
2. Projectile/ribbon trail
3. Beam/tether
4. Ground ring/telegraph
5. Persistent ground zone
6. Cone/wedge
7. Impact/detonation burst
8. Character aura/status marker

The C++ template enum, palette data, parameter defaults, budgets, asset paths, and primitive
fallback contract are implemented. The eight Niagara master `.uasset` systems remain editor work.

Required user parameters where applicable:

- `PrimaryColor`
- `SecondaryColor`
- `TeamRelationColor`
- `Radius`
- `Length`
- `Width`
- `Direction`
- `Lifetime`
- `Progress`
- `Intensity`
- `bEmpowered`
- `bEnemy`

Performance baseline:

- Short bursts over dense persistent particles.
- Prefer unlit meshes, decals, ribbons, and sprites.
- No Niagara fluids or heavy GPU simulation.
- Persistent effects get Low/Medium variants, max draw distance, culling, instance caps,
  and explicit shutdown.
- Test worst-case overlap from multiple hunters, not only isolated effects.

## Per-hunter VFX specification

### Ghost

- LMB: compact rifle tracer, muzzle flash, directional impact sparks.
- RMB: spiked grenade silhouette, visible flight trail, expanding detonation ring.
- Shift: low horizontal streak close to the ground and brief departure afterimage.
- Q: thin grey aim indicator where appropriate, then a very fast bright piercing beam
  with separate impact flashes on every target.
- R: clear warning circle, descending strike marker, then orange burning floor zone with
  a pulsing boundary.
- Passive: brief reset pulse when a kill refreshes Shift.

### Eluna

- LMB: crescent projectile; held stacks increase brightness/density as damage rises and
  range falls.
- RMB: slow dark sticky orb, heavy trail, delayed circular warning, then vertical binding
  bands/pillars showing the root area.
- Shift: pale lunar ribbon/crescent departure burst; extra ally-colored pulse on a valid
  ally pass-through refund.
- Q: healing disc with exact radius, rhythmic heal pulses, visible travel when tossed,
  and a large final heal burst.
- R: persistent Eluna-to-target beam, particles moving toward target, increasing channel
  intensity, success/revive burst, and clear cancellation cleanup.
- Passive: quiet radial healing pulse and visible carried-wisp state.

### Kingpin

- LMB: broad melee swipe; a visible chain arc links the primary and secondary target.
- RMB: hook head plus persistent chain/tether, latch flash, and pull streak.
- Shift: heavy forward wedge, dust trail, shoulder impact and knockback streak.
- Q: short cone telegraph, overhead strike flash, ground shock/crack, and stun marker.
- R: two individually readable shotgun flashes and broad pellet cones; affected targets
  get an anti-heal status marker.
- Passive: small response pulse when CC reduces cooldowns. Low priority.

### Hudson

- LMB: muzzle flash, tracer, metallic impact; spun-up state gets denser tracers and a
  hotter muzzle. Heal feedback remains subtle but visible to Hudson.
- RMB: one-second spin-up progress ring, persistent heat while active, wind-down steam.
- Shift: twin jet exhaust trails and a clear collision/knockback burst.
- Q: readable physical barbed wire plus pulsing/striped danger boundary.
- R: hook projectile, thick cable, latch burst, reel motion, and strong execute flash.
- Passive: scrap-colored particles pull toward Hudson when Salvager triggers.

### Crysta

- LMB: sharp cyan crystal projectile. Empowered version fires two aligned projectiles,
  one behind the other, with a gold/white core.
- Passive: crystal mark visibly attached to/orbiting marked enemies. Detonation shatters
  the mark and removes it.
- RMB: returning crystal/crescent with curved path trail. Outbound and return legs use
  different patterns. Return impact shows pull direction toward Crysta.
- Shift: sharp gold dash trail; empowered-LMB readiness leaves a restrained weapon/hand
  glow.
- Q: cyan ground circle with accumulating crystal growth and a closing timer treatment;
  recast/expiry collapses inward then bursts.
- R: grey full-length lane indicators, alternating magenta beams, convergence at aim,
  extension beyond convergence by remaining range, and a final dual-beam flash. Close
  convergence produces a long X; far convergence produces a V.

### Void

- LMB: dense purple orb. Charged version has a white core, distortion shell, and clear
  phase trail indicating it can pass through world objects.
- Passive: three small orbiting motes fill as confirmed hits accumulate; empowered state
  completes and intensifies the ring.
- RMB: moving cone-shaped projectile with its tip pointing outward along velocity.
  Empowered version is larger with a bright secondary shell.
- Shift: arcing orb with destination circle traveling beneath it plus a source circle
  around Void. Both collapse/reopen during the swap. Visuals must cover hunters, wisps,
  test allies, chests, and marked swappable props.
- Q: round dark puddle with exact radius, inward-moving timer ring, tick ripples, and
  collapse/burst on expiry or Q recast. Empowered Q has a visibly larger secondary rim.
- R: warning ring with inward spirals, dense central core, pull trails from affected
  actors, and a sharp implosion/end cue. Empowered radius must be obvious.

## Schedule and hard gates

All gates are calendar events at 20:00 Europe/Bucharest with popup reminders 12 hours
and 1 hour before the deadline.

| Date | Gate | Required evidence |
| --- | --- | --- |
| Jul 12 | Green baseline | Named checkpoint; Editor/Game/Server compile; PureLogic and GameSystems green |
| Jul 13 | Listen-server combat | Two clients; all six selections; spawn/move/aim/basic damage |
| Jul 14 | Dedicated server | Dedicated server plus two clients through lobby, combat, and death |
| Jul 15 | Packaged networking | Fresh packaged direct-IP server/client test; second machine if remote |
| Jul 16 | VFX foundation lock | Templates, palettes, cue paths, budgets, fallbacks, parameter contract |
| Jul 17 | Ghost complete | All abilities readable and two-client tested |
| Jul 18 | Eluna complete | All abilities readable and two-client tested |
| Jul 19 | Kingpin complete | All abilities readable and two-client tested |
| Jul 20 | Hudson plus internal test | Hudson complete; first short multiplayer playtest |
| Jul 21 | Crysta complete | All abilities readable and two-client tested |
| Jul 22 | Void complete | All abilities readable and two-client tested |
| Jul 23 | Hard feature/VFX freeze | Full-roster integration; generic fallback for anything unfinished |
| Jul 24 | Multiplayer playtest 1 | Recorded failures, logs, readability notes; no live polishing |
| Jul 25 | Repair day 1 | Highest P0/P1 findings fixed and regression-tested |
| Jul 26 | Redo gate | Remaining P0/P1 repair; failed readability effects redone/fallbacked |
| Jul 27 | Packaged lag/loss test | `Net PktLag=100`, `Net PktLoss=2`; roster cut decision |
| Jul 28 | Performance/cleanup | Low/Medium performance; death/disconnect/cue cleanup |
| Jul 29 | Dress rehearsal | Exact July 31 topology, build, distribution, and instructions |
| Jul 30 | Release candidate freeze | Final package distributed and connection-verified |
| Jul 31 | Playtest | Use verified RC; configuration-only emergency fixes |

## Networking strategy

### Preferred topology

- Windows dedicated server.
- Two to four Windows clients.
- Direct IP over UDP 7777.
- Test a second physical machine by July 15 if July 31 is remote.

### Deadline fallback

- If dedicated is red but listen server is green by July 15, use listen server.
- If internet routing is red but LAN/VPN direct IP is green, lock the playtest to the
  proven connection method and document it.
- Do not build session discovery or matchmaking to solve a direct-IP logistics problem.
- If the complete match loop is red, run a TestMap combat playtest with manual reset.

### Network smoke matrix

For every enabled active ability, test all applicable rows:

- Owner activates once; remote client sees it once.
- Server accepts valid activation and rejects blocked activation.
- Damage/heal/CC happens once and matches both clients.
- Projectile path, cone orientation, beam endpoints, zone radius, and hitbox agree.
- Cooldown and gameplay tags agree after activation.
- Ability release/recast works, especially Crysta RMB, Hudson RMB, Crysta Q, and Void Q.
- Persistent state cleans up on normal end, timeout, cancel, owner death, target death,
  actor destruction, and match reset.
- Movement abilities do not permanently desync position or movement mode.
- Late observation/relevancy does not leave an active zone invisible.
- Repeat under `Net PktLag=100` and `Net PktLoss=2` on July 27.

Core session tests:

- Lobby join and slot assignment.
- Hunter select/lock/ready.
- Countdown, spawn/drop, possession, ASC avatar binding.
- Combat, death/wisp/revive where enabled.
- Disconnect and one reconnect attempt.
- Match reset/return flow or documented TestMap reset procedure.
- Server and client logs checked for crash, ensure, Accessed None, repeated net warnings,
  prediction rejection loops, stale GameplayCues, and missing cooked assets.

## Definition of done

An ability is done only when:

1. Owner sees correct warning/cast/travel/active/impact/cleanup.
2. Another client sees the same gameplay-relevant sequence.
3. Visible geometry matches the real gameplay geometry.
4. Gameplay applies once under server authority.
5. Cooldown, tags, and recast/release state remain correct.
6. Death/cancel/expiry leaves no effect, timer, actor, or tag behind.
7. A short 100 ms latency check does not make it unusable.
8. It has a working primitive or low-quality fallback.

A hunter is done only when every enabled ability passes and the result is recorded in the
status tracker. Compiling or looking correct in standalone PIE is not sufficient.
Use `docs/plans/july-31-manual-vfx-acceptance.md` for the focused two-client execution order
and hunter-specific acceptance checks.

## Test and build commands

Run from repository root in PowerShell.

### Build Editor

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' BreachborneEditor Win64 Development -Project='C:\Unreal Projects\Breachborne\Breachborne.uproject' -WaitMutex -NoHotReloadFromIDE
```

### Build game and server targets

The Epic Launcher engine can build the game target but refuses dedicated server targets.
For dedicated or packaged network tests, build both Game and Server with the same source-built
UE 5.7.4 toolchain. Mixing a Launcher Game client with the source-built Server produces different
network version hashes and is rejected as `OutdatedClient`.

```powershell
& 'C:\UnrealEngine-5.7.4-release\Engine\Build\BatchFiles\Build.bat' Breachborne Win64 Development -Project='C:\Unreal Projects\Breachborne\Breachborne.uproject' -WaitMutex
& 'C:\UnrealEngine-5.7.4-release\Engine\Build\BatchFiles\Build.bat' BreachborneServer Win64 Development -Project='C:\Unreal Projects\Breachborne\Breachborne.uproject' -WaitMutex
```

### Automated tests

Run the suites separately. Combining them with `Quit` previously caused only the first
suite to be observed.

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Unreal Projects\Breachborne\Breachborne.uproject' -unattended -nop4 -nullrhi -nosound '-ExecCmds=Automation RunTests Breachborne.PureLogic; Quit' -TestExit='Automation Test Queue Empty' -log
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Unreal Projects\Breachborne\Breachborne.uproject' -unattended -nop4 -nullrhi -nosound '-ExecCmds=Automation RunTests Breachborne.GameSystems; Quit' -TestExit='Automation Test Queue Empty' -log
```

Inspect `Saved/Logs/Breachborne.log` after each suite. A process exit code alone is not
sufficient evidence.

### Local networking

```powershell
.\Scripts\Networking\StartLocalDedicatedServer.ps1
.\Scripts\Networking\StartLocalClients.ps1 -ClientCount 2
```

For a repeatable source-built dedicated handshake with separate logs:

```powershell
.\Scripts\Networking\TestHeadlessDedicatedHandshake.ps1 -ClientCount 2
```

Expected phase flow:

```text
HunterSelect -> Countdown -> Dropping -> Playing -> PostMatch -> HunterSelect
```

### Package playtest

```powershell
.\Scripts\Packaging\PackagePlaytestWindows.ps1
```

Expected outputs:

- `Builds/WindowsClient`
- `Builds/WindowsServer`
- `Builds/PlaytestBuildSummary.txt`

Then validate the packaged artifacts themselves:

```powershell
.\Scripts\Playtest\TestPackagedHeadlessHandshake.ps1 -ClientCount 2
.\Scripts\Playtest\VerifyPlaytestCandidate.ps1
```

### Network emulation

Automated packaged activation gate:

```powershell
.\Scripts\Playtest\TestPackagedNetworkImpairment.ps1
```

Current evidence passes hunters 1-6 across three matches with server and both clients confirming
the configured values in their logs. This does not replace manual hit, damage, persistent-zone,
cleanup, or visual-agreement testing.

In each relevant client console:

```text
Net PktLag=100
Net PktLoss=2
```

Reset both values to zero after the test.

## Severity and scope-cut policy

### Severity

- P0: crash, cannot connect, cannot spawn, corrupted match, unrecoverable state.
- P1: broken ability, wrong authority, duplicate gameplay, invisible enemy danger,
  permanent movement desync, cleanup leak, or unusable readability.
- P2: cosmetic polish, minor timing mismatch, weak but understandable feedback.

P0/P1 blocks a gate. P2 does not block a package unless it hides gameplay.

### Automatic cuts

- Behind on July 19: LMB and passive effects stay primitive. Focus RMB/Q/Shift/R.
- Behind on July 23: unfinished effects use generic templates. No bespoke Niagara after
  freeze.
- Broken hunter on July 27: disable that hunter for the playtest.
- Dedicated server red on July 15 but listen green: use listen server.
- Full match loop red: use controlled TestMap combat flow.
- Heavy or unstable effect: restore primitive fallback.
- No new mechanics after July 20 unless something of equal or larger scope is removed.

## Daily accountability process

Every workday ends with an update to `docs/plans/july-31-playtest-status.md` containing:

```text
Date:
Gate due:
Completed:
Build result:
Automated tests:
Two-client test:
Evidence/log path:
Open P0/P1 issues:
Tomorrow's single target:
```

Rules:

- One hunter or shared blocker in progress at a time.
- Every green gate gets a named commit/checkpoint and evidence path.
- "Almost done" is red.
- After 90 minutes blocked, use the documented fallback or bring logs/code to an agent.
- Test a hunter immediately after its VFX day; do not defer six hunter tests to July 24.
- Do not spend a scheduled repair day on new content.
- Calendar deadlines are acceptance reviews, not merely reminders to start the task.

## Agent handoff protocol

Any agent taking a task must:

1. Read this entire document and the status tracker.
2. Inspect `git status` and preserve unrelated/user changes.
3. Identify the current red gate and work only on that gate unless explicitly redirected.
4. Read the relevant existing hunter and Hudson visual/cue implementation before editing.
5. State the exact acceptance checks before making edits.
6. Compile in proportion to the change and run the relevant test/smoke matrix.
7. Update the status tracker with facts, commands, evidence, and remaining failures.
8. Never mark a gate green based only on static analysis.

The next agent should start with the July 12 red baseline, specifically the crashing
GameSystems test, before adding any Niagara content.

## Ownership split

User/editor-facing work:

- Author and tune Niagara systems, materials, decals, textures, and cue Blueprint content.
- Judge whether the effect is readable and visually appropriate.
- Supply a second machine/testers when required.
- Decide final artistic palette values on July 16.

Codex/engineering work:

- Audit and repair cue calls, locations, directions, radii, lifetimes, and cleanup.
- Standardize data/parameter contracts and primitive fallbacks.
- Implement network-safe gameplay/cosmetic hookups.
- Compile targets, run automation, inspect logs, and execute repeatable smoke tests.
- Maintain the master/status documents and enforce gate definitions.
- Integrate and debug assets authored in the Unreal Editor.

## Existing supporting documents

- `docs/visuals-low-spec-rollout.md`
- `docs/VFX_FOUNDATION.md`
- `docs/networking-cheatsheet.md`
- `docs/PIEChecklist.md`
- `docs/architecture.md`
- `docs/plans/crysta-hunter-plan.md`
- `docs/plans/void-hunter-plan.md`
- `docs/plans/july-31-manual-vfx-acceptance.md`
- `Scripts/Networking/README.md`

This plan takes precedence for July 31 scheduling and scope. The supporting documents
remain authoritative for deeper system details that do not conflict with this plan.
