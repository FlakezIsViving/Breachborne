# July 31 Manual VFX And Network Acceptance

Use this checklist after any full-roster primitive or authored-VFX batch. Results belong in
`docs/plans/july-31-playtest-status.md`. A compile or headless connection does not satisfy
this checklist.

## Acceptance count

- Hunter abilities and cleanup/stress below: `41` checks (`36` hunter + `5` global).
- Range-indicator matrix in `docs/ABILITY_RANGE_INDICATORS.md`: `9` checks.
- Wisp UI below: `5` checks.
- Total manual presentation acceptance: `55` checks. The second-machine direct-IP gate is tracked
  separately because it requires external hardware rather than a visual checklist row.
- Stable row IDs are authoritative across session records: `GE` (Ghost/Eluna), `KH`
  (Kingpin/Hudson), `CV` (Crysta/Void), `CS` (cleanup/stress), `RI` (range indicators), and `WI`
  (wisp UI).

## Session setup

1. Use the packaged `TestMap` candidate. The launcher starts a hidden dedicated server and
   separate client windows; do not open PIE for the recorded acceptance pass.
2. Use two players for ordinary opposing-pair checks. Use the three-client launcher documented
   below for Ghost/Eluna because Eluna's Shift, R, and passive require a real allied hunter.
3. Verify the playtest fallbacks directly; the legacy DrawDebug multicast path is removed.
4. Put the two players on opposing teams. Keep a second ally/test target available for heal,
   chain, swap, and revive tests.
5. For each ability, watch once from the owner window and once from the other window.
6. The recorded launcher already uses a hidden dedicated server. Repeat failures and the required
   release/recast cases under the documented 100 ms/2% loss profile rather than changing topology.

For every row, record `PASS` only when:

- owner and observer see one warning/active/resolution sequence;
- visible width, length, and radius match the hit area;
- damage, healing, CC, and movement happen once;
- cooldown and empowered state agree on both clients;
- expiry, recast, release, death, and cancellation remove persistent visuals.

## Execution batches

Run these as four separate packaged sessions. Use the pair-specific launcher for the first three
batches and `StartPackagedLocalSmoke.ps1` for cleanup/stress. Stop and review every isolated session
with `.\Scripts\Playtest\StopPackagedLocalSmoke.ps1` before changing hunter pairs.

Automated precondition: `.\Scripts\Playtest\TestPackagedFullRosterAbilitySmoke.ps1` must pass
before these sessions. It proves grants and input lifecycle only; do not use it to check off any
visual, hit, damage, CC, movement-agreement, or readability item below.

Automated impairment precondition: `.\Scripts\Playtest\TestPackagedNetworkImpairment.ps1` must
also pass. It proves the same activation lifecycle survives confirmed 100 ms lag and 2% loss; it
does not replace the manual sustained-fire, persistent-zone, cleanup, or readability checks.

Core death precondition: `.\Scripts\Playtest\TestPackagedDeathWispSmoke.ps1` must pass. It proves
lethal GAS damage reaches HealthSet depletion, replicated wisp possession, capped healing-driven
revive, and victim-client hunter repossession; it does not prove the two bars are visible,
proximity priority is readable, or ability visuals clean up when their caster dies.

Wisp-rule precondition: `.\Scripts\Playtest\TestPackagedWispRulesSmoke.ps1` must pass. It proves
natural decay, ally freeze/fill, accelerated enemy drain, enemy priority over allies and healing,
the Eluna-carry exception, actual passive pickup, CC drop, and final revive/re-possession. The
two-bar presentation and whether players can understand those states remain manual checks.

Damage precondition: `.\Scripts\Playtest\TestPackagedFullRosterHitSmoke.ps1` must pass. It proves
each hunter's LMB can produce authoritative enemy health loss; it does not prove visual hitbox
agreement or any RMB/Shift/Q/R damage, healing, movement, or crowd-control outcome.

Non-LMB outcome precondition: `.\Scripts\Playtest\TestPackagedFullRosterOutcomeSmoke.ps1` must
pass. It proves 24/24 hunter-specific RMB/Shift/Q/R damage, healing, movement, state, actor,
pull/swap, and recast contracts on a dedicated server. It does not prove owner/observer visual
readability, visible-geometry agreement, persistent cleanup presentation, or subjective quality.

Impaired outcome precondition: `.\Scripts\Playtest\TestPackagedOutcomeNetworkImpairment.ps1`
must also pass. It repeats the same 24 contracts with confirmed 100 ms lag and 2% loss on every
server/client process. Manual impaired checks still judge presentation, sustained fire, overlap,
and whether visual geometry remains synchronized with gameplay.

