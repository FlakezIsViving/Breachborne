# Void Hunter Implementation Plan

## Summary
- Roster ID: `6`.
- Role: Controller/displacement mage.
- Native fallback stats: `500 HP`, `45 AttackPower`, `65 AbilityPower`, `590 MoveSpeed`.
- Mana is omitted for the July 31 playtest because Breachborne has no live mana resource.

## Implementation

- [x] Add `GA_Void_LMB`, `GA_Void_RMB`, `GA_Void_Shift`, `GA_Void_Q`, `GA_Void_R`, and `GA_Void_Passive`.
- [x] Add `ABBVoidOrbProjectile`, `ABBVoidSnapProjectile`, `ABBVoidSwapProjectile`, `ABBVoidAnomalyPuddle`, `ABBVoidSingularityActor`, `UBBVoidGravitySlowEffect`, and `UBBVoidSwappableComponent`.
- [x] Passive Hands of Fate: confirmed enemy damage emits `Event.Void.DamageDealt`. After `3` hits within `6s`, Void gains `State.Void.Empowered`. RMB/Shift/Q/R consume it.
- [x] LMB: projectile loop at `0.6s`. If Void has not fired for `1.5s`, next orb is charged and ignores world static/world dynamic collision while still hitting pawns.
- [x] RMB: moving cone-shaped projectile. Tip faces outward; per-tick cone hit logic stuns and damages the first valid enemy. Empowered increases size/speed/damage.
- [x] Shift: arcing zone projectile using the nuke-style Bezier visual path with `1.5s` travel. On timeout, swap eligible actors around Void with eligible actors around the destination.
- [x] Swap eligibility: alive hunters, wisps, test allies, chests, and actors/components marked with `UBBVoidSwappableComponent`.
- [x] Q: round damaging puddle for `3.5s`, tick damage while enemies stand in it, then burst damage. Re-press bursts immediately. Empowered increases radius.
- [x] R: warning into active vortex, one-time stun, short server-authoritative pull. Empowered increases radius.

Implementation evidence: native classes and roster fallback are present; Editor, Game, and
source-engine Server targets compiled successfully on July 13, 2026. Checked implementation
items mean source-complete, not multiplayer-accepted.

## Tests

- [x] Select hunter ID `6` and confirm all abilities grant on owner and server.
- [x] Verify charged LMB pierces walls but still hits pawns.
- [x] Verify RMB cone projectile hitbox, outward orientation, and empowered size/speed.
- [x] Verify Shift swaps hunters, wisps, test allies, chests, and marked props while skipping unmarked static props.
- [x] Verify Q recast skips remaining puddle ticks and triggers burst.
- [x] Verify R warning, stun, pull, and empowered radius.
- [ ] Verify death/cancel/expiry cleanup and repeat with `Net PktLag=100`.

Acceptance evidence (July 13, 2026):

- Packaged dedicated smoke selected Void as hunter `6`, reached server preparation, and the owning
  client received all `6` activatable abilities at
  `Saved/Logs/PackagedOutcomeSmoke/20260714-001923`.
- The same run passed authoritative RMB damage/stun, Shift two-actor displacement, Q recast damage,
  and R stun plus pull/movement (`4/4` outcome contracts).
- GameSystems proves that charged orb collision ignores WorldStatic and WorldDynamic while still
  overlapping Pawn; the latest packaged LMB hit run at
  `Saved/Logs/PackagedHitSmoke/20260714-001319` proves authoritative pawn damage.
- GameSystems also proves the cone tip aligns with travel, rejects points behind/outside its
  bounded hit volume, and increases length/radius/speed when empowered. The Q contract plus the
  early `Burst()` GameSystems test proves a second press destroys the live puddle and resolves the
  burst before natural expiry.
- GameSystems 54/54 covers Shift category eligibility, actual paired teleport execution for
  hunters, wisps, test allies, chests, and marked props, marked-prop mobility/replication, and
  rejection of unmarked props. It also covers the Singularity warning-to-active lifecycle,
  one-time stun/pull state, completion, and empowered radius. Death/cancel/expiry cleanup remains
  a separate manual regression.
- The Jul 17 package cooks the updated `TestMap`; placed `ABBTargetDummy` actors now provide the
  marked-prop visual fixture and replicate actor movement from class defaults. That visual subcase
  is no longer setup-blocked.
- The matching impaired outcome contract passed at
  `Saved/Logs/PackagedOutcomeNetworkImpairment/20260714-002552` with `100 ms` lag and `2%` loss.
