# Supervive Mechanics Reference

Full specification of game mechanics for Breachborne, modeled on Supervive Open Beta. Load this on-demand when implementing any specific system.

---

## Sacred vs. Negotiable

When implementation pain on a system exceeds its value to the prototype, refer to this list to decide whether to push through or ship a simpler version.

### Sacred (cutting these means it's no longer the game we're building)

- **Top-down isometric camera** — defines the entire feel; not negotiable
- **Gliding + spike + abyss death** — signature mechanic; the "I just got spiked" moment is core
- **Wisp state with proximity revive, enemy stomp, and execute** — defines combat closure feel
- **Deathbox lootability** — drives the punishment-and-pickup loop after a fight
- **Open Beta item philosophy** — equipment found in match, upgraded via shards, NO persistent unlocks
- **Skillshot-based ability combat** — most abilities are aimable, not auto-targeted
- **Powers (F/G slots)** — meaningful per-match build customization separate from equipment
- **Squad-of-4 BR structure with shrinking storm**
- **Five hunter roles** (Fighter / Initiator / Frontliner / Protector / Controller)

### Negotiable (simpler versions acceptable if needed)

- **Mantling** — could ship without if too gnarly; affects feel but not core combat
- **Storm shifts beyond Bullet Trains** — Bullet Trains is iconic, others can be added post-prototype
- **Vision cones / sneaking / brush stealth** — could ship with full visibility in prototype
- **Day/night cycle gating level cap** — could be fixed timer in prototype, day/night purely visual
- **Contract system** — could ship with smaller quest list (3-4 quests, no tiering) and expand later
- **Wisp carry ability** — Elluna-only mechanic, could be omitted from her kit in prototype
- **Most Wanted crown** — adds drama but Beacon revival covers the same design need
- **Multiple boss types** — one good boss is better than five half-baked ones
- **Stagger refund interaction** — could ship as "ability hits staggerable mob does bonus damage" without the cooldown/mana refund mechanics in prototype
- **Forge armor repair zone** — Basecamp anvil repair covers armor restoration; Forge is the "risky free" alternative
- **Big Shard reward** — tier 3 contracts could just give more regular shards in prototype

---

## Hunter Roles

Five roles, each fills a tactical niche in squad composition:

- **Fighter** — primary damage dealer. Squishy, high DPS. Examples: Ghost (ranged), Brall (melee).
- **Initiator** — engages fights, displaces enemies, tankier than Fighters. Example: Kingpin.
- **Frontliner** — tankiest role, soaks damage, creates space. Example: Felix.
- **Protector** — heals/shields allies, support utility, can revive. Example: Elluna.
- **Controller** — zoning, crowd control, area denial. Example: Bishop.

Every hunter is balanced to be theoretically able to 1v1 any other hunter, with role differences expressing through team utility rather than raw power.

---

## Hunter Kit Structure

Each hunter has a fixed input layout:

- **LMB** — primary weapon attack (auto-firing or click-to-shoot, hunter-specific)
- **RMB** — secondary weapon attack (alt-fire, often utility or status effect)
- **Shift** — dodge/dash (movement ability, short cooldown)
- **Q** — primary basic ability
- **R** — ultimate ability (long cooldown, unlocks at level 5)
- **Passive** — always-active effect, no input required
- **F / G** — power slots (filled in-match by Powers picked up from vaults/bosses/drops)

---

## Movement & Camera

### Top-down camera

- SpringArm at ~55-65° angle
- Camera follows local player's pawn
- NOT replicated — local-only
- Edge-of-screen panning is optional polish

### Movement

- WASD movement, planar XY
- Character does NOT rotate to face movement direction
- Character DOES rotate to face cursor (independent of movement direction)
- Use `bOrientRotationToMovement = false` and `bUseControllerDesiredRotation = true`
- Movement preserves momentum across acceleration sources (gliders, dashes, jump pads)

### Sneaking

- Left Ctrl: sneak mode
- Slower movement
- Expanded vision cone (you see more of the surrounding area)
- Vision is directional (tied to facing direction)
- Brush/bushes hide hunters from enemy vision (stealth zones)

