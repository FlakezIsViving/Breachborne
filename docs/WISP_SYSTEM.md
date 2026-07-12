# Wisp System

This document is the playtest contract for combat deaths, wisp bars, proximity
rules, Eluna carry, execution, and revival. Gameplay is server-authoritative;
clients render replicated state.

## Overhead UI

Every wisp displays, from top to bottom:

1. The downed player's name.
2. A thin green revive bar (`RezBarProgress`, 0 to 1).
3. A larger yellow master decay bar (`WispHP / MaxWispHP`).

The decay bar is authoritative. Reaching zero kills the wisp and creates a
deathbox even if the revive bar has partial progress.

## Proximity And Healing Priority

The ally/enemy proximity radius is 175 cm, plus normal collision-shape overlap
leeway.

| Nearby state | Master decay bar | Revive bar |
| --- | --- | --- |
| Nobody | Drains at 2 HP/s | Existing progress decays |
| Ally only | Frozen | Fills at 20%/s |
| Enemy only | Drains at 6 HP/s | Existing progress decays |
| Ally and enemy | Drains at 6 HP/s; enemy wins | Does not fill and existing progress decays |
| Healing, no enemy | Frozen | Healing accelerates fill, capped at 2x total speed |
| Healing with enemy | Drains at 6 HP/s; enemy wins | Healing contribution is blocked |

## Eluna Carry Exception

Eluna automatically picks up one allied wisp within 200 cm. While carried:

- The wisp follows Eluna 80 cm above her.
- The master decay bar is frozen.
- The revive bar fills at the existing maximum 2x rate.
- Enemy proximity does not contest or accelerate decay.
- `State.Stunned`, `State.Spiked`, `State.Dazed`, `State.Hooked`, Hudson hook,
  or Void singularity pull drops the wisp.
- Ending or cancelling Eluna's passive, including Eluna death, drops the wisp.

After a drop, ordinary ally/enemy priority resumes on the next 0.2-second wisp
server tick.

## Completion And Death

- Revive bar reaches 100%: restore the hidden hunter at the wisp location with
  50% health and destroy the wisp.
- Master decay reaches zero: destroy the wisp and spawn a deathbox.
- Enemy Interact/E within 150 cm: starts a two-second execution. Damage to the
  executor cancels it. Completion creates a deathbox and fully heals executor.
- Allied Interact/E within 300 cm of a deathbox: starts a five-second revive,
  restoring the hunter with 35% health if uninterrupted.

## Multiplayer Test Matrix

1. Observe name, thin upper revive bar, and larger lower decay bar on every client.
2. Leave the wisp alone and confirm only the decay bar falls.
3. Stand an ally on it and confirm decay freezes while revive fills.
4. Apply healing with no enemy present and confirm decay freezes and revive fills faster.
5. Add an enemy and confirm decay resumes faster while ally/healing revive stops.
6. Remove the enemy and confirm ally/healing revival resumes from remaining progress.
7. Carry it with Eluna while an enemy overlaps and confirm revival continues.
8. Apply hard CC to Eluna and confirm the wisp drops and normal contest rules resume.
