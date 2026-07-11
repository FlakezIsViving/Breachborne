# Breachborne — PIE Test Checklist

**Setup for every session:**

- Launch in PIE with **3 clients + dedicated server** (New Editor Window, Net Mode = Play As Listen Server disabled, use `-NumPlayers=3 -DedicatedServer`)
- Or: Edit → Play → Advanced Settings → Number of Players = 3, Net Mode = Play As Client
- All "server-only" checks require observing the **dedicated server log** (Output Log → filter `LogBreachborne`)
- "Client 1" = owning client; "Client 2/3" = simulated proxies

Use `tick` boxes during a test run. Mark `[FAIL]` with a note if a check doesn't pass.

---

## Phase 0/1 — Match Phase State Machine + Squad Setup

| # | Check | Pass |
|---|-------|------|
| 1.1 | GameMode spawns 3 HunterCharacters at dropzone markers on server. Verify in server log: `"HunterCharacter: ASC bound"` × 3. | ☐ |
| 1.2 | `GetMatchPhase()` == WaitingForPlayers until all clients have loaded. Use `showdebug gamestate` (client 1). | ☐ |
| 1.3 | Phase transitions Waiting → Dropping → Playing in order. Each transition logged on server: `"SetMatchPhase:"`. | ☐ |
| 1.4 | All 3 clients replicate MatchPhase within 1 frame of server change (no stale HUD). | ☐ |
| 1.5 | SquadSize = 4 enforced: a 5th client connect attempt is rejected by GameMode. | ☐ |

---

## Phase 2 — Gameplay Ability System Binding

| # | Check | Pass |
|---|-------|------|
| 2.1 | Server log shows `"ASC bound — Owner=<Name> Avatar=<Name> (Server)"` for each player. | ☐ |
| 2.2 | Client 1 log shows `"ASC bound — Owner=<Name> Avatar=<Name> (Client)"`. | ☐ |
| 2.3 | Kill client 1 PIE window and reconnect. On reconnect, ASC re-binds without duplicate log line. | ☐ |
| 2.4 | Ghost's RMB ability is grantable and activates: press RMB, see daze GE applied to a target in server log (`"Added Daze GE"`). | ☐ |
| 2.5 | Ability cooldown is a GE, not a timer: verify via `showdebug abilitysystem` on client 1 — cooldown bar appears and drains. | ☐ |

---

## Phase 3 — Inventory, Items, and Item Registry

| # | Check | Pass |
|---|-------|------|
| 3.1 | Open console (`~`), type `ServerDebugEquipTestItem Weapon`. Weapon slot populates on HUD. | ☐ |
| 3.2 | Equip a second weapon: old weapon slot replaced, no crash. | ☐ |
| 3.3 | `FBBInventoryManager::AddGold` via console command: gold counter increments on owning client within 1 frame. | ☐ |
| 3.4 | Shards added via debug command: `ServerDebugAddShards 5` → shard count updates on HUD. | ☐ |
| 3.5 | Shopkeeper actor placed in map: interacting purchases an item from the `UBBItemRegistry` (check server log for `"EquipItem: success"`, NOT `"NewObject stub"`). | ☐ |
| 3.6 | `ABBWorldItem` pickup: walk over a placed WorldItem → item equips automatically, actor despawns. | ☐ |

---

## Phase 4 — Wisp Downed State, Deathbox, Revival

| # | Check | Pass |
|---|-------|------|
| 4.1 | Hunter at 0 HP transitions to Wisp state: `AWispPawn` spawns, `HunterCharacter` hides (server log: `"Hunter entered Wisp state"`). | ☐ |
| 4.2 | Wisp HP drains at ~5/s when no ally is nearby. Drain stops when ally moves within 300 cm. | ☐ |
| 4.3 | Ally within 300 cm: revive bar fills; Hunter respawns with partial HP at Wisp location. `AWispPawn` destroyed. | ☐ |
| 4.4 | Enemy within 300 cm presses E: execute animation plays; Wisp dies instantly → `ADeathboxActor` spawns. | ☐ |
| 4.5 | Deathbox visible on all 3 clients. Team mate interacts → full revive from deathbox, deathbox despawns. | ☐ |
| 4.6 | Abyss fall: Hunter teleports instantly to `LastGroundedLocation`. No Wisp state (abyss = instant death). | ☐ |
| 4.7 | `ABBRespawnBeacon` placed in map: Beacon activated (server) → squad members respawn within 200 cm of beacon. | ☐ |
| 4.8 | Most Wanted Crown: team with crown gets `State_MostWanted`. Every 30s server log: `"MostWanted: broadcast location"`. | ☐ |

