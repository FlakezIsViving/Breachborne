# Crysta Hunter Implementation Plan

## Summary
- Roster ID: `5`.
- Role: Fighter burst mage.
- Native fallback stats: `525 HP`, `50 AttackPower`, `60 AbilityPower`, `600 MoveSpeed`.
- Mana is omitted for the July 31 playtest because Breachborne has no live mana resource.

## Implementation

- [x] Add `GA_Crysta_LMB`, `GA_Crysta_RMB`, `GA_Crysta_Shift_Primary`, `GA_Crysta_Shift_Secondary`, `GA_Crysta_Q`, `GA_Crysta_R`, and `GA_Crysta_Passive`.
- [x] Add `ABBCrystaLMBProjectile`, `ABBCrystaFluxSnareProjectile`, `ABBCrystaBurstZone`, and `UBBReverberationMarkEffect`.
- [x] Passive Reverberation: Crysta abilities apply `State.Crysta.Reverberation`. LMB and R detonate it for bonus damage, remove it, and emit `Event.Crysta.ReverberationDetonated`.
- [x] LMB: fast projectile, detonates Reverberation. If `State.Crysta.EmpoweredLMB` is present, consume it and fire two collinear shots spaced front-to-back.
- [x] RMB: tap fires outward and auto-returns. Hold samples replicated aim and bends outbound movement toward the flick direction before returning. Release after the hold threshold starts return immediately.
- [x] Shift: two separate cooldown-backed dash charges. Each dash grants empowered LMB.
- [x] Q: first press places a target-ground zone, second press detonates, auto-detonates after `6s`.
- [x] R: five timed salvos converging from outside-in. Central targets use X pattern; near-range targets collapse to a V pattern. R detonates each target's Reverberation once per cast, then refreshes the mark.

Implementation evidence: native classes and roster fallback are present; Editor, Game, and
source-engine Server targets compiled successfully on July 12, 2026. Checked implementation
items mean source-complete, not multiplayer-accepted.

## Tests

- [ ] Select hunter ID `5` and confirm all abilities grant on owner and server.
- [ ] Verify RMB tap, RMB hold/flick, and RMB release return on two clients.
- [ ] Verify LMB detonates marks and empowered LMB fires two collinear shots front-to-back.
- [ ] Verify Shift charges recharge separately and passive detonation reduces both cooldowns.
- [ ] Verify Q recast detonates and R switches between X and V patterns by aim distance.
- [ ] Verify death/cancel/expiry cleanup and repeat with `Net PktLag=100`.
