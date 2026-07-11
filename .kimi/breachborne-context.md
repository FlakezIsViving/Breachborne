# Breachborne — Kimi Context Reference

**Generated:** 2026-05-11
**Game:** Top-down isometric squad BR inspired by Supervive (Open Beta era). NO persistent unlocks.
**Engine:** UE 5.7 (pinned) | **Network:** Dedicated server only | **Squad size:** 4

---

## Current Implementation State (vs May 2 Audit)

The May 2 audit marked many systems "not started," but **substantial code now exists** for Phases 7–9. Likely added between May 2 and May 11.

| Phase | System | Status | Notes |
|-------|--------|--------|-------|
| 0 | Hygiene | 🟡 | `BB` prefix inconsistent on core classes (`BreachborneGameMode`, `HunterCharacter`, etc.) |
| 1 | Movement + Camera | ✅ | Top-down CMC, cursor aim replication at 20Hz |
| 2 | GAS + Ghost | ✅ | Full Ghost kit (LMB/RMB/Q/R/Shift/Passive). Damage only via `UBBDamageExecution` |
| 3 | Gliding + Spiking | ✅ | `UGliderComponent`, heat, spike on damage, abyss instant kill |
| 4 | Wisp + Revival | 🟡 | Wisp (rez/stomp/execute) done. **Deathbox, Beacon, MostWanted** have cpp files (~500+ lines) but need verification |
| 5 | Storm | 🟡 | Shrinks + damages. `BBStormShift_BulletTrains.cpp` exists (161 lines) but **no train visuals** — needs more work |
| 6 | Items/Economy | 🟡 | Inventory manager, shopkeeper, shards, powers, consumables. Some placeholder `NewObject<>` usage |
| 7 | PvE | 🟡 | **All headers + cpp exist**: `BBCreepCamp`, `BBCreepCharacter`, `BBBossCharacter`, `BBBasecampActor`, `BBVaultInteractable`, `BBContractSubsystem`. Need runtime verification |
| 8 | Mantling | 🟡 | `BBMantleComponent` + custom CMC mode `MOVE_Mantling` exist (169 lines). Need test |
| 9 | Kingpin | 🟡 | **All 6 abilities + passive have cpp** (~1100 lines total). Need test in PIE |
| 10 | Polish | 🟡 | Debug HUD, vision cone component, brush volume exist. No UMG production UI |

**Biggest uncertainty:** Whether the newly-added systems (Kingpin, PvE, Mantle, Deathbox, BulletTrains) actually compile and run, or if they are partially wired.

---

## UE 5.7 Notes (Relevant to This Project)

- **No breaking GAS/CMC/networking API changes** in 5.7. Project compiles clean.
- `FNonshrinkingAllocator` now available for TArray if we ever need containers that only compact on explicit `Shrink()`.
- Experimental `SpringMath` API added (spring damping functions) — could be useful for camera/UI animations later.
- `TIsConst` deprecated, `PLATFORM_COMPILER_SUPPORTS_CONSTEXPR_BUILTIN_FILE_AND_LINE` deprecated — not used in this codebase.
- Visual Studio 2026 support is experimental; MSVC v14.50 is **not** supported. Current build uses VS 2022 (safe).
- **No action needed** — the upgrade from 5.6→5.7 is transparent for this project's C++ gameplay code.

---

## Architecture Pillars

### GAS Setup
- **ASC lives on `ABreachbornePlayerState`** — survives respawn
- **Replication Mode:** Mixed
- **Attribute Sets:** `UBBHealthSet`, `UBBCombatSet`, `UBBMovementSet`, `UBBWispHealthSet`
- **Custom proxy replication:** `FRepHealthData`, `FRepCombatData`, etc. replicate as PlayerState properties (workaround for UE subobject misrouting). `UpdateHealthProxy()` pushes values to clients.
- **Net Execution Policy:**
  - LMB/RMB/Q/Shift → `LocalPredicted`
  - R (Ultimate) → `ServerInitiated`
  - Passive → `ServerOnly`