---

## Phase 5 — Storm Shifts

| # | Check | Pass |
|---|-------|------|
| 5.1 | Match start: server log shows `"StormManager: SetActiveShift → <ShiftName>"`. | ☐ |
| 5.2 | `GetActiveShiftName()` on GameState returns the same shift name on all 3 clients (replicated). | ☐ |
| 5.3 | BulletTrains shift: during storm phase, all hunters receive `Effect_StormShift_BulletTrains` GE → movement speed increases. Verify via `showdebug abilitysystem`. | ☐ |
| 5.4 | Default shift: no speed buff applied during storm phase. | ☐ |
| 5.5 | Re-run PIE 3 times: shift selection is random (observe different shift names at least twice in 3 runs). | ☐ |

---

## Phase 6 — PvP Kill Rewards (Gold Steal)

| # | Check | Pass |
|---|-------|------|
| 6.1 | Client 1 kills Client 2: server log shows `"AwardKillRewards: killer=<Name>, victim=<Name>, stolen=<N>g"`. | ☐ |
| 6.2 | Victim had 100g → stolen = 25g. Killer's gold increases by 25, victim's decreases by 25. | ☐ |
| 6.3 | Victim had 3g → stolen = 1g (minimum steal floor). | ☐ |
| 6.4 | Victim had 0g → stolen = 1g (min steal, victim gold goes to –1 or clamps at 0 — check `AddGold` clamping policy). | ☐ |
| 6.5 | Contract system notified: if killer's team has a Brawler contract, progress increments (server log: `"Contract progress: Brawler <N>/<M>"`). | ☐ |

---

## Phase 7 — PvE Framework

### 7a. Creep Camps

| # | Check | Pass |
|---|-------|------|
| 7a.1 | `ABBCreepCamp` placed in map with `CreepCount=3`. On play: 3 `ABBCreepCharacter` actors spawn around camp center. Server log: `"CreepCamp: spawned 3 creeps"`. | ☐ |
| 7a.2 | Creep AI: creeps walk toward nearest Hunter within `LeashRadius` (1500 cm). They leash back when Hunter moves away. | ☐ |
| 7a.3 | Hunter kills a creep: shard reward (2) and gold reward (5) added to killer's inventory. Server log confirms. | ☐ |
| 7a.4 | All 3 creeps die: `OnCampCleared` fires, server log: `"CreepCamp: camp cleared by team <ID>"`. Camp re-spawns after `RespawnDelay` seconds. | ☐ |
| 7a.5 | Camp clear awards contract progress if the nearby team has a CreepFarm contract. | ☐ |

### 7b. Boss Character

| # | Check | Pass |
|---|-------|------|
| 7b.1 | `ABBBossCharacter` placed: spawns with 2000 HP. Health bar visible on HUD. | ☐ |
| 7b.2 | Boss drops to 50% HP: server log `"Boss: enrage triggered"`. Attack frequency visibly increases. | ☐ |
| 7b.3 | Boss dies: `ABBWorldItem` spawns at death location with `DropItemID` configured in editor. | ☐ |

### 7c. Vault Interactable

