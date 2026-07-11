# Train Fix History — DO NOT REPEAT THESE

> This file exists so the AI doesn't suggest the same failed fixes after context compaction.

## Failed Approaches

### 1. Attachment (`AttachToActor`)
- **Result**: Catastrophic. CMC fought attachment transform, flung character miles away.
- **Status**: Fully reverted. Never try again.

### 2. CMC Base Movement (`UpdateBasedMovement`)
- **Result**: Jitter land-miss-land-miss cycle. `FindFloor` runs BEFORE `UpdateBasedMovement`. Carriage moves in PrePhysics, character is left behind, `FindFloor` misses, character falls, next frame `FindFloor` hits, `UpdateBasedMovement` snaps them forward.
- **Status**: Abandoned. The CMC's base-following is fundamentally incompatible with spline-teleport movement.

### 3. `SetBase(nullptr)` + Pre-Positioning
- **Result**: Worse push-pull. Character visually in void.
- **Root cause**: `SideCollision` box was mis-sized (inherited mesh scale → 64m wide, 9cm tall, at wrong height). `FindFloor` never hit it. Character fell every frame. Captured fallen offset drifted downward perpetually.
- **Status**: Abandoned until collision is fixed.

## Current Understanding

### Why `FindFloor` Misses
- `SideCollision` is child of `CarriageMesh` which has scale (4, 8, 0.3) or (8, 8, 0.3).
- `SideCollision` inherits this scale. `SetBoxExtent(200,400,15)` × scale(4,8,0.3) = world extent (800, 3200, 4.5).
- The box is 64m wide, 9cm tall, centered at Z=0 (bottom of platform).
- Character capsule center is at ~103cm. `FindFloor` sweeps down only 47cm → never reaches the box.
- Even if it did, initial overlap handling is unreliable for thin boxes.

### Why Train Jitters
- Unknown — possibly spline evaluation precision, `FMath::Fmod` in `AdvanceProgress`, or camera lag exaggeration.
- `ClientTrackProgress` interpolation (speed 5.0) may contribute on client.

## Fixes That Worked

### Collision Scale Fix (2026-05-24)
- **Problem**: `SideCollision` inherited mesh scale `(4,8,0.3)`, becoming 64m wide × 9cm tall at Z=0. `FindFloor` sweep (down ~47cm from capsule center at ~103cm) never reached it.
- **Fix**: `SetAbsolute(false,false,true)` + tall box (platform top to 115cm) with correct footprint.
- **Result**: ✅ Character no longer flung off train.

## Fixes Attempted for Train Jitter

### Disable Movement Replication (2026-05-24)
- **Theory**: Carriages had `bReplicateMovement=true`. Server set position, client also set position via `TrackProgress`. Potential server-client transform fighting caused jitter.
- **Fix**: `Carriage->SetReplicateMovement(false)` on spawn.
- **Result**: ❌ Still jittery.

### Remove Client Interpolation (2026-05-24)
- **Theory**: `ClientTrackProgress = FMath::FInterpTo(...)` caused visual lag/jitter on client.
- **Fix**: Client uses replicated `TrackProgress` directly.
- **Result**: ❌ Still jittery.

### `TeleportPhysics` on carriage move (2026-05-24)
- **Theory**: `SetActorLocationAndRotation` without teleport causes physics-scene broadphase updates every frame, creating hitches.
- **Fix**: `SetActorLocationAndRotation(..., false, nullptr, ETeleportType::TeleportPhysics)`.
- **Result**: Pending test.

## Root Cause: Variable DeltaTime (2026-05-24)
- **Per-frame delta logging** revealed train deltas swinging from **8 to 15 cm/frame** (50% variation).
- At 800 u/s, that means frame time oscillates between **~10ms and ~18ms**.
- The human eye is extremely sensitive to rigid-body motion with this kind of speed fluctuation.
- **NOT** server-client fighting, camera lag, or physics — purely frame pacing causing variable `DeltaTime`.

## Fix: Smoothed DeltaTime (2026-05-24)
- **Approach**: Low-pass filter on `DeltaTime` for train movement.
- `SmoothedDeltaTime = Lerp(SmoothedDeltaTime, Clamp(DeltaTime, 8.3ms, 33.3ms), 0.15)`.
- **Result**: ❌ No visible improvement. User reports no difference after 8 hours of observation.
- **Conclusion**: Train jitter is NOT caused by variable `DeltaTime` / frame pacing.