### Damage Pipeline
```
Ability → MakeOutgoingGameplayEffectSpec(DamageEffectClass)
       → SetSetByCallerMagnitude(Data.Damage, Amount)
       → ApplyGameplayEffectSpecToTarget
       → UBBDamageExecution::Execute_Implementation
       → captures Source AttackPower/Crit + Target Armor/Shield
       → outputs to IncomingDamage meta attribute
       → PostGameplayEffectExecute → Health -= IncomingDamage
```
**NEVER** use `ApplyDamage` / `TakeDamage`.

### ASC Binding (Lyra Pattern)
`AHunterCharacter` binds in **both** places:
- `PossessedBy()` → server path
- `OnRep_PlayerState()` → client path (with retry timer fallback)

### Inventory
- `FRepInventoryData` replicated on PlayerState (`OnRep_InventoryData`)
- `FBBInventoryManager` — static methods, server-only, mutate PS + apply/remove GEs
- Equipment slots: Weapon, Helmet, Boots, Armor
- 2 Power slots (grant abilities to ASC)
- Consumable backpack (hotkeys 1–4)

---

## Class Prefix Conventions

| Prefix | Meaning |
|--------|---------|
| `ABB…` | Actor (e.g., `ABBDeathboxActor`) |
| `UBB…` | UObject / Component (e.g., `UBBAbilitySystemComponent`) |
| `FBB…` | Struct (e.g., `FBBInventoryManager`) |
| `EBB…` | Enum (e.g., `EBBContractType`) |

**Violations in core:** `ABreachborneGameMode`, `ABreachborneGameState`, `ABreachbornePlayerState`, `ABreachbornePlayerController`, `AHunterCharacter`, `UGliderComponent`, `AAbyssKillVolume`. These use full names or no `BB` prefix.

---

## Key Source Directories

```
Source/Breachborne/
  Core/         — GameMode, GameState, PlayerController, PlayerState, Types
  Characters/   — HunterCharacter, BBCharacterMovementComponent, GliderComponent, BBMantleComponent
  Abilities/    — ASC, AttributeSets, GameplayTags, GameplayAbility base, HunterDefinition
    Hunters/Ghost/   — 6 ability classes (fully implemented)
    Hunters/Kingpin/ — 7 ability classes (implemented, untested)
  Combat/       — DamageExecution, Projectiles, Grenade, NapalmZone, Status GEs (Stun, Daze, Knockup, Hook)
  Items/        — InventoryManager, EquipmentDefinition, PowerDefinition, ConsumableDefinition, Shopkeeper, WorldItem, ShardPickup
  Storm/        — StormManager, StormShiftBase, StormShift_Default, StormShift_BulletTrains
  PvE/          — CreepCamp, CreepCharacter, BossCharacter, BasecampActor, VaultInteractable, ContractSubsystem
  Wisp/         — BBWispPawn, BBDeathboxActor
  Revival/      — BBRespawnBeacon, BBMostWantedCrown
  Vision/       — BBVisionConeComponent, BBBrushVolume
  UI/           — BBDebugHUD (Canvas-based)
  Tests/        — BB_Spec_PureLogic, BB_Spec_GameSystems (automation specs)
  World/        — AbyssKillVolume
```

---

## Networking Rules (Hard)