### Vision

- Each player sees a vision cone in front of them, scaling with day/night
- Brush hides hunters inside it from enemies outside
- Server replicates all actor positions to all clients (no fog-of-war culling on the network layer)
- Vision is purely client-side rendering for prototype scope
- Server-side relevancy filtering for true anti-wallhack is deferred to post-prototype

---

## Gliding & Spiking

### Glider mechanics

- Glider opens when jump is held while in the air
- Consumes glider fuel over time (visualized as a circle above the player)
- Fuel refills while walking on solid ground
- When fuel is empty: glider closes and cannot redeploy until refueled on ground
- Glider can be released and reactivated to conserve fuel ("Feathering")
- Below ground level, activating the glider gives an upward impulse to ground level
- Releasing the glide just before reaching ground level converts impulse to vertical momentum (advanced traversal)

### Spiking

- Taking ANY damage while gliding: transition to Spiked state
- Spiked: stunned, fall to the ground (or into the abyss)
- Over abyss (Z below `AbyssThreshold`): instant kill, deathbox spawns at the killer's location
- On solid ground: stun + fall damage, can be picked up by team
- Spike state duration ~1.5s, then transitions to falling normally
- Some abilities grant temporary spike immunity

### Implementation notes

- All glider state is server-authoritative (FuelLevel, GliderState, Spiked transitions)
- Client predicts opening/closing for responsiveness
- Fuel display interpolates from replicated value
- See `@docs/architecture.md` for the gliding component architecture

---

## Mantling

The Breach is built of floating islands and uneven terrain at varying heights. Mantling lets a player who is below "ground level" lift themselves up onto a higher surface by walking into its side.

### What "ground level" means

Imagine a pit. The walls of the pit are vertical surfaces. The top edge of the pit (where you'd stand if you climbed out) is the horizontal plane — that's "ground level" for that pit. Different areas of the map have different ground levels: a plateau's top is its own ground level, the surrounding lower terrain has its own. Walking into the side of the plateau from below should mantle you up onto it.

### Trigger

- Mantling is **NOT** triggered by jump
- Mantling triggers when a player below ground level presses a directional key whose direction causes their hitbox to collide with a wall
- There's leniency on the directional key as long as the resulting motion produces wall collision
- Pressing jump near a wall does a normal jump, not a mantle

### Mantle motion

- Character is lifted by a small autonomous hop that overshoots the target ground level by a small amount
- The overshoot is critical: stopping exactly at ground level would leave the character's capsule colliding with the ledge edge (standing in front of the ledge, not on top of it). The overshoot puts the capsule cleanly on top so the character can land and walk seamlessly.
- Player keeps momentum across the mantle — they don't come to a static stop

### What counts as "mantleable" (geometry-driven, not tag-driven)

There is no need for level designers to mark surfaces as climbable. The system reads the world's collision geometry and decides on the fly. The check answers: "is the top of this wall a place I could stand?"

A surface qualifies as mantleable if:

1. The character's input causes their capsule to collide with a wall (a steep surface that can't be walked up)
2. From the wall collision point, tracing upward along the wall finds a top edge
3. From that top edge, casting downward a short distance hits a surface whose normal is within UE's `WalkableFloorAngle` (default ~45° from vertical-up). Gentle slopes pass; steep walls fail.
4. The walkable area above has enough headroom for the character capsule to occupy

If any check fails, no mantle. This naturally distinguishes:

- **Walls connecting to flat or gently sloped terrain** → mantleable
- **Walls connecting to more wall, steep cliffs, or thin geometry edges** → not mantleable
- **Walls near deathbox-platforms over the abyss** → mantleable (the deathbox ground patch has a walkable normal)

### Selecting a target ledge when multiple qualify

If more than one mantleable surface is reachable from the player's position (e.g., a stepped wall with two different ledge heights), pick by:

1. **Closest first** — the candidate whose target landing point is nearest the character capsule's center wins
2. **Highest as tiebreaker** — if two candidates are effectively equidistant, prefer the higher one