## Fix: Per-Frame Diagnostic Logging (2026-05-24)
- **Added**: `bTestFixed60HzDelta` — hardcodes `DeltaTime = 1/60s` to completely eliminate frame pacing.
- **Added**: `bTestFreezeProgress` — stops train progress advance but still evaluates spline each frame (tests if spline itself jitters).
- **Added**: Per-frame logging in `UpdatePosition` showing `CarriageProgress`, `SplineLocation`, `TargetLocation`, `ActualLocation`, and `Diff`.
- **Result**: `diff = 0` always, `spline = target = actual` always. Nothing moves the carriage after we set it. Spline evaluation is perfectly consistent.

## Fix: Multi-Carriage Rider Death Spiral (2026-05-24)
- **Root cause**: Detection spheres on adjacent carriages overlap (700cm apart carriages, 550cm radius spheres). Characters were detected by multiple carriages simultaneously.
- **Fix 1**: `PrePositionRiders` now uses `TeleportPhysics` and zeros `CMC->Velocity`.
- **Fix 2**: `BBTrain::UpdateRiders()` does global conflict resolution. Each character assigned to exactly ONE carriage.
- **Result**: Rider death spiral fixed.

## Root Cause: Variable Displacement (2026-05-24)
- **TrainJitter log analysis**: Per-frame Euclidean delta oscillates by ~7% (10.89 → 10.34 → 10.82 → 10.12 cm/frame).
- **Source 1**: `EffectiveDelta` still varies ~6% even with smoothing (factor 0.15 too weak).
- **Source 2**: Spline curvature causes chord length to vary for equal arc-length steps.
- **Combined**: Creates visible speed fluctuation on a large rigid body.

## Fix: Fixed Timestep with Visual Interpolation (2026-05-24)
- **Approach**: Separate logic update from visual update.
  - `TrackProgress` advances at exactly 60Hz (fixed timestep accumulator).
  - Carriages rendered at interpolated visual position.
- **Bug found**: When `TimeAccumulator == 0` after a step, `Alpha = 0`, so `VisualProgress = Lerp(Previous, Current, 0) = Previous`. The train rendered **one full step behind** its logic position every frame. At ~60fps this created a persistent lag + jitter as frame rate wobbled.
- **Fix**: When `TimeAccumulator == 0`, render at `TrackProgress` directly. Only interpolate when `TimeAccumulator > 0` (partway through next step).
- **Toggle**: `bUseFixedTimeStep` (default `true`). Disable to test variable delta modes.
- **Result**: ✅ Carriage-level jitter FIXED. Train is smooth when viewed from the ground.

## Root Cause: Interpolation Sawtooth (2026-05-24)
- **Observation**: Train jitters even with no player on it. `diff=0` always (carriage placed exactly at spline), but per-frame `delta` varies wildly (2.4 → 5.6 → -0.4 → 18.5 → -12.8 cm/frame).
- **Root cause**: The `Lerp(PreviousTrackProgress, TrackProgress, Alpha)` interpolation was **backward**.
  - `PreviousTrackProgress` was set **before** each step → it holds the position **before** the most recent step.
  - `TrackProgress` holds the position **after** the most recent step.
  - `Alpha = TimeAccumulator / FixedTimeStep` is the fraction into the **next** (not yet taken) step.
  - So `Lerp(Previous, Current, Alpha)` goes from "one step behind" to "current step" as Alpha goes 0→1.
  - When a new step happens, it snaps back to "one step behind" and starts catching up again.
  - This creates a **sawtooth oscillation** where the visual constantly lags by one step, then catches up, then lags again.
  - On a curved spline, this manifests as highly irregular per-frame displacements (the jitter).

## Fix: Continuous Visual Extrapolation (2026-05-24)
- **Approach**: Replace backward interpolation with forward extrapolation.
  - `VisualProgress = AdvanceProgress(TrackProgress, Speed * TimeAccumulator, bMovingForward)`
  - `TrackProgress` = position after last logic step.
  - `Speed * TimeAccumulator` = exact distance traveled since that step.
  - `AdvanceProgress` handles loops, clamps, and spline curvature correctly.