1. **Never** `GetGameMode()` on clients → nullptr
2. **Never** spawn UMG widgets in server code paths → crash
3. **Never** `GetFirstLocalPlayerController()` on server → nullptr
4. All replicated props need `DOREPLIFETIME` + `Super::GetLifetimeReplicatedProps`
5. RPC impls use `_Implementation` suffix
6. Use `IsLocallyControlled()` for client cosmetics, NOT `!HasAuthority()`
7. Server RPCs should use `WithValidation` (project currently doesn't — deferred anti-cheat)

---

## Testing Conventions

- **PureLogic specs:** `Breachborne.PureLogic` — enum values, formula verification, struct defaults. Run on every compile.
- **GameSystems specs:** `Breachborne.GameSystems` — UWorld-based (GameState, ContractSubsystem, BrushVolume). Run before PR merge.
- **PIE checklist:** 3 clients + dedicated server. `docs/PIEChecklist.md` has per-phase manual checks.
- **Dev auto-respawn:** `bDevAutoRespawn = true` in GameMode — auto-respawns dead hunters after 3s for iteration. Disable for production deathbox/beacon flow.

---

## Roadmap / Deferred Work

### High Priority (Testing & Wiring)
- [ ] **PIE test Kingpin full kit** — all abilities granted, no crashes, replication correct
- [ ] **PIE test Deathbox revive channel** — interact, progress bar, cancel on damage, completion
- [ ] **PIE test Respawn Beacon** — activation, squad revive, consumption
- [ ] **PIE test Most Wanted Crown** — pickup, broadcast, survive timer, squad revive
- [ ] **PIE test Mantle** — edge detection, MOVE_Mantling replication, animation
- [ ] **PIE test PvE actors** — CreepCamp spawn, AI movement/leash, Boss spawn, Vault hack, Basecamp capture
- [ ] **PIE test Bullet Trains shift** — speed buff GE applied, storm center drifts, faster shrink
- [ ] **Fix Shopkeeper `NewObject<>` stub** — replace with real `UBBItemRegistry` lookup

### Medium Priority (Polish & Completion)
- [ ] **Gold steal on PvP kill** — formula exists in `AwardKillRewards`, verify in PIE
- [ ] **Contract system wiring** — `ReportKill` and `ReportCampClear` called, payout works
- [ ] **Staggerable mobs** — windup window, ability interrupt, cooldown refund
- [ ] **Vision cone + brush stealth** — client-side rendering, day/night range scaling
- [ ] **Day/night cycle gating level cap** — `AdvanceDayNightPhase` exists but level cap logic not wired

### Low Priority / Deferred
- [ ] **Prefix refactoring** — rename core classes to `ABBGameMode`, `ABBHunterCharacter`, etc. **BLOCKED:** breaks Blueprint references. Do after content freeze or with heavy BP fixup.
- [ ] **Anti-cheat / RPC validation** — add `WithValidation` to Server RPCs
- [ ] **Custom ReplicationGraph** — deferred per CLAUDE.md until 16+ player tests show degradation
- [ ] **Production UMG UI** — replace Canvas debug HUD
- [ ] **Audio polish** — GameplayCue Blueprints for VFX/SFX

---

## Known Open Questions / Risks

1. **Prefix refactoring:** Should old core classes be renamed to `ABBGameMode`, `ABBHunterCharacter`, etc.? Large refactor, lots of Blueprint references would break.
2. **GAS proxy replication:** The `SpawnedAttributes` removal + proxy struct pattern is non-standard but documented. If upgrading UE versions, verify this still works.
3. **Kingpin abilities:** Code exists but has never been tested in PIE. Hook pull lives on `AHunterCharacter` (not the ability) for timer stability — verify this doesn't break with multiple Kingpins.
4. **PvE AI:** `AIModule` is in Build.cs but no BehaviorTree/AIController references seen in C++. Creep AI may be Blueprint-driven or not yet wired.
5. **Mantle CMC mode:** `EBBCustomMovementMode::Mantling = 3` is pinned in tests. `BBCharacterMovementComponent::PhysCustom` must dispatch to `PhysMantling`.
6. **Storm shift wiring:** `BBStormShift_BulletTrains` exists but train visuals are missing. Need to add moving train actors or FX.
7. **Deathbox revive:** `BBDeathboxActor.cpp` is 213 lines — likely functional, but need to verify `ServerBeginReviveChannel` is called from PlayerController input.
8. **Most Wanted Crown:** 219 lines — has holder tracking, broadcast timer, squad revive on survival. Verify integration with GameMode.

---

## Workflow Notes

- Propose plan for non-trivial work, await confirmation before coding.
- Compile in chunks: header → review → cpp → compile → PIE with 3 clients.
- Use `/clear` between unrelated tasks.
- When unsure about UE 5.7 API, say so rather than guess.
- **Out of scope:** Anti-cheat, matchmaking, save data, listen server, console ports, custom ReplicationGraph.
