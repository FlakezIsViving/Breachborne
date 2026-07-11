# Networking Cheatsheet

Quick-reference code patterns for UE 5.7 multiplayer work. Load this when implementing replicated state, RPCs, or GAS abilities.

---

## Replicated property

Header (.h):

```cpp
UPROPERTY(ReplicatedUsing=OnRep_Health)
float Health;

UFUNCTION()
void OnRep_Health();
```

Implementation (.cpp):

```cpp
void AMyActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMyActor, Health);
}

void AMyActor::OnRep_Health()
{
    // React to replicated change on clients
    // E.g., update UI, play hit reaction, etc.
}
```

**Always remember:**
- Call `Super::GetLifetimeReplicatedProps` first
- Set `bReplicates = true` in the constructor
- For ActorComponents: also set `SetIsReplicatedByDefault(true)` in constructor

---

## Conditional replication (DOREPLIFETIME_CONDITION)

When you want a property to replicate only to specific recipients:

```cpp
void AMyActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replicate only to the owning client (e.g., personal inventory)
    DOREPLIFETIME_CONDITION(AMyActor, PrivateInventory, COND_OwnerOnly);

    // Replicate to everyone except the owning client
    DOREPLIFETIME_CONDITION(AMyActor, PublicState, COND_SkipOwner);

    // Replicate only on initial spawn, never again
    DOREPLIFETIME_CONDITION(AMyActor, ImmutableConfig, COND_InitialOnly);
}
```

Common conditions:
- `COND_OwnerOnly` — only to the owning client
- `COND_SkipOwner` — to all clients EXCEPT owner
- `COND_InitialOnly` — once on spawn, never again
- `COND_SimulatedOnly` — only to simulated proxies (other clients)

---

## Server RPC (client requests action)

Header (.h):

```cpp
UFUNCTION(Server, Reliable, WithValidation)
void ServerRequestPurchase(int32 ItemID);
```

Implementation (.cpp):

```cpp
bool AMyActor::ServerRequestPurchase_Validate(int32 ItemID)
{
    // Anti-cheat sanity checks. Return false to disconnect the client.
    return ItemID >= 0 && ItemID < MaxItemID;
}

void AMyActor::ServerRequestPurchase_Implementation(int32 ItemID)
{
    // Server-side logic. Validate game state, change replicated properties.
    if (!HasAuthority()) return; // defensive — should always be true here

    if (!IsItemAffordable(ItemID)) return;
    if (!IsNearShopkeeper()) return;

    DeductGold(GetItemCost(ItemID));
    AddItemToInventory(ItemID);
}
```

**Reliability choices:**
- `Reliable` — guaranteed delivery, ordered. Use for state-changing actions.
- `Unreliable` — may drop, no ordering guarantee. Use for cosmetics or high-frequency updates.
- `WithValidation` — pairs with a `_Validate` function. Use for any RPC that comes from a client.

---

## Client RPC (server tells specific client)

Header (.h):

```cpp
UFUNCTION(Client, Reliable)
void ClientShowDamageNumber(float Damage, FVector Location);
```

Implementation (.cpp):

```cpp
void AMyActor::ClientShowDamageNumber_Implementation(float Damage, FVector Location)
{
    // Runs on the owning client only. Safe to spawn UI here.
    if (!IsLocallyControlled()) return; // defensive
    SpawnDamageWidget(Damage, Location);
}
```

Common pitfall: Client RPCs require the actor to have an owning connection. Most often called on a `PlayerController` (which has a clear owning client by definition).

---

## Multicast RPC (server tells all clients)

Header (.h):

```cpp
UFUNCTION(NetMulticast, Unreliable)
void MulticastPlayHitEffect(FVector Location, FRotator Rotation);
```

Implementation (.cpp):

```cpp
void AMyActor::MulticastPlayHitEffect_Implementation(FVector Location, FRotator Rotation)
{
    // Runs on server AND all clients. Use for cosmetic-only effects.
    UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitVFX, Location, Rotation);
    UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, Location);
}
```

**Use multicasts sparingly.** They're bandwidth-heavy at scale. For ability effects, prefer GameplayCues — they handle prediction, replication, and culling automatically.

---

## Authority and locality checks

