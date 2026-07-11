# Architecture Deep Dive

## Server-authoritative pattern (every interaction)

```
Client Input -> Server RPC -> Server Validates -> Server Changes State -> State Replicates -> Client Reacts
```

Concrete example — **shopkeeper purchase:**

1. Client UI calls `ServerRPC_RequestPurchase(ItemID)`
2. Server validates: proximity to shopkeeper, gold balance, slot availability, item exists
3. Server deducts gold, adds to `PlayerState->EquipmentLoadout`
4. Loadout replicates → `OnRep_EquipmentLoadout` fires on owner
5. UI updates, item GE applies to ASC, attribute changes replicate

This pattern applies to EVERY player-initiated state change. No exceptions.

---

## ASC binding pattern (the part most people get wrong)

ASC lives on PlayerState. The pawn must bind to it in TWO places because the order of `OnRep_PlayerState` and `PossessedBy` differs between server and client.

On the server, `PossessedBy` runs first. On clients, `OnRep_PlayerState` runs first. If you only handle one, the other path silently breaks ability granting on respawn.

```cpp
// In ABBHunterCharacter

void ABBHunterCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    InitAbilityActorInfo(); // server path
}

void ABBHunterCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    InitAbilityActorInfo(); // client path
}

void ABBHunterCharacter::InitAbilityActorInfo()
{
    ABBPlayerState* PS = GetPlayerState<ABBPlayerState>();
    if (!PS) return;

    UBBAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
    ASC->InitAbilityActorInfo(PS, this); // owner = PS, avatar = pawn

    // Server-only: grant abilities here based on selected hunter
    if (HasAuthority())
    {
        GrantHunterAbilities();
    }
}
```

**Reference implementation:** Lyra's `ULyraPawnExtensionComponent` is the production-grade version. Copy the full pattern, not just the bind step.

---

## Top-down networking specifics

Top-down games have different networking needs than FPS/TPS. Key differences:

**Aim replication:** In FPS, aim is a camera rotation. In top-down, aim is a world-space cursor point (where the cursor intersects the ground plane). Replicate the aim point as `FVector_NetQuantize` on the character at ~20Hz. Other clients use it to orient the model and predict ability directions.

**Movement settings for top-down:**
```cpp
GetCharacterMovement()->bOrientRotationToMovement = false;
GetCharacterMovement()->bUseControllerDesiredRotation = true;
GetCharacterMovement()->RotationRate = FRotator(0, 720, 0); // fast snap to cursor
```

**Skillshot targeting:** Client sends world-space target via `WaitTargetData`. Server validates the target is within reasonable range of the client's replicated position (anti-cheat tolerance, even though we're deferring real anti-cheat).

**Camera:** NOT replicated. Local-only SpringArm at ~55-65°. Each client manages their own camera independently. Edge-of-screen panning is optional.

---

## GAS replication notes for MOBA abilities

**Replication Mode: Mixed** — full ability replication to owning client, minimal (tags + attributes only) to simulated proxies. This saves bandwidth significantly with multiple players in view.

**Cooldown prediction:** GAS predicts cooldowns locally. High-latency clients see slightly longer effective cooldowns due to server-side correction. Acceptable for BR scope. For high-impact ultimates, prefer ServerInitiated to remove prediction entirely.

**GameplayCues:** Never spawn VFX/sounds from ability logic directly. Use GameplayCues — they're designed to be client-only and handle prediction correctly. Server says "play cue X at location Y", clients handle locally.

**Ability input flow for top-down:**
```
Player presses key
  -> Enhanced Input fires
  -> PlayerController routes to ASC
  -> ASC->TryActivateAbility(GameplayTag)
  -> Ability uses GetAimLocation() (cursor world projection)
  -> Server validates aim point is within tolerance
  -> Ability executes
```

