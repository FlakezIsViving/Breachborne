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
source-engine Server targets compiled successfully on July 13, 2026. Checked implementation
items mean source-complete, not multiplayer-accepted.

## Tests

- [x] Select hunter ID `5` and confirm all abilities grant on owner and server.
- [ ] Verify RMB tap, RMB hold/flick, and RMB release return on two clients.
- [x] Verify LMB detonates marks and empowered LMB fires two collinear shots front-to-back.
- [x] Verify Shift charges recharge separately and passive detonation reduces both cooldowns.
- [x] Verify Q recast detonates and R switches between X and V patterns by aim distance.
- [ ] Verify death/cancel/expiry cleanup and repeat with `Net PktLag=100`.

Acceptance evidence (July 13, 2026):

- Packaged dedicated smoke selected Crysta as hunter `5`, reached server preparation, and the
  owning client received all `7` activatable abilities at
  `Saved/Logs/PackagedOutcomeSmoke/20260714-001824`.
- The same run passed authoritative RMB damage/ground-or-pull, Shift movement plus empowered LMB,
  Q recast damage plus Reverberation mark, and R damage plus mark (`4/4` outcome contracts).
  It also proves the second Shift charge activates, both specific Shift cooldowns exist, and a
  Reverberation detonation reduces both remaining durations by approximately `1.5s`. GameSystems
  49/49 proves that LMB removes the active Reverberation effect and that empowered shots are
  collinear and ordered front-to-back. The Q contract includes the explicit second press and
  authoritative detonation damage. The GameSystems contract
  `extends Crysta R lanes past close convergence into an X` proves that a
  close `800`-unit convergence retains `1500` units of overrun while a `2200`-unit convergence
  retains only `100`, producing the intended X-to-V transition. Live beam readability is still
  part of the manual visual matrix.
- The matching impaired outcome contract passed at
  `Saved/Logs/PackagedOutcomeNetworkImpairment/20260714-002447` with `100 ms` lag and `2%` loss.
  Crysta RMB tap/hold/flick/release path readability still requires the deliberate two-client
  manual check; automated outcomes do not substitute for that visual/steering acceptance.
