# Project: Breachborne

Top-down isometric squad BR with MOBA hero abilities. Inspired by Supervive (Open Beta era — NO Armory).

**Engine:** UE 5.7 (pinned in .uproject — do not auto-update)
**Language:** C++ primary, Blueprints for tuning/data only
**Network:** Dedicated server only — no listen server, no singleplayer
**Squad size:** 4 (fixed)
**Solo dev project.**

## Performance Contract
- Target playable 60+ FPS on roughly 10-year-old PC hardware at 1080p Low/Medium.
- Favor gameplay-readable, low-cost visuals over high-density cinematic VFX.
- VFX/SFX must use GameplayCues with scalability/culling budgets; avoid direct spawning from ability code.
- Every new visual feature needs a low-spec fallback or quality-scaled version.
- Avoid heavy GPU simulation, Niagara fluids, dense translucent clouds, excessive overdraw, large particle counts, and permanent high-frequency tick effects.

## On-demand context (load when relevant, NOT every session)
- `@docs/architecture.md` — full system architecture, GAS deep-dive, replication patterns
- `@docs/phases.md` — phased build plan with exit criteria
- `@docs/supervive-reference.md` — full mechanics spec (gliding, wisp, items, storm, etc.)
- `@docs/networking-cheatsheet.md` — RPC/replication code patterns

## Core Identity
- Top-down camera (~55-65° SpringArm), NOT first/third person
- Floating island map with abyss — falling = instant death
- Physics-based gliding; damage while gliding = spike (instant kill over abyss)
- Wisp downed state: HP drains, ally proximity revives, enemy proximity drains, enemy E executes
- Open Beta items: equipment slots + 2 power slots + consumables, upgraded via shards. NO persistent unlocks.
- Day/night cycle gates level cap; storm shrinks; storm shifts mutate match rules
- 5 roles: Fighter, Initiator, Frontliner, Protector, Controller

## UE Coding Standards
- Prefixes: A (Actors), U (UObjects), F (structs), E (enums), I (interfaces), T (templates)
- Project class prefix: `BB` (e.g., `ABBHunterCharacter`, `UBBAbilitySystemComponent`)
- UPROPERTY/UFUNCTION required for reflected members
- `TObjectPtr<>` not raw pointers for UObject refs
- Forward declarations in headers, full includes in .cpp only
- `#pragma once`, never include guards
- `Category = "Breachborne|<SubCategory>"` for Blueprint-exposed members
- Enhanced Input only — NOT legacy input

## Networking Rules (CRITICAL)
- All gameplay state changes happen on server (`HasAuthority()` check)
- Client requests via Server RPCs → server validates → replicates state
- **NEVER** create UMG widgets in server code paths — crashes dedicated server
- **NEVER** call `GetGameMode()` on clients — returns nullptr
- **NEVER** call `GetFirstLocalPlayerController()` on server — returns nullptr
- Replicated properties need `DOREPLIFETIME` in `GetLifetimeReplicatedProps`
- Always call `Super::GetLifetimeReplicatedProps` first
- RPC implementations use `_Implementation` suffix
- Use `IsLocallyControlled()` for client cosmetic intent, NOT `!HasAuthority()`
- Movement uses `CharacterMovementComponent` (built-in replication)
- Skillshots: spawn on server, client predicts locally for responsiveness

## Class Presence on Network

| Class | Server | Owning Client | Other Clients |
|-------|--------|---------------|---------------|
| ABBGameMode | Yes | No | No |
| ABBGameState | Yes | Yes | Yes |
| ABBPlayerState | Yes (all) | Yes (all) | Yes (all) |
| ABBPlayerController | Yes (all) | Yes (own) | No |
| ABBHunterCharacter | Yes (all) | Yes (all) | Yes (all) |
| AWispPawn | Yes (all) | Yes (all) | Yes (all) |
| ADeathboxActor | Yes (all) | Yes (all) | Yes (all) |
| AHUD / UUserWidget | No | Yes | Yes |
| UAbilitySystemComponent | Yes (on PS) | Yes (on PS) | Yes (minimal) |