- **Why it works**:
  - Visual position is a **continuous function of time** — no jumps, no lag, no sawtooth.
  - Per-frame displacement is exactly proportional to `DeltaTime`.
  - At step boundary: just before step N, visual ≈ position N. After step N, `TrackProgress` becomes position N, and `TimeAccumulator` resets to near 0, so visual ≈ position N. Seamless!
- **Removed**: `PreviousTrackProgress` variable (no longer needed).

## Fix: Frame-Rate-Independent Visual Smoothing (2026-05-24)
- **Problem**: Even with perfectly correct extrapolation, the train jitters at 30/60/120 FPS but is buttery smooth at 500 FPS. Root cause: the **absolute per-frame displacement** is what the eye detects.
  - At 500 FPS: displacement = 1.6 cm/frame. Even ±50% variation = ±0.8 cm — imperceptible.
  - At 60 FPS: displacement = 13.3 cm/frame. Even ±10% variation = ±1.3 cm — very noticeable.
  - At 30 FPS: displacement = 26.7 cm/frame. ±5% variation = ±1.3 cm — noticeable.
  - The eye detects jitter when absolute displacement variation exceeds ~1–2 cm, regardless of percentage.
- **Approach**: Low-pass filter on `VisualProgress` using `FMath::FInterpTo` with constant time constant.
  - `VisualSmoothingSpeed = 125` → time constant `tau = 1/125 = 8 ms`.
  - Lag distance = `Speed * tau = 800 * 0.008 = 6.4 cm` — imperceptible on a train.
  - The lag is **constant in time (8 ms) and distance (6.4 cm)** at ALL frame rates.
  - Handles loop wrap-around correctly.
- **Console command**: `Train.SmoothingSpeed [value]` — 0 = off, 125 = default.
- **Result**: ✅ Train glides smoothly at 30/60/120/144/500 FPS. The ~6.4 cm lag is completely imperceptible.

## Remaining Issue: On-Train Jitter (2026-05-24)
- **Observation**: Train is smooth from ground. Jitter only visible when character is ON the train.
- **Theory**: CMC `FindFloor` micro-adjusts character Z each frame as the carriage rotates. Camera is attached to character, so micro-jitters are amplified.
- **Test added**: `bTestFlyingModeForRiders` — puts riders in `MOVE_Flying` (no FindFloor, no gravity). If this eliminates on-train jitter, CMC is confirmed as the cause.
- **Result**: Pending test.


## Fix: CarriageLength Replication + Client Collision Mismatch (2026-05-24)
- **Problem**: `CarriageLength` was `EditDefaultsOnly` (NOT replicated). Server set it to `800` for locomotive, but client kept default `400`.
  - Client locomotive `SideCollision` was only 400cm long (front/back 200cm missing).
  - Client `FrontKillZone` was at `X=200` instead of `X=400`.
  - Client-server desync caused players to "phase through" the locomotive on the client, then get corrected by server.
- **Fix**: 
  - Made `CarriageLength` `Replicated`.
  - Added `DOREPLIFETIME(ABBTrainCarriage, CarriageLength)`.
  - `OnRep_CarriageType()` now recomputes `CarriageLength = GetCarriageLength(CarriageType)` before calling `SetupCollision()`.

## Fix: Rider Movement Lock (2026-05-24)
- **Problem**: `PrePositionRiders()` hard-teleported every rider every frame via `SetActorLocation(TeleportPhysics)` AND zeroed `CMC->Velocity`. This reset the CMC's movement state each frame, preventing the player from walking on the train.
- **Fix**:
  - Only teleport if position error > 1cm (`TELEPORT_THRESHOLD_SQ = 1.0f`).
  - Removed `CMC->Velocity = FVector::ZeroVector` — let CMC preserve input-driven momentum.
  - Kept `CMC->SetBase(nullptr)` to prevent `UpdateBasedMovement` double-movement.
- **Result**: Players can now walk freely on platforms while still being synced to train movement.

## Fix: Rapid Board/Leave Cycle (2026-05-24)
- **Problem**: `MinHeight = 0.0f` was too tight. CMC hovers at `Z≈0.07`, then dips to `Z≈-0.30` due to floating point / floor detection micro-adjustments. This triggered one-frame BOARDED → LEFT cycles.
- **Fix**: `MinHeight = -5.0f`. Allows small floating-point dips while still rejecting players who fell through locomotive (`Z≈-68`).