```cpp
// Server-only logic
if (HasAuthority())
{
    // Modify replicated state, spawn authoritative actors, validate inputs
}

// Owning-client cosmetics
if (IsLocallyControlled())
{
    // Update local UI, play first-person VFX, predict actions
}
```

**NEVER use `!HasAuthority()` to mean "client".** It's wrong on listen servers (where the host has authority AND is local). Use `IsLocallyControlled()` for client-cosmetic intent. We don't support listen server, but the habit avoids confusion.

---

## GAS ability activation patterns

The Net Execution Policy of a `UGameplayAbility` determines how it replicates. Choose based on the ability's role:

```cpp
// In ability constructor or class default

// Combat skillshots (LMB, RMB, Q, Shift) — client predicts, server confirms/corrects
NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

// Ultimates (R) — server runs and replicates result; no prediction
NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

// Server-only systems (e.g., applying a passive buff on level up)
NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

// Client-only cosmetics (e.g., emote)
NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
```

---

## GAS — applying damage through GameplayEffectExecutionCalculation

NEVER use raw `ApplyDamage` or `TakeDamage`. All damage flows through `UBBDamageExecution`.

```cpp
// In an ability's ActivateAbility (or equivalent):

if (HasAuthority(&CurrentActivationInfo))
{
    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
    if (TargetASC)
    {
        FGameplayEffectContextHandle Context = MakeEffectContext();
        FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, Level);
        // Set magnitude via Set-By-Caller or attribute capture
        Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), DamageAmount);
        ApplyGameplayEffectSpecToTarget(Spec, TargetASC);
    }
}
```

The actual damage calculation happens in `UBBDamageExecution::Execute_Implementation`, which captures source attributes (AttackPower, AbilityPower) and target attributes (Armor, Shield) and produces the final value.

---

## GAS — GameplayCues (do this, not direct VFX spawning)

Don't do this:

```cpp
// WRONG — server-side spawn, may not replicate properly, no prediction
UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MyVFX, HitLocation);
```

Do this:

```cpp
// RIGHT — GameplayCue handles all the network nuance
FGameplayCueParameters CueParams;
CueParams.Location = HitLocation;
CueParams.Normal = HitNormal;
TargetASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(FName("GameplayCue.Hit.Default")), CueParams);
```

Then create a `UGameplayCueNotify_Static` (one-shot) or `AGameplayCueNotify_Actor` (persistent) Blueprint matching that tag. The cue handles client-side spawning, prediction, and culling.

---

## Common pitfalls (catch in review)

1. **Forgetting `DOREPLIFETIME`** for a UPROPERTY marked `Replicated` or `ReplicatedUsing`
2. **Forgetting `Super::GetLifetimeReplicatedProps(OutLifetimeProps)`** — parent properties silently stop replicating
3. **Calling `GetGameMode()` on a client** — returns nullptr. GameMode is server-only. Use `GameState` for shared data.
4. **Spawning UMG widgets on dedicated server** — crashes the server. Guard with `IsLocallyControlled()` or keep UI code in client-only paths.
5. **Missing `_Implementation` suffix** when defining the function body of a UFUNCTION RPC
6. **Using raw `ApplyDamage`** instead of GAS DamageExecution — breaks the entire attribute system
7. **Putting ASC on Character** instead of PlayerState — abilities and cooldowns vanish on respawn
8. **Skipping `OnRep_PlayerState`** — ASC binding only works through `PossessedBy` on the server side; clients need OnRep too
9. **Spawning particles directly from ability code** instead of via GameplayCues — desyncs and prediction issues
10. **Hardcoding tick rates or timers in unscaled real time** — server tick rate may differ from client; use `GetWorld()->GetDeltaSeconds()` and trust UE's time system

---

## Quick reference: which class lives where

When you don't remember whether a piece of state should live on Character, PlayerState, GameState, or GameMode, ask:

- **Does it survive pawn respawn?** → PlayerState (or higher)
- **Is it shared across the whole match?** → GameState
- **Is it server-only logic?** → GameMode
- **Is it tied to the physical pawn instance?** → Character
- **Is it client-local UI state?** → PlayerController or local widget

Common mappings:
- Health, abilities, attributes → PlayerState (via ASC)
- Position, velocity, animation state → Character
- Match phase, storm state, alive teams → GameState
- Spawn rules, win condition checks, lobby logic → GameMode
- Cursor world position, input state, local camera → PlayerController