## GAS Architecture (canonical decisions)
- **ASC lives on `ABBPlayerState`**, NOT on Character (survives respawn)
- Replication Mode: **Mixed** (full to owner, minimal to simulated proxies)
- Pawn binds ASC via `IAbilitySystemInterface` in BOTH `OnRep_PlayerState` AND `PossessedBy` — copy Lyra's pattern in full
- Attribute Sets: `UBBHealthSet`, `UBBCombatSet`, `UBBMovementSet`
- Damage flows ONLY through `UBBDamageExecution` (GameplayEffectExecutionCalculation). NEVER raw `ApplyDamage`.
- Cooldowns are GameplayEffects, NOT manual timers
- VFX/SFX via GameplayCues, NOT direct spawning from ability code
- Net Execution Policy:
  - Combat (LMB/RMB/Q/Shift): `LocalPredicted`
  - Ultimates (R): `ServerInitiated`
  - Passives: `ServerOnly`
  - Cosmetic-only: `LocalOnly`
- GameplayTag namespace: `Ability.Hunter.<HunterName>.<Slot>`

## Project Structure

```
Source/Breachborne/
  Core/         - GameMode, GameState, GameInstance, MatchPhaseManager
  Characters/   - HunterBase, HunterMovement, GliderComponent, MantleComponent
  Abilities/    - GAS setup, ASC, AttributeSets
    Hunters/<HunterName>/  - per-hunter ability classes
  Combat/       - DamageExecution, HitDetection, Projectiles
  Items/        - ItemBase, Shopkeeper, EquipmentSlots, UpgradeShards, PowerPickup
  Storm/        - StormManager, StormShiftBase, ZoneVolumes
  PvE/          - CreepCamp, BossBase, VaultInteractable, ContractSystem
  Wisp/         - WispPawn, WispCarryAbility, FinisherAbility, DeathboxActor
  Revival/      - RespawnBeacon, Everbeacon, MostWantedBounty
  Vision/       - VisionConeComponent, FogOfWarManager, BrushVolume
  UI/           - HUD, Minimap, ScoreBoard, ShopWidget (CLIENT-ONLY)
  Networking/   - ReplicationHelpers, prediction utilities
```

## Common Mistakes to Catch in Review
1. Forgetting `DOREPLIFETIME` for a replicated property
2. Forgetting `Super::GetLifetimeReplicatedProps` call
3. Calling `GetGameMode()` on client code paths
4. Spawning UMG widgets in server code paths
5. Missing `_Implementation` suffix on RPC impls
6. Using raw `ApplyDamage` instead of GAS pipeline
7. Putting ASC on Character instead of PlayerState
8. Not handling `OnRep_PlayerState` for ASC binding on respawn
9. Spawning particles directly from ability code instead of GameplayCues
10. Pre-optimizing with custom ReplicationGraph before it's needed (it isn't, until it is)

## Workflow for Claude
- For non-trivial work: propose a plan first, await confirmation before coding
- Compile in chunks: header → review → cpp → compile → test in PIE with 3 clients
- Use `/clear` between unrelated tasks to free context
- When you don't know a UE 5.7 API specifically, say so and reference docs rather than guessing

## Out of Scope (do not propose work in these areas)
- Anti-cheat (deferred to post-prototype)
- Account systems / matchmaking (deferred — IP-based connection for prototype)
- Save data / persistence (no between-match progression by design)
- Listen server / split-screen / singleplayer modes
- Console ports (PC-only)
- Custom ReplicationGraph (default replication is fine until concurrent player tests demand otherwise)

## Do Not Modify
- `Engine/`, `Intermediate/`, `Saved/`, `Binaries/`, `DerivedDataCache/`
- `Content/` (`.uasset`, `.umap` — binary)
- `*.generated.h` (UHT-generated)
- `.vs/`, `.idea/`