In practice, exact equidistance is rare, so closest-wins handles almost all cases.

### Cancellation and edge cases

- Player cannot manually cancel a mantle in progress
- Player can be Spiked (dunked) while attempting to mantle
- If a Spike pushes the player back into the wall, mantling re-triggers and they may be lifted up again
- Mantling works on any walkable static geometry — not tagged surfaces, not pre-marked ledges
- Critically: this includes the temporary "ground" patch under a deathbox spawned over the abyss. As long as walkable geometry exists at mantle reach, the player can mantle to it
- This means mantling is an emergent physics property, not a level-design property

### Implementation approach

The detection logic, in order:

1. Player input collides their capsule with a wall surface (server detects via sweep/collision)
2. From the wall collision normal, trace upward along the wall face to find the top edge of the wall geometry
3. From the top edge, trace forward + downward a short distance to find a candidate landing surface
4. Read the landing surface's normal; reject if it's steeper than `WalkableFloorAngle`
5. Sweep a capsule-shaped probe at the landing point to confirm headroom for the character
6. If multiple candidates passed (stepped geometry), apply the closest-wins-with-height-tiebreaker rule above
7. Trigger the mantle: root motion animation, capsule lifts to slightly above the landing surface, character resumes normal movement

Network specifics:

- Detection runs on the owning client for responsiveness
- The mantle is sent to the server as a movement mode change; server re-runs the same checks to validate
- Custom `CharacterMovementComponent` movement mode (e.g., `MOVE_Mantling`) handles replication
- During mantle: character is briefly not actionable (no abilities, no attacks, ~0.5s)
- If server rejects the mantle, it corrects the client position back

### UE 5.7 note

UE 5.7 has improved built-in character movement features. Before implementing custom mantle logic, evaluate whether the engine ships something that already covers this. If so, use it. If not, the algorithm above is the target behavior.

---

## Wisp System

When a hunter's HP reaches 0, they enter a downed state called Wisp.

### Wisp transition

```
HP reaches 0
|
+-- Death from fall / abyss / spike-over-abyss  ->  Skip wisp, instant deathbox
|
+-- Normal death  ->  Transition to WISP STATE
```

### Wisp state behavior

- Wisp is a separate replicated pawn (`AWispPawn`)
- Wisp has its own HP that drains over time (configurable rate)
- Wisp is **invulnerable to direct damage** — cannot be shot, only affected by proximity and execute interaction
- Wisp can move slowly, in any direction
- Wisp cannot use abilities, attack, or glide

### Proximity interactions (server tick every 0.2s, ~2m radius)

- **Ally nearby:** rez bar fills at configured rate. When rez bar reaches 1.0 → revive at partial HP, keep all items (Gold, Equipment, Powers, Consumables)
- **Ally steps away:** rez bar AND wisp HP both resume draining
- **Enemy nearby (stomp):** wisp HP drains FASTER
- **Both ally and enemy nearby:** rez bar fills, but HP still drains (reduced rate vs. enemy-only)
- Multiple allies do not stack rez speed — one is enough

### Execute mechanic

- Enemy presses E within range of wisp
- Server starts execute sequence: executor enters locked animation (~2s)
- During animation: executor cannot move, cannot use abilities
- If executor takes damage during animation: execute is CANCELLED (server-validated)
- If animation completes:
  - Wisp instantly killed → Deathbox spawns
  - Executor fully healed (HP and mana restored)

### Wisp carrying (hunter-specific ability — e.g., Elluna)

- Carry-capable hunter activates ability near ally wisp → wisp attaches to carrier
- Carrier gains a shield buff (GameplayEffect)
- Carrier's movement speed reduced
- Carrier CAN still use abilities and attack (reduced effectiveness optional)
- Wisp HP drain pauses while being carried
- Carrier can put the wisp down (re-press interact)
- If carrier dies while carrying: wisp drops at carrier's location, resumes normal wisp state

### Network notes