For skillshots: use `AbilityTask_WaitTargetData` with a custom `AGameplayAbilityTargetActor` that does a line trace along the cursor direction. Client sees targeting indicator immediately; server receives target data and fires the actual damaging trace.

---

## Gliding architecture

`UGliderComponent` is server-authoritative on key state, client-predicted on input.

**Server-replicated state:**
- `EGliderState` — Closed, Open, Spiked
- `float FuelLevel` — 0 to 1
- `float FuelDrainRate` / `FuelRefillRate` — server-calculated glider resource tuning

**Client predicts:** opening and closing the glider for responsiveness. Server confirms or corrects.

**Spike logic (server only):**
- On any damage received while `GliderState == Open`: transition to `Spiked`
- Spiked: disable input, apply downward velocity, play stun animation
- If spiked over abyss (Z below `AbyssThreshold`): instant kill, deathbox spawns at attacker location
- Spike state duration ~1.5s, then transitions to falling normally

**Abyss detection:** large kill volume below the map. Additionally, a `PostProcessVolume` or `ZoneVolume` triggers the danger-zone UI indicator on clients before death.

---

## Wisp lifecycle

When a hunter's HP reaches 0:

```
HP reaches 0
|
+-- Fall/abyss death  ->  Skip wisp, instant deathbox at killer location
|
+-- Normal death  ->  Transition to WISP STATE
                      |
                      +-- Wisp HP drains over time (server tick)
                      +-- Ally proximity  ->  rez bar fills  ->  REVIVED (partial HP, keep items)
                      +-- Enemy proximity  ->  HP drains FASTER (stomp)
                      +-- Enemy presses E  ->  Execute animation  ->  kill + executor full heal
                      +-- Carry-capable ally  ->  pick up wisp  ->  reposition
                      +-- Wisp HP reaches 0  ->  DEATHBOX
                                                |
                                                +-- Enemies loot equipment, powers, armor
                                                +-- Ally channels deathbox revive (loud, interruptible)
                                                +-- Respawn Beacon  ->  full squad revive
                                                +-- Most Wanted crown  ->  2 min survive  ->  squad revive
```

`AWispPawn` properties (all replicated): `WispState`, `WispHP`, `RezBarProgress`, `CarriedBy`, `OwningPlayerState`.

**Server tick every 0.2s:** checks proximity (~2m radius), updates rez bar / HP drain accordingly. Multiple allies don't stack rez speed (one is enough). Both ally AND enemy nearby: rez bar fills, but HP still drains (reduced rate vs enemy-only).

**Wisp is invulnerable to direct damage** — only proximity and execute affect it. Cannot be "shot."

**Execute mechanic:** Enemy E within range starts a server-locked animation (~2s). Executor cannot move or use abilities. If executor takes damage during animation, execute cancels (server validates). On completion: wisp dies → deathbox + executor fully healed (HP and mana).

---

## Replication Graph (DEFERRED until you actually need it)

A custom ReplicationGraph improves perf at scale (36+ players, hundreds of PvE actors). Default replication is fine for prototype scope and 3-client PIE testing.

**When to add it:** when you start running real network tests with 16+ concurrent players and the server tick rate degrades. Not before. Building a custom graph too early is 1-2 weeks of work that solves a problem you don't have yet.

**Eventual node setup (for reference, when the time comes):**
- Grid-based spatial node — divide map into cells, only replicate nearby actors
- Always-relevant node — GameState, PlayerStates, StormManager
- Team node — squad members always relevant to each other regardless of distance
- Dynamic frequency node — far-away actors replicate less often

---

## Anti-cheat (DEFERRED)

Trust the client for prototype testing. Server validates targeting within reasonable tolerance, but no EAC/BattlEye integration. This MUST be addressed before any public playtest, but is out of scope for the prototype phase.

Practical implication: don't over-engineer anti-exploit measures during prototype. Server-side validation of state-changing actions is sufficient. If players can hack their way into invalid states in PIE, that's fine for now.