| # | Check | Pass |
|---|-------|------|
| 7c.1 | `ABBVaultInteractable` placed: interact triggers hack (`ServerBeginHack`). `HackProgress` bar fills over 5s on HUD. | ☐ |
| 7c.2 | Hunter moves >80 cm during hack: hack cancels, `bHacking` resets to false on all clients. | ☐ |
| 7c.3 | Hunter takes damage during hack: hack cancels immediately. | ☐ |
| 7c.4 | Hack completes: reward spawns, vault enters cooldown (can't re-hack for `RespawnDelay`). | ☐ |

### 7d. Basecamp

| # | Check | Pass |
|---|-------|------|
| 7d.1 | `ABBBasecampActor` placed: single team member inside fills capture at 1x speed. Server log prefix: `BasecampCapture`. | ☐ |
| 7d.2 | Two members from same team: capture rate doubles and caps at 2x. | ☐ |
| 7d.3 | Members from 2 different teams inside: contested state appears and capture progress pauses. | ☐ |
| 7d.4 | Capture completes: `OwningTeamID` replicates, owner visual updates, and owned-team players heal on platform. | ☐ |
| 7d.5 | Capture a second basecamp with the same team: second camp becomes the active recall camp and the prior one clears for that team. | ☐ |
| 7d.6 | Press `B` with no owned camp: recall is rejected. Press `B` with owned camp: recall channel starts and completes at `RecallLandingPoint`. | ☐ |
| 7d.7 | Recall cancels on movement, health damage, death/downed state, or ownership transfer away. Enemy contest alone does not cancel recall. | ☐ |
| 7d.8 | Press `E` in anvil zone: owner can repair damaged armor after channel; non-owner/no armor/full armor/insufficient gold are rejected. | ☐ |
| 7d.9 | Anvil cancellation by movement, damage, or enemy contest removes `State.Vulnerable` and does not spend gold. | ☐ |
| 7d.10 | Armor durability drops when taking damage; armor at 0 durability stops providing armor stat; repair restores it. | ☐ |
| 7d.11 | Place/spawn a `ViveBean` consumable pickup: player can pick it up, brew at cauldron, and receive `ViveBrew`. | ☐ |
| 7d.12 | Cauldron cancellation by movement, damage, or enemy contest removes `State.Vulnerable` and does not consume `ViveBean`. | ☐ |
| 7d.13 | `ViveBrew` can be used from backpack/hotkey and applies health-over-time. | ☐ |
| 7d.14 | Basecamp replicated state, station progress, recall progress, and in-world labels are visible on simulated clients. | ☐ |

### 7e. Contracts

| # | Check | Pass |
|---|-------|------|
| 7e.1 | Match start: each team receives one contract. `IssueContractToTeam` called in server log. | ☐ |
| 7e.2 | Brawler contract (3 kills): after 3 kills server log shows `"Contract completed: Brawler — paying <N>g to team <ID>"`. | ☐ |
| 7e.3 | CreepFarm contract (2 clears): after 2 camp clears, same payout log. | ☐ |
| 7e.4 | `UBBContractSubsystem::ShouldCreateSubsystem` returns false on clients: no subsystem exists on client-only contexts (verify in log — no contract subsystem init on clients). | ☐ |

---

## Phase 8 — Mantle Component

| # | Check | Pass |
|---|-------|------|
| 8.1 | Mark floating-island terrain with `MantleRecoverable` tag/profile or `BBMantleSurfaceComponent`. Fall below the island edge and contact the side/underside: character recovers to the top surface. | ☐ |
| 8.2 | Mantle recovery arc is smooth and diagonal up/inward (SmoothStep interpolation — no teleport, no vertical wall grind). | ☐ |
| 8.3 | Server log: `"mantle-start: landing=<Loc>"` when mantle starts. | ☐ |
| 8.4 | `State_Mantling` loose tag applied during mantle; removed on finish. Verify via `showdebug abilitysystem`. | ☐ |
| 8.5 | Contact on non-opt-in props/buildings/trains: no mantle. | ☐ |
| 8.6 | Contact too shallow below the top surface: no mantle. | ☐ |
| 8.7 | No headroom or blocked path to landing: validation fails, no mantle. | ☐ |
| 8.8 | Client 2 observes client 1 mantling correctly (movement replication). | ☐ |

---

## Phase 9 — Kingpin Hunter Abilities

Grant all Kingpin abilities via console: `GiveAbility /Game/.../GA_Kingpin_LMB` etc., or configure in the Hunter Blueprint.

### 9a. LMB — Chain Strike

| # | Check | Pass |
|---|-------|------|
| 9a.1 | Hold LMB: strikes fire at 0.45s intervals while held. Release: strikes stop immediately. | ☐ |
| 9a.2 | Enemy within 250 cm and front hemisphere: takes full `AttackDamage`. Server log shows damage GE applied. | ☐ |
| 9a.3 | Second enemy within 400 cm of first: takes ~0.6× damage (chain bounce). | ☐ |
| 9a.4 | No target in range: ability fires but deals no damage (no crash). | ☐ |

### 9b. RMB — Iron Hook

| # | Check | Pass |
|---|-------|------|
| 9b.1 | Click RMB on an enemy within 2500 cm: target receives `State_Hooked` GE + is pulled toward Kingpin. | ☐ |
| 9b.2 | Target takes 60 damage on hook hit. | ☐ |
| 9b.3 | `State_Hooked` expires after 0.8s. | ☐ |
| 9b.4 | Hook fires passive event: server log `"Kingpin Passive: CC applied, scanning cooldowns"`. | ☐ |
| 9b.5 | 12s cooldown visible in `showdebug abilitysystem`. | ☐ |

### 9c. Q — Ground Slam

| # | Check | Pass |
|---|-------|------|
| 9c.1 | Press Q: all enemies within 300 cm and 60° half-angle receive `State_Stunned` (1.5s). | ☐ |
| 9c.2 | Stunned enemies cannot activate abilities (blocked by `State_Stunned` tag on `ActivationBlockedTags`). | ☐ |
| 9c.3 | Slam deals 80 damage to each hit target. | ☐ |
| 9c.4 | Target outside the 60° cone but within 300 cm: NOT stunned. | ☐ |

### 9d. Shift — Charge

| # | Check | Pass |
|---|-------|------|
| 9d.1 | Press Shift: Kingpin charges forward for 0.6s at 1600 cm/s. | ☐ |
| 9d.2 | First enemy hit during charge: knocked back 900 units and takes 55 damage. | ☐ |
| 9d.3 | Same enemy not hit again during same charge (HitActors set prevents double-hit). | ☐ |
| 9d.4 | `State_Charging` tag applied during charge, removed on finish. | ☐ |

### 9e. R — Titan's Stomp (ServerInitiated)

| # | Check | Pass |
|---|-------|------|
| 9e.1 | Press R: Kingpin leaps upward (visual pause). After `AirDuration` (0.6s), lands and hits allw enemies in 600 cm radius. | ☐ |
| 9e.2 | Enemies in radius knocked up (`LaunchCharacter` with 1200 Z-force) and receive `Effect_Knockup` HasDuration GE. | ☐ |
| 9e.3 | Enemies take 120 damage. | ☐ |
| 9e.4 | 45s cooldown visible in `showdebug abilitysystem`. | ☐ |

### 9f. Passive — Chain Commander

| # | Check | Pass |
|---|-------|------|
| 9f.1 | After Q lands on an enemy: passive fires, all active cooldown GEs have their remaining time reduced by 1.5s. | ☐ |
| 9f.2 | Verify visually: cooldown bar for RMB (12s) shrinks by 1.5s when Q hits. | ☐ |
| 9f.3 | Passive fires on RMB hook land as well. | ☐ |
| 9f.4 | If a cooldown would expire from the reduction, the GE is removed (ability is immediately ready). | ☐ |

---

## Phase 10 — Vision, Brush, Day/Night

### 10a. Vision Cone Component

| # | Check | Pass |
|---|-------|------|
| 10a.1 | Blueprint HUD draws a 120°-wide cone extending 2000 cm from each hunter (call `GetEffectiveRange()` / `GetHalfAngleDegrees()`). | ☐ |
| 10a.2 | `IsInVisionCone()` returns false for a target 90° to the side (verify via server log or Blueprint print). | ☐ |
| 10a.3 | `IsInVisionCone()` returns true for a target 50° off-center within range. | ☐ |
| 10a.4 | `GetEffectiveRange()` returns `DayVisionRange` (2000) during day and `NightVisionRange` (1200) during night. | ☐ |

### 10b. Brush Volume (Stealth)

| # | Check | Pass |
|---|-------|------|
| 10b.1 | Place `ABBBrushVolume` in map. Hunter walks inside: server log `"BrushVolume: <Name> entered brush"`. | ☐ |
| 10b.2 | `State_InBrush` loose tag applied while inside, removed on exit. Verify via `showdebug abilitysystem`. | ☐ |
| 10b.3 | Hunter inside brush: `GetEffectiveRange()` returns `DayVisionRange × 0.6` = 1200 cm (outward cone reduced). | ☐ |
| 10b.4 | Enemy vision cone does NOT see the brush-hidden hunter beyond the reduced range. | ☐ |
| 10b.5 | Hunter walks out of volume: `State_InBrush` removed, vision range restores. | ☐ |

### 10c. Day/Night Cycle

| # | Check | Pass |
|---|-------|------|
| 10c.1 | Match starts in Day. After `DayPhaseDuration` (300s, or reduce for testing via `DayPhaseDuration = 10` in BP): transitions to Night. Server log: `"DayNight: Day -> Night (Cycle 1)"`. | ☐ |
| 10c.2 | Night lasts `NightPhaseDuration` (180s / test value), then transitions back to Day. | ☐ |
| 10c.3 | `GetDayNightCycle()` increments correctly each transition. | ☐ |
| 10c.4 | `OnRep_DayNightPhase` fires on all clients: HUD updates to show day/night indicator without extra server round-trip. | ☐ |
| 10c.5 | During Night, `GetEffectiveRange()` returns 1200 not 2000 (vision cone component reads GameState). | ☐ |
| 10c.6 | Match ends (phase = Ended): day/night timer stops (no more log entries after end). | ☐ |

---

## Cross-Phase Regression Checks

Run these after every feature that touches networking, regardless of which phase changed.

| # | Check | Pass |
|---|-------|------|
| R.1 | **No UMG in server code paths**: dedicated server log has zero `"UserWidget"` or `"CreateWidget"` warnings. | ☐ |
| R.2 | **No GetGameMode() on client**: client log has zero `"GetGameMode: returned nullptr"` assert hits. | ☐ |
| R.3 | **No GetFirstLocalPlayerController() on server**: server log has zero such calls. | ☐ |
| R.4 | **Authority guards hold**: in a 3-client session, no gameplay state changes (gold, health, phase) originate from a non-authoritative path. Verify via server-only log lines for every state mutation. | ☐ |
| R.5 | **DoRepLifetime coverage**: add a property, ship without DOREPLIFETIME, and observe it fails to replicate. Fix it. (Meta-check: run this on any new replicated property.) | ☐ |
| R.6 | **No raw ApplyDamage calls**: grep the source tree — zero results for `UGameplayStatics::ApplyDamage` or `ApplyPointDamage`. | ☐ |
| R.7 | **ASC on PlayerState, not Character**: grep — no `CreateDefaultSubobject<UAbilitySystemComponent>` inside any Character class. | ☐ |

---

## Automated Test Suite (Run Before PIE Session)

From the Session Frontend (Editor) or command line, run:

```
Automation RunTests Breachborne.PureLogic
Automation RunTests Breachborne.GameSystems
```

All tests in `Breachborne.PureLogic` must pass before opening PIE — they run in < 1s and catch formula/enum regressions.

`Breachborne.GameSystems` tests require the editor and take ~5s. Run them before each PR merge.

---

## Logging Reference

Key log categories and what they confirm:

| Log line fragment | Confirms |
|---|---|
| `HunterCharacter: ASC bound` | GAS binding (Phases 1–2) |
| `BrushVolume: <Name> entered brush` | Brush overlap working |
| `DayNight: Day -> Night` | Day/night timer firing |
| `StormManager: SetActiveShift` | Shift selection (Phase 5) |
| `AwardKillRewards: stolen=` | Gold steal formula (Phase 6) |
| `CreepCamp: camp cleared by team` | Camp cleared signal (Phase 7a) |
| `Contract progress:` | Contract tracking (Phase 7e) |
| `mantle-start: landing=` | Mantle trace passed (Phase 8) |
| `Kingpin Passive: CC applied` | Passive listener working (Phase 9f) |