1. Ghost versus Eluna: run `StartGhostElunaAcceptance.ps1` with Eluna plus a helper ally against
   Ghost, then complete both six-item hunter sections.
2. Kingpin versus Hudson: run `StartKingpinHudsonAcceptance.ps1` with a second nearby enemy for
   Kingpin's chain, then complete both six-item hunter sections, including Hudson's ten-second
   sustained-fire check.
3. Crysta versus Void: run `StartCrystaVoidAcceptance.ps1`, then complete both six-item hunter
   sections. Record the marked-prop visual subcase as setup-blocked until a fixture is cooked.
4. Cleanup and stress: complete the five global checks, then repeat only failed hunter checks on
   dedicated server with lag/loss where required.

Do not carry a failed visual forward as "probably fine." Record it in the status issue table with
the session directory printed by the launcher, then continue the batch so all failures are known
before repairs begin.

### Ghost/Eluna exact setup

Start the recorded three-client session only when the tester is present:

```powershell
.\Scripts\Playtest\StartGhostElunaAcceptance.ps1
```

Assign the lobby exactly as follows before readying:

| Window | Team/slot | Hunter | Purpose |
| --- | --- | --- | --- |
| Client 1 | Team 0 / Slot 0 | Eluna (`3`) | Eluna owner view |
| Client 2 | Team 0 / Slot 1 | Hudson (`4`) | Allied heal, pass-through, R, and wisp target |
| Client 3 | Team 1 / Slot 0 | Ghost (`1`) | Ghost owner and opposing observer view |

Use Ghost to damage the helper below full health before testing Eluna passive healing and living-
ally R. After the helper becomes a wisp, dash Eluna through the wisp for the Shift refund. Test Q travel/attach/healing on the
helper. Then have Ghost kill the helper: this supplies Ghost's passive-reset kill and an allied
wisp for Eluna pickup/drop and wisp-target R. Watch each cast once from the owner and once from
another window. Stop the whole recorded session with `StopPackagedLocalSmoke.ps1`; do not close
individual windows, because that loses the reliable session cleanup/review path.

### Kingpin/Hudson exact setup

Run `StartKingpinHudsonAcceptance.ps1`. Assign Client 1 to Kingpin on Team 0; assign Hudson and a
Ghost helper to Team 1. Keep both enemy targets near each other for Kingpin's chain line and sector
checks. The helper is an observation/target fixture, not an additional acceptance hunter.

### Crysta/Void exact setup

Run `StartCrystaVoidAcceptance.ps1`. Assign Crysta to Team 0 and Void to Team 1. Use the map wall,
chest, and ordinary props for wall-pierce and swap readability. The current frozen package contains
no placed prop with `UBBVoidSwappableComponent`; deterministic tests cover category execution, but
the marked-prop visual subcase must remain setup-blocked until a fixture is added and repackaged.

## Ghost

- [x] `GE-01` LMB tracer and impact are visible without debug draw.
- [x] `GE-02` RMB grenade flight and detonation radius are readable on both clients.
- [ ] `GE-03` Shift shows a low directional trail and movement remains synchronized.
- [x] `GE-04` Q shows a thin full piercing line plus a burst on every hit target.
- [x] `GE-05` R warning appears before damage starts; active orange floor matches the damage radius.
- [x] `GE-06` Kill reset produces one passive pulse and correctly refreshes Shift.

## Eluna

- [x] `GE-07` Held LMB remains legible at repeated-fire speed; damage stacking still works.
- [ ] `GE-08` RMB sticky projectile, delayed growing root area, pop, damage, and root agree.
- [ ] `GE-09` Shift path is visible; dashing through an allied wisp collects it and produces the refund pulse and full cooldown reset.
- [ ] `GE-10` Q disc uses the exact heal radius, visibly travels when tossed, attaches correctly, pulses,
  and produces one final heal burst.
- [ ] `GE-11` R uses the narrow translucent tether, remains attached while both actors move, and cleans up
  on success, range break, target death, and caster cancellation.
- [ ] `GE-12` Passive heal pulse is restrained; wisp pickup/drop state is understandable.

## Kingpin

- [ ] `KH-01` LMB swipe footprint, primary impact, and one chain line match actual targets.
- [ ] `KH-02` RMB hook line/latch is visible; the pull occurs once and the line does not remain afterward.
- [ ] `KH-03` Shift charge/dash direction and collision bursts agree on both clients.
- [ ] `KH-04` Q sector outline matches the actual stun/damage cone.
- [ ] `KH-05` R visibly fires two separate broad sectors; each target takes one hit per shell and gains
  anti-heal.
- [ ] `KH-06` Passive response appears once when CC shortens cooldowns.

## Hudson

