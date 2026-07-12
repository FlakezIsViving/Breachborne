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
source-engine Server targets compiled successfully on July 12, 2026. Checked implementation
items mean source-complete, not multiplayer-accepted.

## Tests

- [ ] Select hunter ID `6` and confirm all abilities grant on owner and server.
- [ ] Verify charged LMB pierces walls but still hits pawns.
- [ ] Verify RMB cone projectile hitbox, outward orientation, and empowered size/speed.
- [ ] Verify Shift swaps hunters, wisps, test allies, chests, and marked props while skipping unmarked static props.
- [ ] Verify Q recast skips remaining puddle ticks and triggers burst.
- [ ] Verify R warning, stun, pull, and empowered radius.
- [ ] Verify death/cancel/expiry cleanup and repeat with `Net PktLag=100`.