- All wisp state is server-authoritative
- Wisp position, HP, rez bar progress, carrier attachment all replicate
- Execute animation lock is server-validated (prevents animation cancel exploits)
- See `@docs/architecture.md` for the full wisp lifecycle diagram

---

## Deathbox & Further Revival

When the wisp dies (HP reaches 0), the player enters Deathbox state.

### Deathbox

- `ADeathboxActor` spawns at wisp death location
- Contains snapshot of player's inventory: equipment, powers (non-soulbound), armor, gold, consumables
- Enemies can interact to loot individual items
- Soulbound items (5-star powers) drop on the ground at deathbox spawn instead of staying in the box
- Special: deathboxes spawned over the abyss create a small temporary "ground" patch beneath them. The ground exists as long as the deathbox does (looted dry → both disappear). Players can mantle onto this patch.

### Revival paths from deathbox

**Channeled deathbox revive:**
- Teammate holds interact (E) near deathbox for ~5s
- Loud sound effect plays (audible to nearby enemies through fog of war)
- Enemy entering the area pauses progress
- Revived player returns with reduced HP and NO equipment (must re-loot)

**Respawn Beacon:**
- Fixed map locations, one-time use per match
- Any living teammate activates → ALL dead squad members revive
- Loud audio + visual indicator on minimap visible to enemies
- Some beacons require building (spend Gold) before use
- Greyed icon = needs building, red icon = already used

**Everbeacon:**
- Near map center, reusable
- Beacon sphere KILLS any hunter on contact (extreme risk/reward)

**Most Wanted crown:**
- Pickup that appears when squad has dead members
- Carrier's position broadcast to ALL teams on the map
- If carrier survives ~2 minutes or gets enough kills/spikes → all dead teammates revive
- Knocks/kills/assists shave time off; spike kills may instantly trigger
- High-risk, high-reward

### No Resurgence

There is no auto-respawn timer. Once you're a deathbox, you stay there until revived through one of the paths above. (Ranked-style rules.)

---

## Equipment & Item System (Open Beta era)

Every match starts fresh. NO persistent between-match progression. NO Armory, NO Prisma, NO Forge unlock system. Pure skill expression.

### Equipment slots

- **Weapon (1 slot):** 4 weapon types, each with 3 evolution paths. Role-specific.
- **Helmet (1 slot):** 3 helmet types, each with 3 evolution paths. Defensive/utility effects.
- **Boots (1 slot):** movement speed + a passive effect.
- **Armor (worn):** tiered white → green → blue → yellow → red. Found as world drops, repaired at Basecamp anvils or with Armor Shards.

### Equipment sources

- Creep camp drops (random equipment + upgrade shards)
- Vaults (guaranteed high-quality items, contested locations)
- Deathboxes (enemy equipment lootable on death)
- Map shopkeepers (buy with Gold at fixed locations)
- Chests / supply drops scattered across the map

### Upgrade system

- `UUpgradeShard` — collected from PvE kills, PvP kills, and objectives
- Equipment starts at base tier
- Collect enough shards → upgrade → choose one of three evolution paths
- Each evolution gives different stats and a unique passive/active effect
- Evolution choice is permanent for that match
- `FEquipmentEvolution` data asset defines stats, passive GE, and description per evolution

### Big Shard

- Special reward (e.g., from Tier 3 contracts)
- Equivalent to 10 regular shards
- Effectively an instant upgrade for an item of the player's choice
- Implemented as a consumable or auto-applied resource — TBD during Phase 6 implementation

### Powers (F / G slots)

- Two power slots, separate from equipment
- `UBBPowerDefinition` — data asset. Each power is a GameplayAbility granted to ASC on pickup.
- Sources: vaults (especially Abyssal Vaults), bosses, world drops, enemy deathboxes
- Some powers are passive (always active), some are activatable (bound to F or G)
- Rarity tiers: Common, Uncommon, Rare, Epic, Legendary, Exotic
- Non-Legendary/Exotic powers are Soulbound — don't drop on player death, respawn with the player
- Legendary/Exotic powers drop on death — contestable loot

### Consumables (backpack slots)