## Fix: Sweep-Based Kill Detection (2026-05-24)
- **Problem**: `OnComponentBeginOverlap` on `FrontKillZone` is unreliable for teleported volumes (physics engine may miss "begin overlap" transitions when object jumps into overlap in one frame).
- **Fix**: Added `CheckFrontKillZoneSweep()` called every `Tick()` on server for locomotive. Uses `OverlapMultiByObjectType` with the kill zone box shape. Checks dot product (must be in front) and skips existing riders.
- **Logging**: Added `TrainKillSweep:` log to confirm kills are happening.


## Fix: Kill Zone Position Wrong Due to Mesh Scale Inheritance (2026-05-24)
- **Root cause**: `FrontKillZone` is a child of `CarriageMesh`, which has non-uniform scale `(8, 8, 0.3)` for locomotive. Unreal multiplies child **relative location** by parent scale:
  - `RelativeLocation.X = 400` (HalfLength)
  - World offset = `400 * 8 = 3200` cm ahead of locomotive center
  - Kill zone was **3200cm** in front of train, not 400cm!
  - Box extent `(50, 400, 50)` became world extent `(400, 3200, 15)` — giant flat plane far ahead
- **Why player phased through**: Kill zone was nowhere near the player. Locomotive `SideCollision` was overlap, so player passed through.
- **Fix**: In `SetupCollision()`, divide `RelativeLocation` and `SetBoxExtent()` values by `CarriageMesh->GetRelativeScale3D()` to counteract parent scale:
  ```cpp
  FrontKillZone->SetRelativeLocation(FVector(HalfLength / SafeScale.X, 0.0f, 0.0f));
  FrontKillZone->SetBoxExtent(FVector(50.0f / SafeScale.X, HalfWidth / SafeScale.Y, 50.0f / SafeScale.Z));
  ```
- **Same fix applied to `SideCollision`** to ensure consistent world-space sizing regardless of mesh scale.


## Fix: Kill Zone Depth Configurable + Positioned at Mesh Edge (2026-05-24)
- **Problem**: Kill zone was centered at `HalfLength` with 50cm depth, covering 350-450. If visual mesh was shorter than `CarriageLength`, kills happened before visual contact.
- **Fix**: Added `KillZoneDepth` property (default 25cm). Kill zone back face is now at `HalfLength` (locomotive front edge), extending forward by `KillZoneDepth`.
  ```cpp
  FrontKillZone center = HalfLength + KillZoneDepth/2
  FrontKillZone extent X = KillZoneDepth/2
  ```
- **Tuning**: User can adjust `KillZoneDepth` in Blueprint defaults to match their custom mesh.

## Fix: Locomotive Side Overlap Diagnostic (2026-05-24)
- **Problem**: Player getting "stuck" on far end of locomotive — need to know what's blocking them.
- **Fix**: Added `OnLocomotiveSideOverlap` delegate bound to `SideCollision->OnComponentBeginOverlap`. Logs player name, local position, world position, and carriage position when overlapping locomotive side collision.


## Fix: KillZoneDepth Default 10cm + Collision Diagnostics (2026-05-24)
- **KillZoneDepth**: Reduced default from 25cm to **10cm** so kills happen much closer to visual mesh contact. User can tune in Blueprint defaults under `Breachborne|Train` category.
- **Collision diagnostics added**:
  - `TrainCarriage:` setup log now shows `meshEnabled`, `meshPawn`, `sidePawn` collision states
  - `OnCarriageMeshHit` delegate bound to `CarriageMesh->OnComponentHit` — logs as **Error** if any player ever hits the mesh (should NEVER happen)
  - `TrainLocoOverlap:` logs when players overlap locomotive `SideCollision`


## Fix: Explicitly Disable CarriageMesh Collision After SetStaticMesh (2026-05-24)
- **Discovery**: `SetStaticMesh()` applies the mesh asset's default collision profile, which overrides our `ECR_Ignore` setting. This caused the locomotive mesh to secretly block player movement even though `SideCollision` was overlap-only.
- **Fix**: Added explicit collision disable in two places:
  1. `InitializeCarriage()` — immediately after `SetStaticMesh()`
  2. `SetupCollision()` — every time collision is reconfigured
  ```cpp
  CarriageMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  CarriageMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
  ```
- **Diagnostic**: `TrainCarriage:` setup log now shows `meshProfile` (collision profile name) to confirm the mesh isn't using a blocking profile like `BlockAllDynamic`.


