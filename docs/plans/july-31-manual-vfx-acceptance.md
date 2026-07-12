# July 31 Manual VFX And Network Acceptance

Use this checklist after any full-roster primitive or authored-VFX batch. Results belong in
`docs/plans/july-31-playtest-status.md`. A compile or headless connection does not satisfy
this checklist.

## Session setup

1. Open `TestMap` in Unreal Editor.
2. Use two players in separate windows with one listen server and one client.
3. Verify the playtest fallbacks directly; the legacy DrawDebug multicast path is removed.
4. Put the two players on opposing teams. Keep a second ally/test target available for heal,
   chain, swap, and revive tests.
5. For each ability, watch once from the owner window and once from the other window.
6. After the listen pass, repeat the failures and the release/recast cases on dedicated server.

For every row, record `PASS` only when:

- owner and observer see one warning/active/resolution sequence;
- visible width, length, and radius match the hit area;
- damage, healing, CC, and movement happen once;
- cooldown and empowered state agree on both clients;
- expiry, recast, release, death, and cancellation remove persistent visuals.

## Execution batches

Run these as four separate packaged sessions. Start each session with
`.\Scripts\Playtest\StartPackagedLocalSmoke.ps1`, then stop and review its isolated logs with
`.\Scripts\Playtest\StopPackagedLocalSmoke.ps1` before changing hunter pairs.

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

Damage precondition: `.\Scripts\Playtest\TestPackagedFullRosterHitSmoke.ps1` must pass. It proves
each hunter's LMB can produce authoritative enemy health loss; it does not prove visual hitbox
agreement or any RMB/Shift/Q/R damage, healing, movement, or crowd-control outcome.

1. Ghost versus Eluna: complete both six-item hunter sections.
2. Kingpin versus Hudson: complete both six-item hunter sections, including Hudson's ten-second
   sustained-fire check.
3. Crysta versus Void: complete both six-item hunter sections.
4. Cleanup and stress: complete the five global checks, then repeat only failed hunter checks on
   dedicated server with lag/loss where required.

Do not carry a failed visual forward as "probably fine." Record it in the status issue table with
the session directory printed by the launcher, then continue the batch so all failures are known
before repairs begin.

## Ghost

- [ ] LMB tracer and impact are visible without debug draw.
- [ ] RMB grenade flight and detonation radius are readable on both clients.
- [ ] Shift shows a low directional trail and movement remains synchronized.
- [ ] Q shows a thin full piercing line plus a burst on every hit target.
- [ ] R warning appears before damage starts; active orange floor matches the damage radius.
- [ ] Kill reset produces one passive pulse and correctly refreshes Shift.

## Eluna

- [ ] Held LMB remains legible at repeated-fire speed; damage stacking still works.
- [ ] RMB sticky projectile, delayed growing root area, pop, damage, and root agree.
- [ ] Shift path is visible; ally pass-through produces the refund pulse and cooldown change.
- [ ] Q disc uses the exact heal radius, visibly travels when tossed, attaches correctly, pulses,
  and produces one final heal burst.
- [ ] R uses the narrow translucent tether, remains attached while both actors move, and cleans up
  on success, range break, target death, and caster cancellation.
- [ ] Passive heal pulse is restrained; wisp pickup/drop state is understandable.

## Kingpin

- [ ] LMB swipe footprint, primary impact, and one chain line match actual targets.
- [ ] RMB hook line/latch is visible; the pull occurs once and the line does not remain afterward.
- [ ] Shift charge/dash direction and collision bursts agree on both clients.
- [ ] Q sector outline matches the actual stun/damage cone.
- [ ] R visibly fires two separate broad sectors; each target takes one hit per shell and gains
  anti-heal.
- [ ] Passive response appears once when CC shortens cooldowns.

## Hudson

- [ ] Hold RMB for at least one second: progress outline grows, spun-up state appears, and releasing
  RMB clears the visual and applies wind-down.
- [ ] Hold LMB for ten seconds in normal and spun-up states: tracers/impacts/heal feedback remain
  readable and no cue-overflow warning appears.
- [ ] Shift trail and collision burst appear without repeated debug RPCs; release ends hover.
- [ ] Q physical barbed wire is visible, colored, correctly oriented, and bounded to its hit box.
- [ ] R fire/latch/tether/reel/execute are distinct and all visuals clean up on miss, timeout,
  execute, caster death, and target death.
- [ ] Passive repair/heal pulse is visible but does not obscure combat.

## Crysta

- [ ] LMB impact is visible; empowered shots are two aligned projectiles front-to-back on both
  clients.
- [ ] Reverberation application/detonation is understandable from impact feedback; note whether
  a persistent mark indicator is required before playtest.
- [ ] RMB outbound cyan and return orange states follow the curved path; release returns early;
  return impact communicates pull direction.
- [ ] Both Shift charges show a gold trail and grant the empowered LMB state.
- [ ] Q disc matches its true gameplay radius and recast/expiry produces one magenta burst.
- [ ] R grey indicators show complete lane length; alternating magenta beams cross past close
  convergence into an X and remain a V at far convergence; final salvo fires both beams.

## Void

- [ ] LMB normal orb is violet; charged orb is larger/white and visibly passes through a wall
  while still hitting a pawn.
- [ ] Passive reaches empowered after three confirmed hits; empowered consumption is apparent.
- [ ] RMB cone tip points along travel direction; hitbox follows it; empowered size is visible
  to the observer.
- [ ] Shift shows source and moving destination zones at exact radius, then one burst at each
  endpoint; verify hunters, wisps, chests, and one marked prop swap.
- [ ] Q puddle matches exact radius, deals ticks, and bursts once on expiry or immediate recast;
  empowered Q is visibly larger.
- [ ] R warning radius matches pull radius, changes when active, stuns once, pulls, and ends with
  one implosion without leaving movement or tags stuck.

## Cleanup and stress

- [ ] Kill each caster during one persistent ability: Ghost R, Eluna Q/R, Hudson Shift/R,
  Crysta RMB/Q/R, and Void Shift/Q/R.
- [ ] Confirm no stale mesh, cue, timer, movement mode, hooked state, or empowered state remains.
- [ ] Repeat Hudson sustained LMB and one persistent-zone overlap with `Net PktLag=100` and
  `Net PktLoss=2`; restore both to zero afterward.
- [ ] Repeat the final candidate on Low and Medium effects quality.
- [ ] Fire the Grappling Hook power from one client: the observer sees one chain and authoritative
  latch endpoint; wall and actor hits pull once, and the hook cleans up on hit, range, and timeout.

## Result recording

Record failures in `docs/plans/july-31-playtest-status.md` as P0/P1/P2 with hunter, ability,
owner/observer, reproduction steps, expected result, actual result, and log path. Mark an ability
`PASS` in the status matrix only after both windows agree and cleanup succeeds.
