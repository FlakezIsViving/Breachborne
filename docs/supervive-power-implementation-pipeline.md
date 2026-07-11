# SUPERVIVE Power Implementation Pipeline

Use this checklist when adding a Breachborne power based on the SUPERVIVE catalog.

## 1. Verify the Power

- Confirm the power name in `docs/supervive-nov-2024-open-beta-architecture-audit.md`.
- Choose one activation type: `Active`, `Passive`, or `Toggle`.
- Record any known aliases in the data asset description.
- If tooltip behavior is uncertain, implement only after footage, patch notes, or data evidence confirms it.

## 2. Create the Gameplay Implementation

- Active powers: create a `UBBPowerGameplayAbility` subclass.
- Passive powers: prefer `StatBonuses` if possible; use a passive `UBBPowerGameplayAbility` only for ticking or triggered behavior.
- Toggle powers: create a `UBBPowerGameplayAbility` subclass that remains active until canceled.
- For projectiles, start from the existing projectile path or add a small power-specific projectile actor.
- For deployables, start from `ABBPowerDeployableActor`.

## 3. Create the Power Definition

- Create a `UBBPowerDefinition` data asset when content workflow is ready.
- Set:
  - `PowerID`
  - `DisplayName`
  - `Description`
  - `Rarity`
  - `ActivationType`
  - `GrantedAbilityClass`
  - `CooldownSeconds`
  - `bSoulbound`
  - `bDropOnDeath`
- Leave `PowerInputTagOverride` empty unless the power must ignore the equipped slot's F/G binding.

## 4. Test the Equip Pipeline

- Use `DebugGivePower <PowerID> 1` for F.
- Use `DebugGivePower <PowerID> 2` for G.
- Use `DebugDropPower <1|2>` to verify cleanup.
- Use `DebugSpawnPowerPickup <PowerID>` to verify world pickup flow.
- Use `DebugPrintPowers` to confirm replicated slot state.

## 5. PIE Acceptance Checklist

- F/G HUD shows the equipped power ID.
- Active powers fire from the correct slot and show cooldown.
- Passive powers apply immediately on equip and are removed on drop.
- Replacing a slot removes the old power's effect and ability.
- Power pickup works with `E`.
- Dedicated server and simulated clients see replicated effects and spawned actors.

## Built-In Reference Powers

These are registered in code so the pipeline can be tested before editor assets exist:

- `BungeeShot`: active projectile power.
- `EmergencyPlatform`: active deployable platform power.
- `RegenerativeArmor`: passive armor repair power.
