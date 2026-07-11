# Build Phases

Each phase ends with measurable exit criteria. Don't move on until criteria are met.
Test in PIE with 3 clients (dedicated server mode) at every phase end.

---

## Phase 0: Project hygiene (1-2 days)
**Goal:** Clean foundation before adding complexity.

- [ ] `.uproject` engine version pinned to UE 5.7
- [ ] Source build folders match the structure in CLAUDE.md
- [ ] `BB` class prefix applied consistently
- [ ] PIE multiplayer settings: 3 clients, "Play as Client" net mode
- [ ] `.gitignore` excludes Intermediate/, Saved/, Binaries/, DerivedDataCache/
- [ ] `Build.cs` includes only modules actually used (audit existing)

**Exit criteria:** Project compiles clean. PIE launches with 3 clients connecting to a dedicated server. No warnings about deprecated APIs.

---

## Phase 1: Networked movement foundation
**Goal:** 3 players moving on a test map with top-down camera, server authoritative.

Build (or audit existing for):
- `ABBGameMode` — server only, lobby/spawn/match state, fixed squad size 4
- `ABBGameState` — replicates MatchPhase, StormState, ElapsedTime, AliveTeamCount
- `ABBPlayerState` — replicates TeamID, HunterID, Level, XP, IsAlive; OWNS the ASC
- `ABBPlayerController` — top-down input (WASD + cursor aim), screen-to-world cursor projection
- `ABBHunterCharacter` — uses `CharacterMovementComponent`, planar XY movement, character faces cursor not movement direction

**Top-down movement settings:**
```cpp
GetCharacterMovement()->bOrientRotationToMovement = false;
GetCharacterMovement()->bUseControllerDesiredRotation = true;
GetCharacterMovement()->RotationRate = FRotator(0, 720, 0);
```

**Exit criteria:**
- 3 PIE clients can move around the test map, top-down camera
- Movement is smooth and snappy on the owning client
- Other clients see correct positions with no visible desync after 5 minutes
- Character body rotates to face cursor independently of movement
- Dedicated server window logs no errors

---

## Phase 2: GAS + first hunter abilities (Ghost as reference)
**Goal:** Ghost fully playable in multiplayer with all 5 abilities + passive.

GAS setup order:
1. `UBBAbilitySystemComponent` (custom ASC, lives on PlayerState, Mixed replication)
2. `UBBHealthSet` (Health, MaxHealth, Shield, MaxShield, Armor)
3. `UBBCombatSet` (AttackPower, AbilityPower, CooldownReduction, CritChance)
4. `UBBMovementSet` (MoveSpeed, GlideSpeed, DashDistance)
5. `UBBGameplayAbility` (base class enforcing NetExecutionPolicy by category)
6. `UBBDamageExecution` (the only damage path)

Ghost's abilities (canonical patterns — every other hunter follows these):

| Slot | Ability | GAS Pattern | Network |
|------|---------|-------------|---------|
| LMB | Automatic rifle | Repeating, hitscan | LocalPredicted, server-validated |
| RMB | Spike Grenade | Projectile + AoE daze GE | LocalPredicted, server spawns projectile |
| Shift | Combat Slide | Root motion dash, kill resets cd | LocalPredicted via CMC |
| Q | Piercing Laser | Line trace, damage GE | LocalPredicted, WaitTargetData |
| R | Napalm Strike | Server-spawned AoE zone, DOT | ServerInitiated |
| Passive | Kill resets Shift | GameplayCue on kill | ServerOnly |

**Exit criteria:**
- Ghost casts all abilities on owning client, server validates, simulated proxies see effects
- Damage applied through DamageExecution, attributes update on UI
- No desync between predicted and authoritative state under normal latency
- Cooldowns work via GE, persist through respawn (ASC on PS)

---

## Phase 3: Gliding & spiking
**Goal:** Glide between islands; damage while gliding spikes you; abyss kills.

`UGliderComponent` on HunterCharacter:
- Server-authoritative state: GliderState (Closed/Open/Spiked), FuelLevel, FuelDrainRate
- Client predicts open/close, server confirms
- Spike on damage while Open → Spiked → downward velocity + stun
- Spike over abyss (Z < AbyssThreshold) → instant kill, deathbox at attacker location

Abyss: large kill volume + danger-zone post-process.

**Exit criteria:**
- Hunter can glide across a gap between two islands
- Heat builds while gliding, resets on landing
- Taking damage mid-glide spikes the hunter (stun + fall)
- Spike over abyss kills instantly
- Spike state and heat replicate to all clients correctly

---

## Phase 4: Wisp system & revival chain
**Goal:** Full down → wisp → deathbox → beacon revival pipeline.