## Fix: Locomotive is Solid Impassable Terrain (2026-05-24)
- **Misunderstanding**: Previously set locomotive `SideCollision` to `ECR_Overlap` so players would "phase through" and get killed by front kill zone. User actually wants locomotive to be a **solid wall** — impassable terrain you bump into.
- **Fix**: Changed locomotive `SideCollision` from `ECR_Overlap` to `ECR_Block` (same as platforms).
  - Players can stand on top of locomotive (if they somehow get on it)
  - Players cannot walk through it from any direction
  - Front kill zone still works: players blocked at front edge have capsule overlapping kill zone, sweep detects and kills them
- **Result**: Locomotive behaves like a solid moving wall. Front contact = death. Side contact = blocked.


## Fix: SideCollision Extent Calculation (2026-05-24)
- **Bug**: `SideCollision` has `SetAbsolute(false, false, true)` which makes its world scale = `(1,1,1)` (absolute, not inherited from parent). But the code was dividing box extent by mesh scale:
  ```cpp
  SetBoxExtent(FVector(HalfLength/8, HalfWidth/8, 50/0.3))  // WRONG
  ```
  This made the locomotive's `SideCollision` only **1.25cm deep in X** and **166cm tall in Z** — a super thin vertical sliver that players easily passed through.
- **Fix**: Since `bAbsoluteScale=true`, world extent = local extent. No division needed:
  ```cpp
  SetBoxExtent(FVector(HalfLength, HalfWidth, 50.0f))  // CORRECT
  ```
  Only the relative location needs scale division (location is still inherited from parent).
- **Result**: Locomotive `SideCollision` is now correctly 800cm × 800cm × 100cm in world space.


## Fix: Locomotive Tall Wall + Inside-Kill Safety Net (2026-05-24)
- **Problem**: Dash/jump abilities could still get players onto the locomotive by clearing the ~100cm tall `SideCollision`.
- **Fix 1: Tall wall**: Locomotive `SideCollision` Z extent is now `LocomotiveWallHeight` (default 2000cm) — an invisible skyscraper wall no jump/dash can clear.
- **Fix 2: Inside-kill safety net**: `CheckInsideLocomotiveKill()` runs every frame on server. If a non-rider player's capsule center is inside the locomotive bounds (X/Y within 80%, Z between 10-500cm), they die immediately. This catches teleport-based abilities that bypass collision.
- **Property**: `LocomotiveWallHeight` under `Breachborne|Train` — adjustable if needed.