- [ ] `KH-07` Hold RMB for at least one second: progress outline grows, spun-up state appears, and releasing
  RMB clears the visual and applies wind-down.
- [ ] `KH-08` Hold LMB for ten seconds in normal and spun-up states: tracers/impacts/heal feedback remain
  readable and no cue-overflow warning appears.
- [ ] `KH-09` Shift trail and collision burst appear without repeated debug RPCs; release ends hover.
- [ ] `KH-10` Q physical barbed wire is visible, colored, correctly oriented, and bounded to its hit box.
- [ ] `KH-11` R fire/latch/tether/reel/execute are distinct and all visuals clean up on miss, timeout,
  execute, caster death, and target death.
- [ ] `KH-12` Passive repair/heal pulse is visible but does not obscure combat.

## Crysta

- [ ] `CV-01` LMB impact is visible; empowered shots are two aligned projectiles front-to-back on both
  clients.
- [ ] `CV-02` Reverberation application/detonation is understandable from impact feedback; note whether
  a persistent mark indicator is required before playtest.
- [ ] `CV-03` RMB outbound cyan and return orange states follow the curved path; release returns early;
  return impact communicates pull direction.
- [ ] `CV-04` Both Shift charges show a gold trail and grant the empowered LMB state.
- [ ] `CV-05` Q disc matches its true gameplay radius and recast/expiry produces one magenta burst.
- [ ] `CV-06` R grey indicators show complete lane length; alternating magenta beams cross past close
  convergence into an X and remain a V at far convergence; final salvo fires both beams.

## Void

- [ ] `CV-07` LMB normal orb is violet; charged orb is larger/white and visibly passes through a wall
  while still hitting a pawn.
- [ ] `CV-08` Passive reaches empowered after three confirmed hits; empowered consumption is apparent.
- [ ] `CV-09` RMB cone tip points along travel direction; hitbox follows it; empowered size is visible
  to the observer.
- [ ] `CV-10` Shift shows source and moving destination zones at exact radius, then one burst at each
  endpoint; verify hunters, wisps, chests, and one marked prop swap.
- [ ] `CV-11` Q puddle matches exact radius, deals ticks, and bursts once on expiry or immediate recast;
  empowered Q is visibly larger.
- [ ] `CV-12` R warning radius matches pull radius, changes when active, stuns once, pulls, and ends with
  one implosion without leaving movement or tags stuck.

## Cleanup and stress

- [ ] `CS-01` Kill each caster during one persistent ability: Ghost R, Eluna Q/R, Hudson Shift/R,
  Crysta RMB/Q/R, and Void Shift/Q/R.
- [ ] `CS-02` Confirm no stale mesh, cue, timer, movement mode, hooked state, or empowered state remains.
- [ ] `CS-03` Repeat Hudson sustained LMB and one persistent-zone overlap with `Net PktLag=100` and
  `Net PktLoss=2`; restore both to zero afterward.
- [ ] `CS-04` Repeat the final candidate on Low and Medium effects quality.
- [ ] `CS-05` Fire the Grappling Hook power from one client: the observer sees one chain and authoritative
  latch endpoint; wall and actor hits pull once, and the hook cleans up on hit, range, and timeout.

## Wisp UI

- [x] `WI-01` The wisp shows two readable replicated bars to owner and remote clients: the thicker master
  decay bar below and the slimmer revive bar above, with no overlap or camera-distance clipping.
- [x] `WI-02` Natural decay visibly drains only the master bar at a steady rate while revive remains empty;
  leaving all proximity/healing effects resumes the same presentation without jumps.
- [ ] `WI-03` Allied proximity freezes the master bar and fills revive; healing also freezes decay and
  fills revive at the accelerated rate, with both clients seeing the same progress.
- [ ] `WI-04` Enemy proximity accelerates master decay. Enemy presence visibly wins over simultaneous ally
  proximity or healing, and revive does not continue filling during the contested state.
- [ ] `WI-05` Eluna carry freezes master decay even with an enemy nearby; CC drops the wisp and restores
  normal priority. Both bars follow the carried/dropped wisp and disappear on revive or final death.

## Result recording

Record failures in `docs/plans/july-31-playtest-status.md` as P0/P1/P2 with hunter, ability,
owner/observer, reproduction steps, expected result, actual result, and log path. Mark an ability
`PASS` in the status matrix only after both windows agree and cleanup succeeds.

Update the manual acceptance ledger in the status tracker after each recorded session. Leave a
setup-blocked row unchecked and link its issue; do not count deterministic or headless coverage as
a manual presentation PASS. Range-indicator rows remain incomplete until every hunter/power named
by that row passes, even when some hunter-specific observations were already successful.