This is one of the signature mechanics. Get it right.

- `AWispPawn` (replicated, server-authoritative state)
- Wisp tick: HP drains; ally proximity fills rez bar; enemy proximity drains HP faster
- Execute (E key): server-locked animation, can be cancelled by damage, completes = full heal for executor
- Deathbox spawn on wisp HP = 0
- Deathbox revive: long channel, loud audio, interruptible
- Respawn Beacon: one-time, full squad revive
- Most Wanted crown: 2 min timer, broadcasts location, squad revive on success
- Wisp carry (Elluna-class ability) deferred until Phase 9 hunter implementations

**Exit criteria:**
- Player at 0 HP becomes wisp, not instant death (unless abyss)
- Ally can revive wisp by proximity
- Enemy can stomp or execute
- Deathbox spawns and is lootable
- Beacon revives entire dead squad
- All transitions replicate correctly under network jitter

---

## Phase 5: Storm system
**Goal:** Shrinking storm forces engagement; storm shifts mutate matches.

- `AStormManager` (server-only); state on GameState (`FStormCircleState`)
- Storm damage via DOT GameplayEffect to hunters outside safe zone
- Storm shifts: `UStormShiftBase` modular subclasses (Nomadic, Bullet Trains, Global Oracle, etc.)
- GameMode picks one at match start, replicates selection

**Exit criteria:**
- Storm visualizes and shrinks on schedule
- Hunters outside safe zone take ramping damage
- One storm shift implemented and verifiably alters match conditions

---

## Phase 6: Item system & match economy
**Goal:** Open Beta items — equipment, evolutions, shopkeepers, powers, consumables, gold.

(Detailed spec in `@docs/supervive-reference.md`.)

- Equipment loadout struct on PlayerState (replicated)
- 4 weapons × 3 evolutions, 3 helmets × 3 evolutions, boots, armor tiers
- `UUpgradeShard` + evolution choice flow
- `AShopkeeper` interactable with server-validated purchase
- 2 power slots from vaults/bosses (powers grant abilities via ASC)
- Consumable backpack (food, vives, armor shards)
- Gold economy

**Exit criteria:**
- Hunter can find equipment, upgrade with shards, choose evolution
- Shopkeeper interaction completes purchase server-side, gold deducts, item appears
- Power pickup grants ability that survives respawn (ASC on PS)

---

## Phase 7: PvE ecosystem
**Goal:** Creeps, bosses, vaults, basecamps, contracts.

- Creep camps (BehaviorTree AI: idle → aggro → attack → leash)
- Bosses (extended creep with unique drops, contestable)
- Vaults (channeled hack mini-game)
- Basecamps (capture, repair armor, brew vives)
- Contracts (Brawler / Creep Farm) issued by merchant after first camp

**Exit criteria:**
- Creep camps populate and respawn
- A boss can be killed and drops contested loot
- A vault can be hacked and yields equipment
- A basecamp can be captured and repairs armor

---

## Phase 8: Mantling
**Goal:** Binary climbable/non-climbable ledges with replication.

- `UMantleComponent` on HunterCharacter
- Forward + upward trace; check Climbable collision tag; check headroom
- Root motion mantle animation, ~0.5s, hunter not actionable during
- Custom CMC movement mode `MOVE_Mantling` for replication

**Note:** UE 5.7 has improved built-in character movement features. Evaluate whether engine-provided mantle solutions cover this need before rolling custom. Don't reinvent if engine ships it.

**Exit criteria:**
- Hunter mantles up tagged ledges, blocked by non-tagged
- Mantle replicates correctly to other clients
- White/red trim materials show correct status (visual only — system reads tags)

---

## Phase 9: Additional hunters (one per role)
**Goal:** Test team comps with 5 hunters total.

- Ghost (Fighter) — already done (Phase 2)
- Kingpin (Initiator) — hook pull tests displacement networking
- Elluna (Protector) — heals, ult revive, **wisp carry** tests carry system
- Felix (Frontliner) — flamethrower tests continuous damage replication
- Bishop (Controller) — displacement tests positional manipulation

Each new hunter is mostly data-driven once GAS architecture is solid.

**Exit criteria:**
- All 5 hunters playable
- Wisp carry verified end-to-end (Elluna picks up downed ally, repositions, revives)
- Each role's signature mechanic replicates without exploits

---

## Phase 10: Polish, vision/sneaking, day/night, UI
Most variable in scope. Includes vision cones, brush stealth, day/night level cap, full UI, audio polish.

Set time-boxed exit criteria when you reach this phase based on remaining ambition and energy.