- Limited slots — inventory management matters
- **Vive Beans:** found in the world. Used to brew Vive Brews at basecamps.
- **Vive Brews:** crafted at basecamps. HP regen + mana over time.
- **Food:** HP regen over time.
- **Armor Shards:** repair one bar of armor (default key '2').
- **Utility consumables:** temporary buffs (speed, vision, etc.)

### Gold economy

- Earned from: PvE camp clears, objective completion, PvP kills (steal portion of enemy's gold), vaults
- Spent at: map shopkeepers (fixed locations) for items, basecamp anvils for armor repair
- Equipment from shopkeepers starts at base tier (still needs shards to evolve)
- Server-authoritative: balance, validation, transaction logging

---

## Forge (armor repair zone — Open Beta)

A risky alternative to paying for armor repair at basecamps.

- Circular zone marked on the map
- Standing in the zone: armor regenerates over time
- Regen rate scales UP the longer you stay in the zone (resets to 0 if you leave)
- While inside: player has a debuff causing them to take **200% damage from all sources**
- Visualized: debuff icon above the player's head
- Trade-off: free repair vs. extreme vulnerability — incentivizes contesting Forges, not just sitting in them

---

## Basecamp

A team-capturable control point with multiple functions.

- `ABasecampActor` — capture by standing on it for a duration
- Capturing makes a sound — alerts nearby enemies
- Once captured by your team:
  - **Anvil interactable:** spend Gold to repair armor (20% per repair). Cost scales with armor tier:
    - Grey: 100 gold
    - Green: 200 gold
    - Blue: 500 gold
    - Yellow: 1000 gold
    - Red: 1000 gold
  - **Vive Brewing station:** convert Vive Beans into Vive Brews
  - Short-term safe zone

---

## PvE Ecosystem

### Creep camps

- `ACreepCamp` spawns a group of `ACreepCharacter` actors (server-authoritative AI)
- Simple BehaviorTree: idle → aggro on proximity → attack → leash back to spawn
- Killing creeps grants XP, drops Gold, Upgrade Shards, and chance for equipment
- Camp respawn timer managed server-side
- Camps marked on map; creep level scales with day/night phase

### Staggerable mobs

A high-skill mechanic that rewards ability planning.

- Some mobs have telegraphed abilities with a windup animation
- During the windup, UI displays "Staggerable" tag on the mob
- If a player hits the mob with a damaging ability during the windup window:
  - Mob's ability is INTERRUPTED
  - Mob takes a big chunk of damage (configurable multiplier on the ability's damage)
  - The first hitting player has their non-ultimate ability cooldowns reset
  - The first hitting player has their mana cost refunded
- If no one hits with an ability during the windup:
  - Mob completes the ability and deals damage
  - Mob gains a shield buff afterward
- Only the first damaging ability triggers the refund — chaining doesn't give multiple refunds
- Ultimates do NOT get cooldown reset, but DO interrupt
- Quirky interactions to preserve: abilities that don't directly damage but enable damage (e.g., Crysta's empowered-auto dash) can still trigger the stagger via the empowered hit
- This rewards squad coordination: line up an enemy team's stun + your stagger window for a double-payoff fight

### Bosses

- `ABossCharacter` extends `ACreepCharacter` with more complex AI
- Drop unique Powers and Soul Buffs (team-wide passive effects)
- Contestable: multiple squads can fight over a boss simultaneously
- Boss health bar replicated and visible to nearby players
- Different bosses tied to biomes; spawn at specific day/time phases

### Vaults

- `AVaultInteractable` — channeled "hacking" mini-game
- Regular vaults: guaranteed equipment drop
- Abyssal Vaults: contain Abyssal Items (special high-power equipment)
- Heavily contested early game — marked on map from match start

---

## Contracts (Open Beta quest system)

Tier-based quest progression; rewards auto-distribute to the whole team on completion.

### Quest NPC behavior

- After landing, if a player hasn't picked up a quest, an NPC spawns near them
- If the player walks away without accepting, NPC despawns and re-spawns near them later
- If the player is near a basecamp (empty / theirs / enemy's), the NPC spawns at the basecamp instead of next to the player
- Anyone in the squad can pick up a quest from the NPC
- Once a quest is picked: locked in for 2 minutes or until completion
- If 2 minutes elapse without completion: quest times out, no reward, but team can pick up a quest from the next tier

### Tier 1 quests (initial)

Pool of three options; team picks one:
- Kill X creeps
- Open a vault
- Capture an enemy basecamp

**Reward:** 500 gold + 1 shard, auto-granted to whole team on completion

### Tier 2 quests (unlocked after Tier 1 completion)

Pool from a wider set:
- Kill the Mega Boar
- Get a supply drop
- Kill an enemy player
- (Additional quests TBD; final pool roughly 10-12 across all tiers)

**Reward:** 1000 gold + 3 shards, auto-granted to whole team on completion

### Tier 3 quests (unlocked after Tier 2 completion)

Final tier, focused on PvP objectives.

**Reward:** 2000 gold + 1 Big Shard (worth 10 shards, instant upgrade for chosen item), auto-granted to whole team on completion

---

## Storm System

### Shrinking storm

- `AStormManager` is server-only; storm state replicates via `FStormCircleState` on GameState
- Storm visualizes and shrinks on a schedule
- Hunters outside the safe zone take damage via DOT GameplayEffect
- Damage scales up across phases (light tick early, lethal late)

### Storm shifts (match mutators)

`UStormShiftBase` modular subclasses. GameMode picks one at match start and replicates the choice.

**Confirmed for prototype:**

- **Bullet Trains:** lethal moving actors travel along fixed paths across the map. Iconic, fun, must-implement.

**Examples to consider (post-prototype or as time allows):**

- **Nomadic:** safe zone moves large distances between phases
- **Global Oracle:** reveals all team positions every 30 seconds
- **Twisters:** tornadoes spawn across the map
- **Soul Powers:** each team spawns with 4 soulbound powers, one per player

Storm shifts are an extensibility point — adding new ones is data + a single subclass.

---

## Match Flow

### Day/night cycle

- Match progresses through Day → Night → Day → ... phases
- Each phase has a configurable duration (in a data asset, NOT hardcoded)
- Level cap scales by phase: e.g., Day 1 cap = 5, Day 2 cap = 10, Day 3 cap = 15
- Baseline values from Open Beta era (Day 1: ~Lv5, Day 2: ~Lv7, Day 3: ~Lv8) — adjusted upward for prototype balance room
- PvP kills can grant XP that exceeds the current cap (rewards aggression)

### Phases (gameplay arc)

- **Phase 1 (Early / Day 1):** Drop, farm creep camps, open vaults, buy first items, choose contract
- **Phase 2 (Mid / Day 2):** Bosses spawn, contest objectives, storm starts closing, upgrade equipment
- **Phase 3 (Late / Day 3+):** Storm forces fights, final squads clash, last team standing wins

### Oracle

- Interactable map object
- Reveals enemy team locations briefly when used (UAV-equivalent)
- Different from the Global Oracle storm shift (which is automatic and global)

---

## Squad Configuration

- Squad size: **4** (fixed)
- 10-12 teams per lobby (40 player target)
- All squad members are on the same team for the entire match
- Friendly fire OFF
- Squad communication via voice chat (default UE VOIP for prototype)

---

## Hunter Roster (prototype scope)

Five hunters for the prototype, one per role. All five must be playable, replicating correctly, and tested in PIE.

| Hunter | Role | Purpose |
|--------|------|---------|
| Ghost | Fighter | Reference implementation. Tests projectile + hitscan + AoE + DOT patterns. |
| Kingpin | Initiator | Tests displacement networking (hook pull replication). |
| Elluna | Protector | Tests heals, ult revive, **wisp carry** end-to-end. |
| Felix | Frontliner | Tests continuous damage replication (flamethrower). |
| Bishop | Controller | Tests positional manipulation and zone control. |

Once these five work in multiplayer, additional hunters become mostly data-driven content work.