## Fix: Locomotive Unwalkable Slope (2026-05-24)
- **Problem**: Correctly-sized blocking `SideCollision` was detected by CMC `FindFloor` as a valid floor, allowing players to stand and walk on the locomotive like a platform.
- **Fix**: Set locomotive `SideCollision` to `WalkableSlope_Unwalkable`:
  ```cpp
  SideCollision->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.0f));
  ```
  - Still **blocks horizontal movement** (impassable wall)
  - CMC **rejects it as a floor** (can't stand on it, slides off)
  - Platforms keep default walkable slope so riders can still stand on them
- **Result**: Locomotive is now a true wall — you bump into it but can't stand on it.


## Cleanup: Streamlined Logging (2026-05-24)
- **Removed**: `TrainJitter`, `TrainKillDiag`, `TrainLocoOverlap`, `TrainKillSweep candidate`, per-frame kill zone position logs, redundant `HandleTrainDeath called` logs.
- **Kept**:
  - `TrainCarriage: %s setup | type=%d | len=%.0f | sideExtent=%s | wallHeight=%.0f | walkable=%s` — once on spawn, confirms collision sizing
  - `TrainKill: %s killed by locomotive (front contact|inside bounds, dot=%.2f)` — when someone actually dies
  - `TrainInside: %s near locomotive (local=%s) — not killed` — debug edge cases
  - `TrainRider: %s BOARDED/LEFT %s` — rider tracking
  - `TrainMeshHit` — Error-level safety net if mesh somehow blocks


## Fix: Eluna Q — Toss Travel + Attach-to-Ally (2026-05-24)
- **Problem**: Moonlight Blessing toss was an instant teleport with no travel time, and could not attach to allies.
- **Fix**: Rewrote `BBMoonlightZone` travel logic:
  - `TossToLocation()` now sets `bIsTraveling = true` instead of instantly warping
  - `Tick()` (server-only) moves the zone toward the target at `TossSpeed` (default 6000 cm/s)
  - `TryAttachToAlly()` performs a pawn-channel overlap sweep (`AttachSearchRadius` default 150cm) during travel
  - If a same-team `AHunterCharacter` is hit, `AttachedAlly` is set and the zone follows them each tick
  - If no ally is hit, the zone lands at the target location
  - Double-toss prevention: `TossToLocation()` early-returns if `bTossed` is already true
  - `GA_Eluna_Q::InputPressed` skips `TossZone()` if `Zone->WasTossed()`
  - Removed instant `Zone->SetActorLocation(TargetLocation)` from `TossZone()` — travel is now visual and authoritative on the server
- **Result**: Q now matches Supervive mechanics: toss travels like a projectile, can attach to allies, and boosts healing by 50%.


## Feature: Debug SpawnTestWisp Console Command (2026-05-24)
- **Problem**: No same-team ally wisp exists in PIE for testing Eluna R revive / wisp-carry mechanics.
- **Fix**: Added `SpawnTestWisp` exec command to `BreachbornePlayerController`:
  - Client exec calls `ServerSpawnTestWisp()`
  - Server spawns an `ABBWispPawn` ~150cm in front of the invoking hunter with slight lateral jitter
  - Calls `Wisp->InitWisp(BBPS)` so the wisp inherits the player's team, gets its health set, and starts ticking
  - Logs spawn location, wisp name, player name, and team ID
- **Usage**: In PIE, open console and type `SpawnTestWisp`
- **Result**: Quick same-team wisp spawning for revive/Q-heal testing without needing a second human player.


## Fix: Eluna Q Toss Speed + Horizontal Travel (2026-05-24)
- **Problem**: Q zone flew "off the screen at mach 10" on redirect — `TossSpeed` was 6000 cm/s (way too fast for visible travel).
- **Fix**: 
  - Reduced `TossSpeed` from 6000 → **1500 cm/s** (~1.3s for max range)
  - `UpdateTravel()` now moves **horizontally only** — Z is set to target ground level, preventing any climb/dive
  - Added `LANDED` debug log when zone reaches target
- **Result**: Toss travel is now visible and predictable.


## Feature: BBTestAlly Placeable Actor (2026-05-24)
- **Purpose**: Placeable test ally for validating Eluna Q heal and wisp interactions (pickup + rez).
- **Class**: `ABBTestAlly` (extends `AActor`, implements `IAbilitySystemInterface`)
- **Properties** (all editable in-editor):
  - `TeamID` (default 0) — set to match the player you're testing with
  - `MaxHealth` (default 100)
  - `HealthDecayRate` (default 20 HP/s) — rapid drain so Q heal is meaningful
  - `LifeSpan` (default 5s) — auto-death timer
  - `bKillNow` — instant-kill debug toggle
- **Behavior**:
  - Spawns with full health
  - Loses `HealthDecayRate` HP per second
  - Dies after `LifeSpan` seconds (or when HP reaches 0, or when `bKillNow` is set)
  - On death: finds nearest player with a valid `PlayerState`, spawns a `BBWispPawn` owned by that player, then destroys self
- **Q Heal Support**: `BBMoonlightZone::TickHeal()` and `ApplyBurstHeal()` now also query for `BBTestAlly` and apply heals via its local `UBBAbilitySystemComponent`
- **Usage**: Place `BBTestAlly` in the level near the player. Set `TeamID` to the player's team (first PIE player = Team 0). It will auto-die in 5s and become a wisp for pickup/rez testing. Heal it with Q before it dies to test healing.


## Feature: BBTestAlly v2 — Hide-on-Death + Auto-Revive + Debug Commands (2026-05-29)
- **Redesign**: `BBTestAlly` no longer destroys itself on death. Instead it hides, disables collision, and spawns a wisp. This allows the actor to be **revived** later (either automatically when the wisp rez bar fills, or manually via console command).
- **Auto-revive**: `BBTestAlly` binds to its spawned wisp's `OnWispReviveReady` delegate. When the wisp's rez bar reaches 100% (player stands near or carries it), the ally automatically revives: wisp is destroyed, actor is unhidden, health resets.
- **Auto-death on wisp death**: If the wisp dies (executed or HP depleted), the ally reference is cleared. The ally stays hidden/dead until manual revival.
- **New console commands** (on `BreachbornePlayerController`, all target the **nearest** `BBTestAlly`):
  - `DebugTestAllyDecay 0` — disable health decay (keeps ally alive for ability testing)
  - `DebugTestAllyDecay 1` — re-enable health decay
  - `DebugTestAllyKill` — instantly kill the ally (forces wisp spawn)
  - `DebugTestAllyRevive` — revive a dead ally (destroys wisp, restores health, unhides actor)
  - `DebugSpawnTestAlly` — spawn a new `BBTestAlly` ~150cm in front of the player
- **Wisp revive answer**: Yes, wisp revive is fully implemented. `BBWispPawn::ServerWispTick()` fills the rez bar when an alive ally is within `ProximityRadius`. When the bar hits 100%, `OnWispReviveReady` broadcasts. `BreachborneGameMode::HandleWispRevive()` teleports the player's cached hunter pawn to the wisp, re-possesses, removes wisp tags, and sets revive HP. For `BBTestAlly`, the same delegate triggers `OnWispReadyToRevive()` which calls `Revive()`.


## Fix: Q Toss Direction Debug Logging + Build Recovery (2026-05-29)
- **Problem**: On second Q press, Moonlight Zone flies off to "narnia" instead of toward the aim cursor. Need targeted debug logs to diagnose. Additionally, a previous `sed`-based log stripping attempt left dangling code lines that broke the build. A `git checkout` recovered tracked files but exposed that many untracked in-progress files reference APIs that never existed in the committed base.
- **Q Toss Debug Logs Added**:
  - `GA_Eluna_Q::TossZone()` — `[Q_TOSS] AimDir=%s | HunterLoc=%s | RawTarget=%s | TraceHit=%s | FinalTarget=%s | ZoneCurrent=%s`
  - `BBMoonlightZone::TossToLocation()` — `[Q_TOSS] Zone::TossToLocation | Start=%s | Target=%s | Speed=%.0f | HasAuthority=%s`
  - `BBMoonlightZone::UpdateTravel()` — per-tick `Travel | Cur=%s | Target=%s | Dist2D=%.1f | MoveDist=%.1f` + `LANDED` / `MOVED` logs
- **Build Fixes** (to make project compile cleanly):
  - Added missing `EWispState` enum to `BBWispPawn.h` (was referenced but never defined in the untracked wisp file)
  - Added `ApplyBBCooldown()` helper to `UBBGameplayAbility` base class (used by all Eluna/Kingpin abilities)
  - Added ~30 missing gameplay tags to `BBGameplayTags.h/cpp` including `State_Wisp`, `SetByCaller_HealAmount`, `State_Executing`, Eluna/Kingpin ability/cooldown tags, and tags for in-progress features (StormShift, MostWanted, etc.)
  - Fixed dangling UE_LOG parameter syntax errors in `BBElunaStickyProjectile.cpp` and `GA_Eluna_Shift.cpp`
  - Added `#include "Engine/OverlapResult.h"` to `BreachbornePlayerController.cpp`
  - Added stub methods/enums to core tracked classes so in-progress untracked files compile:
    - `BreachborneTypes.h`: `EPlayerLifeState`, `EBBDayNightPhase`, `Mantling` movement mode
    - `BBCharacterMovementComponent.h`: `IsMantling()`, `EnterMantling()`
    - `BreachbornePlayerState.h/cpp`: `GetLifeState()`, `GetHealthData()`, `GetCombatData()`, `GetMovementData()`, `GetWispData()`, `GetKnocks()`, `GetWispHealthSet()`, `UpdateWispHealthProxy()`, `FRepWispData`, `Rep_WispData` replication
    - `BreachborneGameState.h/cpp`: `GetDayNightPhase()`, `AdvanceDayNightPhase()`, `GetDayNightCycle()`, `IsNight()`
    - `BreachborneGameMode.h`: `HandleTrainDeath()`
    - `HunterCharacter.h/cpp`: `Multicast_DrawDebugCircle_Implementation()`, `BeginHookPull()`
    - `StormShiftBase.h`: `ApplyShiftEffects()`, `RemoveShiftEffects()` virtuals
    - `BBWorldItem.h`: `SetPickupItemID()` public setter
  - Re-added debug console commands to `BreachbornePlayerController` that were lost in git checkout: `DebugTestAllyDecay`, `DebugTestAllyKill`, `DebugTestAllyRevive`, `DebugSpawnTestAlly`, `SpawnTestWisp` / `ServerSpawnTestWisp`
- **Result**: Build compiles successfully. Q toss debug logs are active — run PIE, press Q twice, and check the Output Log for `[Q_TOSS]` entries to diagnose the aim/target calculation.

## 2025-05-30 — Wisp HUD Indicators + Heal-Driven Revive

**Goal**: Add screen-space decay/revive bars above wisp pawns, and make heals contribute directly to revive progress.

**Files changed**:
- `Source/Breachborne/Wisp/BBWispPawn.h/cpp`
- `Source/Breachborne/UI/WispIndicatorWidget.h/cpp` (updated existing)

**Implementation**:
- **WidgetComponent**: Added `UWidgetComponent` in Screen space to `BBWispPawn`, positioned 80cm above the wisp. Assigned `UWispIndicatorWidget` as the widget class.
- **Indicator widget (`UWispIndicatorWidget`)**: C++-constructed UMG widget with two `UProgressBar` widgets in a `UVerticalBox`:
  - **Yellow decay bar**: `WispHP / MaxWispHP` (Supervive yellow `#FFD500`)
  - **Green revive bar**: `RezBarProgress` 0-1 (bright green `#33FF33`)
  - Polls every tick via `NativeTick` — no OnRep needed
- **Replicated HP for decay bar**: Added `ReplicatedWispHP` and `ReplicatedMaxWispHP` to `BBWispPawn`, synced from `WispHealthSet` every `ServerWispTick`. Uses `OnRep_WispHP`.
- **Heal detection**: Bound `OnWispHPChanged` to the ASC's `WispHP` attribute change delegate in `InitWisp`. Only accumulates **positive** changes (healing), ignoring drains.
- **New tuning constants**:
  - `HealToReviveConversion = 0.005f` — each HP healed contributes this much to `RezBarProgress`
  - `MaxReviveMultiplier = 2.0f` — hard cap on total revive speed
- **Rewritten `ServerWispTick` logic**:
  - **Carried by Eluna**: fills at `MaxReviveMultiplier` (2.0x), no drain
  - **Ally nearby + no enemy**: proximity contributes 1.0x, decay paused
  - **Heals**: converted to an equivalent multiplier via `HealToReviveConversion`
  - **Ally + heal**: multipliers add together, then capped at `MaxReviveMultiplier`
  - **No revive source**: rez bar decays at `RezDecayRate`, HP drains at `BaseDrainRate` (or `StompDrainMultiplier` if enemy present)
- **Widget initialization**: `BeginPlay()` binds the widget to the wisp on all clients + server. `InitWisp()` also does it server-side for early setup.

**Result**: Build compiles successfully. Wisps now show yellow (HP decay) and green (revive progress) bars above them. Heals from any source (e.g. `BBMoonlightZone`) automatically fill the green bar.

## UI Serialization Break — 2026-05-24

### Problem
Custom UI (weapon slots, hero HP, stats, cooldowns) stopped rendering after `BBWispHealthSet` integration into `BreachbornePlayerState`.

### Root Cause
Added `WispHealthSet` as a `UPROPERTY` subobject and `Rep_WispData` as a `UPROPERTY(ReplicatedUsing)` on `BreachbornePlayerState`. This violated the build rule: **do NOT add replicated properties to core classes (PlayerState, GameState) as stubs** — breaks UI serialization at runtime.

### Fix
1. **Removed `WispHealthSet` entirely from `BreachbornePlayerState`** (header + cpp).
2. **Reverted `GetWispHealthSet()` to return `nullptr`** and `UpdateWispHealthProxy()` to no-op.
3. **Moved wisp HP management into `BBWispPawn`** directly via `ReplicatedWispHP` / `ReplicatedMaxWispHP`.
4. **Added `BBWispPawn::ApplyHeal(float)`** for external heal sources (e.g. Eluna R channel, moonlight zone).
5. **Updated `GA_Eluna_R`** to call `Wisp->ApplyHeal()` instead of manipulating `WispHealthSet`.

### Result
Build succeeds. `BreachbornePlayerState` no longer has wisp-related replicated properties.
