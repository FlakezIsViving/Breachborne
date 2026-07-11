# SUPERVIVE November 2024 Open Beta Architecture Audit

Scope: SUPERVIVE during the 24/7 Open Beta that went live on 2024-11-20. This file treats the 2024-11-15 Open Beta patch notes as the launch baseline, the 2024-11-26 Week 1 patch notes as the first live correction pass, and the 2024-12-08 hotfix / 2024-12-18 winter patch as post-launch deltas where the prompt asks for Week 2 / late-beta verification.

Primary source anchors:

- Steam news `1783238125254574`: `[11.20.2024] Open Beta Fireside Chat & Patch Notes`, dated 2024-11-15. URL: `https://steamstore-a.akamaihd.net/news/externalpost/steam_community_announcements/1783238125254574`
- Steam news `1783872411949266`: `The SUPERVIVE 24/7 Open Beta is LIVE!`, dated 2024-11-20. URL: `https://steamstore-a.akamaihd.net/news/externalpost/steam_community_announcements/1783872411949266`
- Steam news `1783872412147263`: `SUPERVIVE Week 1 Open Beta Patch Notes`, dated 2024-11-26. URL: `https://steamstore-a.akamaihd.net/news/externalpost/steam_community_announcements/1783872412147263`
- Steam news `1785321795706021`: `Dec. 8 - Hotfix Balance Patch Notes`, dated 2024-12-08. URL: `https://steamstore-a.akamaihd.net/news/externalpost/steam_community_announcements/1785321795706021`
- Steam news `1786409468842691`: `[UPDATED 12/20] The Cosmic Winter Jubilee Pt. 2 Patch Notes!`, dated 2024-12-18. URL: `https://steamstore-a.akamaihd.net/news/externalpost/steam_community_announcements/1786409468842691`
- Steam news `6212245217911770781`: `Thanks for making SUPERVIVE a top demo at Steam Next Fest!`, dated 2024-10-22, used only for Steam Next Fest vault-attempt statistics that predate Open Beta. URL: `https://steamstore-a.akamaihd.net/news/externalpost/steam_community_announcements/6212245217911770781`

Use this as a reverse-engineering spec, not as a legal clone brief. Every mechanic below should be implemented through equivalent systemic roles in Breachborne, with names/data remapped as needed.

---

## 1. Input & Keybind Inventory

### Movement, Aim, and Camera

| Input | November 2024 role | Mechanical contract |
|---|---|---|
| `WASD` | Planar movement. | Movement is independent from aim direction. Players can kite, retreat, strafe, and cast toward cursor without needing the character to face travel direction. Several channels and placements are WASD-cancellable after a small startup grace period; the October Steam Next Fest notes say most actions were cancellable after `0.25s`, while vaults were cancellable after `0.5s`. |
| Mouse position | Independent aim vector. | Abilities aim toward cursor / projected world point. The October notes mention cast-range display on hover and an aiming-laser option that can extend past cursor to full length, which implies cursor-oriented vector casting was core UI. |
| Mouse camera / map zoom | Tactical view control. | Open Beta notes mention `Default zoom`, `Smart zoom` by holding `RMB`, and returning to prior map view on release. This belongs to camera/map readability, not movement. |
| `Space` | Jump / aerial traversal / rail and glider interaction. | Jump is part of vertical island traversal. Holding or pressing jump in air deploys or maintains the glider. In Open Beta, the glider soft ceiling was tuned to pull down less harshly, out-of-bounds buffer did not regenerate until grounded, and players could grab grindrails from below and start grinding on top. |

### Hunter Kit Layout

| Input | Kit slot | Systemic role |
|---|---|---|
| `LMB` | Primary weapon / main attack. | Hunter-specific primary. Examples in Open Beta notes include Ghost `MK7 Glyph Rifle`, Hudson `Minigun`, Kingpin `Scattergun`, Oath `Magnetic Hammer`, Brall `Caldera`, and Myth `Greenwood Bow`. LMB can be projectile, hitscan-like, channeled, charged, or repeated fire. |
| `RMB` | Secondary weapon / alternate attack. | Hunter-specific alt-fire or utility. Examples: Ghost `Spike Grenade`, Hudson `Overclock`, Jin `Flash Dagger`, Joule `Voltaic Spear`, Oath `Brightshield`, Myth `Heartpiercer`. RMB may also participate in smart zoom / camera behavior when not casting, so input routing must prioritize ability state. |
| `Shift` | Movement ability. | Dash, hover, blink, slide, or jump. Open Beta Week 1 changed Ghost `Combat Slide` cooldown and Hudson `Hover Jets` duration, proving Shift is the mobility slot. |
| `Q` | Basic ability / tactical spell. | Non-ultimate active ability, usually shorter cooldown and often CC, zone, beam, or utility. Open Beta examples include Kingpin `Primal Slam`, Hudson `Barbed Wire`, Ghost `Arcane Railgun`, Oath `Regen Field`. |
| `R` | Ultimate. | Long-cooldown hunter-defining ability. Open Beta notes include Bishop `Blast-off!`, Brall `Volcanic Rebuke`, Shrike `Full Bore`, Void `Singularity`, Felix `Flight of the Firefox`. |
| Passive | No direct input. | Passive effect reacts to combat, movement, status, or ally state. Examples in notes: Elluna `Soulpack`, Felix `Ignite`, Hudson `Salvager`, Brall `Blazing Spirit`. |

### Recall, Communication, Inventory, and Interaction

| Input | November 2024 role | Mechanical contract |
|---|---|---|
| `B` | Recall. | Public patch notes searched here do not expose exact default binding text for recall. The system role to preserve is a channeled, interruptible relocation/return action that cannot become an escape during active storm/fight pressure. Treat prompt-specific constraints as design targets: blocked or limited in storm, cannot freely cast on arbitrary allies or captured camps, and should apply movement/action suppression during channel. |
| `Tab` | Inventory / map / scoreboard layer. | Open Beta notes refer to added game-state information when opening the map, upgraded map/mega-map communication, inventory/progression reset, equipment upgrade hotkeys, and shop/item tabs. Treat `Tab` as the primary overlay surface for inventory, squad status, map objectives, quest state, storm state, and death/respawn state. |
| `G` | Ping / communication wheel or power slot, depending keymap mode. | Local design notes model Powers on `F/G`; the user prompt names `G` for ping wheel/map communication. Preserve both by making bindings data-driven: powers need two active slots, while ping wheel requires a hold/tap communication input with world, minimap, and map-overlay targets. |
| `F` / `E` | Contextual interact. | Used for world interactions and channels. Objects requiring channeling or committed interaction include vaults, armor repair at Basecamp, brewing Vive, resurrection beacons, deathbox revives, supply drop loot, train puzzles/chests, bonfires/base camps, quest vendor acceptance, and enemy-wisp execute/stomp-adjacent interactions. |
| `Alt + 1-3` | Equipment / boots upgrade. | Open Beta notes explicitly added `Alt + 1-3` to upgrade equipment/boots. |
| `Alt + ability` | Ability level-up. | Open Beta notes remind players they can press `Alt` plus abilities to level them. |

Important input architecture rule: channelled actions must share a cancellation/interruption model. WASD movement, damage taken, enemy contesting, storm state, or losing valid range should cancel or pause the action according to object type. This avoids one-off code paths for vaults, armor repair, brewing, revives, and executes.

---

## 2. Interactive Entity Census: Map Objects

This section is organized by object behavior rather than by a hand-authored feature checklist.

### Dynamic Transports and Hazards

#### Grindrails

- Players can attach from below and transition to the top of grindrails; this was explicitly called out in the 2024-11-15 Open Beta notes.
- `Space` participates in traversal timing. Jump timing on landing can chain higher jumps through equipment (`Chain Jump` was listed as removed/replaced in the Open Beta equipment notes), so rail exit and jump-buffer timing must be robust.
- Rails are traversal affordances, not just static set dressing. They need entry volumes, forced movement, exit impulse, collision handoff, and ability/movement cancellation rules.

#### Trains

- Trains existed before and during Open Beta. The 2024-11-15 notes mention train puzzle colors updated for colorblind accessibility, and the October notes mention gold armor hot zones could spawn on the train.
- Train components by systemic role:
  - Moving platform / transport path.
  - Collision hazard: being struck or misplaced around trains can spike/kill.
  - Loot carrier: train puzzle/chest or hot-zone loot.
  - Objective surface: hot zones can appear on it, making train control valuable.
  - Storm-shift candidate: later roadmap and local implementation use Bullet Trains as a match mutator.
- Implementation requirements:
  - Moving-base replication for riders.
  - Front/side hit detection separate from walkable carriage collision.
  - Loot/puzzle state replicated independently of train movement.
  - Colorblind-safe puzzle signals.

#### Abyss, Out-of-Bounds, Spikes, Dunks, Mantles

- SUPERVIVE's map is floating-island topology. Abyss is not a decorative boundary; it is a lethal combat surface.
- Glider and spike rules interact with abyss. The Open Beta notes tuned glider soft ceiling and out-of-bounds buffer, and fixed glider trails / mid-physics crash behavior.
- Spikes, dunks, and mantles received VFX updates in the Open Beta notes, so all three were player-facing systems.
- Abyss interaction model:
  - Grounded or recoverable state: player can mantle, glide, or use traversal back to safe terrain.
  - Spiked/dunked over abyss: lethal conversion pressure.
  - Some powers can move objects into abyss; `AoE Teleport` could teleport Revive Beacons and Base Camps into Abyss in Open Beta notes.

#### Environmental Hazards

- Kaiju Water anti-heals for a short duration.
- Explosive barrels have delayed explosion and apply anti-heal.
- Trees have brush-like stealth interaction in earlier Steam Next Fest notes and remain relevant to stealth/vision.
- Breakable ice and winter map objects are post-November 2024 additions, not November launch baseline.

### Capturable / Usable Objectives

#### Base Camps / Bonfires

- The 2024-11-15 notes use both bonfire and Base Camp terminology: the quest vendor follows the team until they accept a quest, capture a bonfire, or the game reaches Day 2; after that it appears above the team's Base Camp or nearest uncaptured one.
- Base Camps support team ownership, quest routing, and utility interactions.
- Armor repair at Basecamp applies `Vulnerable`, same as brewing Vive. For implementation, treat `Vulnerable` as a dangerous channel-state keyword. Local design targets 2x incoming damage, but the public patch-note text only confirms the keyword, not the multiplier.
- Brewing Vive also applies `Vulnerable`.
- Base Camps and Revive Beacons are teleportable by `AoE Teleport` in the Open Beta baseline. This is a major systemic clue: these objectives must be movable actors or at least relocatable gameplay anchors, not fixed map-only widgets.

Unverified exact values: public November notes in the retrieved archive do not state Base Camp vision radius, healing radius, capture duration, or whether summons can capture. Existing Breachborne design should keep those values data-driven and expose them in editor/data tables.

#### Resurrection / Revive Beacons

- Revive Beacons existed in the November baseline. The Open Beta notes mention updated visuals and smoother animations.
- `AoE Teleport` could move Revive Beacons into Abyss.
- `Two-Way Teleporter` patch note tip explicitly says to teleport resurrection beacons, proving beacon relocation was an intended/known tactic.
- The December 18 event changed regular respawn beacons: under event resurgence rules, regular beacons began ruined and became regular again when automatic respawns ended. That is a later experimental rule, not the November launch baseline.
- System contract:
  - Interactable resurrection objective.
  - Emits high-visibility/high-audio risk.
  - One-time or state-limited per beacon in normal BR rules.
  - Needs map icon state: available, ruined/disabled, consumed.

#### Quest Vendor

- Open Beta introduced Quests as a strategic early-game objective system.
- Vendor appears shortly after drop and is visible/usable only by the team.
- Vendor teleports after the team every `15s` until a quest is accepted, a bonfire is captured, or Day 2 starts.
- After that, vendor appears above the team's Base Camp, or nearest uncaptured Base Camp if the team owns none.
- Each quest offered can be attempted once per game.
- Vendor disappears after Day 4.
- Launch quest pool and rewards:
  - `Brawlers`: kill `3` players. Reward: Megashard.
  - `Farm Creeps`: farm `16` creeps. Reward: `4` Vive Brews plus `1` Hyperscrap per player.
  - `Open Vault`: open any Vault. Reward: `1` Tome of Knowledge plus `1` Hyperscrap per player.
  - `Supply Drop`: spawn and loot a Supply Drop. Reward: `2` green armor plus `1` Hyperscrap per player.
  - `Megaboar`: spawn and hunt a Megaboar. Reward: `2` green armor plus `1` Hyperscrap per player.
  - Supply Drop and Megaboar were mutually exclusive per match in the offered pool.

### World Loot Nodes and Drop Events

#### Vaults

- Vaults were core Open Beta objectives: `Open Vault` was a quest condition, Gold Keys could instantly open Vaults in earlier Steam Next Fest notes, and vaults supplied powers/items.
- Steam Next Fest statistic from 2024-10-22: `3.5 million` vaults were attempted and `70%` were failures. This is not a November Open Beta live stat, but it is the documented patch-history failure rate requested by the prompt.
- October patch notes list Vault Power drop rarity change:
  - Common Powers: `20% -> 15%`.
  - Uncommon Powers: `60% -> 75%`.
  - Rare Powers: `20% -> 10%`.
- November Open Beta notes identify vaults as teammate-efficient content, contrasted with soloable random chests.
- Channel model:
  - Vault opening is a committed interaction.
  - Earlier notes specify vaults are WASD-cancellable `0.5s` after starting.
  - Gold Keys can bypass the channel by opening Vaults instantly.
- Treat a "failed vault attempt" as a team attempted an opening but did not complete/extract reward, not as a random dice failure unless confirmed by telemetry implementation.

#### Random Chests and Armor Chests

- Random Chests spawn in the outskirts of most biomes and contain rewards.
- Design purpose: soloable farming target so squads can split up for efficient biome farming instead of every reward requiring multiple teammates.
- Gold Keys could still be used to open Armor Chests after gaining instant Vault open functionality.

#### Supply Drops

- Supply Drop is a quest. The team must spawn and loot it.
- Launch reward: `2` green armor plus `1` Hyperscrap per player.
- It is mutually exclusive with Megaboar as a quest option per match.

#### Creeps and Minion Camps

- Creeps are early-game PvE economy.
- Open Beta quest: farm `16` creeps.
- Rewards in the broader ecosystem: XP, shards/scraps, gold, and equipment progression.
- Creeps ignore players sneaking around them unless very close.
- Several minions had loot/XP retuned for density/danger in the Open Beta launch notes.
- Minion art and color states were updated; Town Guards change color when attacking.

#### Bosses / Major Objectives

- `Megaboar`: quest-spawned/hunted boss objective, mutually exclusive with Supply Drop in launch quest set.
- `Wavemaker Boss`: projectile orbs destructible by melee attacks.
- Major Objectives and Gold Zones do not spawn in the initial death circle.
- Chaos Steppes received a new Boss arena in the Open Beta notes.

#### Gold Zones / Hot Zones

- BR games start with an initial Death Circle in Open Beta.
- Gold Zones and Major Objectives are excluded from the initial Death Circle.
- Earlier Steam Next Fest notes: Gold Armor zone can be in any biome, can be on the train, and all Gold Armors in a single biome increased from `10% -> 25%`.

#### Meteors / Crates

- The retrieved November 2024 patch-note archive did not expose a mechanic named Meteor or Crate. Do not implement these as November baseline mechanics unless supported by captured footage or game data.
- If a local design needs automated drops, use the documented Supply Drop / Random Chest / Gold Zone / Vault ecosystem as the canonical late-2024 baseline.

---

## 3. Player Lifecycle State Machine

### State Graph

```text
Alive
  |
  | HP <= 0 by normal combat
  v
Knocked / Wisp
  |
  | allied revive completes
  v
Alive, revived near Wisp / ally rules

Knocked / Wisp
  |
  | bleedout, stomp/execute, lethal ability rule, abyss conversion
  v
Deathbox
  |
  | deathbox revive OR resurrection beacon OR Most Wanted success OR special event resurgence
  v
Alive, respawned/revived with penalties or item loss according to path
```

### Alive to Wisp

- When a Hunter reaches `0 HP` by normal combat, they enter Wisp/knocked state rather than disappearing.
- Wisp is a controllable or slowly movable downed-state entity. It preserves team identity and allows ally/enemy interaction.
- Open Beta notes distinguish deaths that go "straight to Deathbox" under specific rules:
  - Dying while Most Wanted sends the player straight to Deathbox.
  - Certain knock/dunk/abyss cases can bypass Wisp and convert immediately.

### Wisp Mechanics

- Enemy pressure:
  - Standing on / stomping / executing a Wisp accelerates conversion to Deathbox or directly kills it depending action.
  - Executioner / Soul Drain style item effects specifically interact with stomping or executing wisps.
  - Open Beta equipment note: `Executioner` caused stomping or executing wisps to heal over time.
  - December 8 Brall note: `Volcanic Rebuke` cooldown reduction on Wisp execution changed from `100% -> 25%` of max cooldown, while knocking a hunter straight to Deathbox still fully reset it.
- Ally pressure:
  - Allies can revive Wisps through channel/proximity/ability interactions.
  - Earlier notes mention Cremated Wisps and revive-bar decay while not being revived, proving Wisp has a revive progress bar separate from death/bleedout.
  - Wisp revive progress can decay if interrupted.
- Elluna-specific:
  - `Soulpack` passive picks up Wisps.
  - Open Beta changed slow while picking up Wisps from `20% -> 30%` and shield from `250 -> 200`.
  - Wisp displacement by allied abilities is legitimate systemic behavior. Kingpin hook / displacement should be validated per target tags to avoid moving enemies/allies inconsistently.
- Wisp bleedout:
  - December 18 later changed Wisp bleedout time from `30s -> 15s`. Therefore the late-November baseline is `30s` unless another November-only source contradicts it.

### Wisp to Deathbox

- Wisp can die through bleedout, enemy execution/stomp acceleration, abyss, or special hunter/ultimate conversion.
- Deathbox contains the defeated player's post-death recoverable state and becomes the anchor for later revive/loot interactions.
- Most Wanted exception from Open Beta: dying while Most Wanted skips Wisp and goes straight to Deathbox.

### Resurrection Methods

#### Wisp Revive

- Teammates revive during Wisp stage by proximity/channel/ability.
- Revive is interruptible by enemy pressure and decay/bleedout.
- Some abilities/items accelerate or enable Wisp interactions.

#### Deathbox / Box Rez

- Teammates can return a dead player by interacting with the Deathbox.
- Public November patch notes retrieved here do not state exact channel time or item-loss rules. Existing local design targets should use a loud, risky channel and return the player with reduced combat readiness.

#### Resurrection Beacon

- Map objective that revives dead squadmates.
- One-time or state-limited under normal rules.
- Visible/audible enough to be a strategic risk.
- Can be teleported by powers, including `Two-Way Teleporter` and `AoE Teleport` interactions in notes.

#### Most Wanted

- Existing in November Open Beta.
- Core rule: a Most Wanted carrier can revive dead squadmates through survival/kill pressure.
- Open Beta specific notes:
  - Dying while Most Wanted sends the carrier straight to Deathbox.
  - Spike kill credit goes to the player who hit the spike and is not stolen by later damage after spike begins.
  - Last-hit leniency: if the Most Wanted player has an assist within `0.5s` of a kill, they get full kill time reduction instead of assist reduction.

#### Resurgence

- November 15 baseline note: "Welcome to Skylands" Stormshift for newer players introduced Resurgence. During the first `3` days, when a teammate is killed, a timer starts on remaining teammates. When the timer expires, the dead teammate is revived.
- December 18 event rule is a different, broader experiment:
  - For the first Day and Night, players respawn in the air above a living ally after a delay.
  - Player respawns above the ally they are spectating.
  - Respawn timer increases per respawn, individually.
  - At the end of the first night, any player waiting to respawn automatically respawns.
  - Regular respawn beacons begin ruined while these rules are active and become regular beacons again when automatic respawns end.
  - Timer next to minimap indicates automatic respawn time remaining.

#### Resurrection Sickness

- The retrieved November and December patch notes do not expose exact Resurrection Sickness debuffs by name or value.
- Implementation recommendation: make this a data-driven GameplayEffect applied after non-Wisp resurrection paths, with configurable duration and modifiers such as damage dealt, damage taken, healing received, movement speed, and item access lockout.

---

## 4. HUD & Interface Reverse Engineering

### Always-On Combat HUD

- Health bar with updated Open Beta visuals: status icons, broken armor indicator, and damage-taken feedback.
- Armor state: armor can be broken, repaired, upgraded, and later ascended.
- Mana/resource indicators: notes mention mana gain feedback from consumables.
- Ability bar:
  - LMB, RMB, Shift, Q, R cooldowns.
  - Cursor cooldown indicator rate-limited and optionally disabled.
  - Ability cast ranges shown on hover.
  - `Alt + ability` levels abilities.
- Item / equipment progression:
  - Two equipment items plus boots are directly upgradeable with `Alt + 1-3`.
  - Equipment could reach Red Tier again in Open Beta.
  - Shards vacuum into the player and upgrade equipment.
  - Purple/Gold shard costs increased slightly in Open Beta.
- Powers:
  - Powers are separate from ability haste after Open Beta; `Power Haste` uniquely affects Power cooldown reduction.
  - Powers have rarity and active/passive behavior. The October list included Common, Uncommon, Rare, Exotic, and non-Power Exotic categories.

### Map / Mega Map / Minimap

- Opening map shows added game-state information in Open Beta.
- Minimap/mega-map later gained audio pulses on December 18; this is not November launch baseline.
- Off-screen indicators show direction and distance for important events.
- Vision cone minimap rendering existed and had bug fixes in October notes.
- Revive beacons have updated visuals/animations in Open Beta.
- Death/respawn timer:
  - For December event resurgence, a timer appears next to minimap.
  - For November newcomer Resurgence, remaining teammates receive a timer when a teammate is killed during the first three days.

### Day/Night and Storm Tracker

- Match is divided into Days and Nights; notes reference first `3` days for newcomer Resurgence and first Day/Night for December event rules.
- Storm/death circle:
  - Open Beta starts BR games with an initial Death Circle.
  - Gold Zones and Major Objectives do not spawn in the initial Death Circle.
  - Squad Day 1 circle duration lowered by `60s`; subsequent days lowered slightly.
  - Week 1 increased initial circle radius by `+10%` while keeping the same close time, making the first close feel faster after it begins.
- HUD must show:
  - Current day/night.
  - Circle phase and closure timing.
  - Whether automatic respawn rules are currently active.
  - Whether beacons are available, ruined, or consumed.

### Inventory and Economy Interface

- Inventory/progression reset for Open Beta.
- Equipment:
  - Equipment can go to Red Tier.
  - Shards / Hyperscraps upgrade equipment.
  - Megashard is a quest reward.
- Consumables:
  - Vive Brews are quest rewards and crafted/brewed at Basecamp.
  - Armor repair at Basecamp is a channel and applies Vulnerable.
  - Brewing Vive applies Vulnerable.
- Quest UI:
  - Quest vendor offers shared team quests.
  - Quest state needs team visibility and per-game attempt tracking.
- Shop / item UI:
  - Item tooltips were restyled in Open Beta.
  - Tooltip data must distinguish Ability Haste from Power Haste.

### Status Keywords

- `Vulnerable`: applied while repairing armor at Basecamp and while brewing Vive. Local design should use `2x` incoming damage if matching existing Breachborne reference, but the patch-note source retrieved only confirms the keyword, not the multiplier.
- `Cremated`: affects Wisp revive-bar decay and stomp bugfixes.
- `Anti-heal`: applied by Kaiju Water and delayed explosive barrels.
- `Stealth` / Brush: hidden unless close; stealth warnings and detection range later changed.
- `Invulnerable`: states like Freeze Ult interact with status readability.
- `Grounded`: e.g. Beebo Ice Zeeps in later notes.
- `Soulbound`: item/power death-drop behavior and spike immunity interactions.

---

## 5. Historical Crawl Verification

### November 15 Open Beta Baseline

- Open Beta introduced/refined:
  - AFK detection that kills/unrespawns AFK players.
  - `Run It Back` team-screen party reinvite.
  - New-player Resurgence in Welcome to Skylands: first `3` days auto revive via teammate timer.
  - Quest system and quest vendor.
  - Initial Death Circle at BR start.
  - Gold Zones and Major Objectives excluded from initial Death Circle.
  - Squad Day 1 circle duration lowered by `60s`; later days lowered slightly.
  - Most Wanted death goes straight to Deathbox.
  - Most Wanted spike kill-credit protection and `0.5s` assist leniency.
  - Hidden `3%` damage reduction and `5%` healing reduction removed from all hunters; compensated by `+30` base Health plus scaling Health.
  - Basecamp armor repair applies Vulnerable, same as brewing Vive.
  - Random Chests added to outskirts of most biomes.
  - Creeps ignore sneaking players unless very close.
  - Vault quest and vault/item economy remained central.
  - Gold Keys can instantly open Vaults from the earlier October patch state.

### Week 1 Patch, November 26

- Ranked:
  - Legend players above `500 RP` reset to `500 RP`.
  - Squads Master+ limited to duo premades at most.
  - Duos Master+ solo-only was reverted due to feedback.
  - Custom games opened temporarily for Legend+ until 2025-01-15 with at least `8` players required.
- Death Circle:
  - Initial circle radius increased by `+10%`.
  - First circle close duration unchanged, so closure feels faster after it starts.
- Hunter balance:
  - Ghost LMB damage increased by about `5%`.
  - Ghost Shift cooldown `12/9/8 -> 11/8/8`.
  - Hudson LMB aim laser fixed to show range.
  - Hudson unempowered Minigun projectile range `1500 -> 1400`.
  - Hudson Hover Jets duration `2/2/2.5 -> 1.4/1.4/1.75`.
  - Jin RMB cast warmup `0.3 -> 0.1`.
  - Shiv LMB base range `800 -> 850`; empowered range `925 -> 950`.
- Equipment:
  - Vampiric Axe base Omnivamp `7.5% -> 8.5%`; tooltip rounds to `9%`.
  - Swiftblade empowered self-heal `50 -> 60`.
  - Rampage AP at max stacks `50 -> 40`.
- Powers:
  - Two-Way Teleporter cooldown `100 -> 70`; minimum reactivation delay `10 -> 8`.
  - Bungee Shot cooldown `20 -> 16`; shot warmup `0.2 -> 0`.

### Week 2 / Late Beta Deltas

The public archive does not label a "Week 2 Open Beta Patch Notes" article. The next explicit live balance patch found here is 2024-12-08. The next major systems patch is 2024-12-18.

December 8 hotfix:

- Hudson:
  - Hover Jets charges `2 -> 1`.
  - Hover Jets cooldown `13/10 -> 11/8`.
  - Minigun base range `1400 -> 1300`.
  - Spin Barrels range `1200 -> 1150`.
  - Base damage `30 -> 27`.
  - AP ratio `8% -> 5%`.
  - Barbed Wire activation time `0.6 -> 1`.
  - Barbed Wire activation range `1400 -> 900`.
- Brall:
  - Blazing Spirit empowered duration `6 -> 8`.
  - Volcanic Rebuke cooldown reduction on Wisp execution `100% -> 25%`.
  - Knock-to-Deathbox still fully resets Volcanic Rebuke.
- Power User no longer Soulbounds Exotic Powers.
- Mana Cloak cost `10 -> 30` mana per second.
- Power Shuffler disabled.

December 18 systems experiment:

- Broader Resurgence Revival Rules active for first Day and Night.
- Respawn above spectated living ally after increasing per-player delay.
- End of first night forces pending respawns.
- Regular respawn beacons start ruined while auto-respawn rules are active.
- Timer appears next to minimap for automatic respawn.
- Ascending Armor:
  - Damage / knock / assist against enemy hunters upgrades armor.
  - Thresholds: `2000` to Green, `5000` to Blue, `15000` to Gold, `20000` to Red.
  - Level-up grants no durability.
- Wisp bleedout `30s -> 15s`.
- Audio pulses added to minimap/mega-map for enemy combat/loud sounds, excluding footsteps.

### Official Power Catalog

Source: Steam app `1283700`, official Steam news item `[10.07.2024] Steam Next Fest Fireside Chat & Patch Notes`, dated 2024-10-07. The note says the team was listing all powers by rarity because many powers had been reintroduced or had rarity, icon, or name changes. This catalog is therefore a verified October 2024 snapshot, not necessarily a final February 2026 tooltip-complete database.

Classification below is implementation-facing:

- `Active` means likely press/use from a power slot.
- `Passive` means likely always-on, triggered, or stat/rule modifying.
- `Uncertain` means the name is verified, but the active/passive behavior needs tooltip, footage, or data verification before implementation.

| Power | Rarity in source | Type | Notes / aliases |
|---|---:|---:|---|
| Icarus Glide | Exotic | Active | Marked `NEW` in source. |
| Wisp Vacuum | Exotic | Active | Marked `NEW` in source. |
| Abyssal Eye | Exotic | Passive | Formerly `Kaiju Eye`. |
| Abyssal Fang | Exotic | Passive | Formerly `Kaiju Scale`. |
| Abyssal Scale | Exotic | Passive | Formerly `Kaiju Heart`. |
| Armor Shredder | Exotic | Passive | Formerly `Radioactive`. |
| Blinkstone | Exotic | Active | Formerly `Veil Piercer`. |
| Delicate Disguise | Exotic | Active | Behavior inferred from name. |
| Fate Rewinder | Exotic | Active | Formerly `Be Kind Rewind`. |
| Guardian Archangel | Exotic | Uncertain | Likely revive/save oriented; verify tooltip. |
| Mega Cluster Nuke | Exotic | Active | Nuke-style activation inferred. |
| Power Shuffler | Exotic | Active | Later disabled in December 2024 notes. |
| Replicate Team | Exotic | Active | Behavior inferred from name. |
| Team Frog Jump | Exotic | Active | Formerly `Frog Jump`. |
| Abyss Specialist | Rare | Passive | Behavior inferred from name. |
| Air Blast | Rare | Active | Behavior inferred from name. |
| Anti-Mobility Field | Rare | Active | Field placement / activation inferred. |
| Berry Eater | Rare | Passive | Behavior inferred from name. |
| Brushwhacker | Rare | Passive | Behavior inferred from name. |
| Bungee Shot | Rare | Active | Week 1 patch changed cooldown and shot warmup. |
| Chonker Cannon | Rare | Active | Placement/use action referenced in cancellation notes. |
| Delicate X-Ray Goggles | Rare | Passive | Vision/reveal behavior inferred. |
| Fixer | Rare | Active | Formerly `Multitool`. |
| Glider Jet | Rare | Active | Behavior inferred from name. |
| Grappling Hook | Rare | Active | Behavior inferred from name. |
| Guardian Angel | Rare | Active | Likely targeted protection/revive utility; verify exact tooltip. |
| Health Packs | Rare | Active | Consumable/deployable heal behavior inferred. |
| Hover Wings | Rare | Passive | Source says mass is slightly increased when using these. |
| Little Cook | Rare | Active | Cooking/food utility inferred. |
| Robber Raccoon | Rare | Active | Exact behavior needs tooltip; likely theft/loot utility. |
| Sand Wall | Rare | Active | Source says it was buffed. |
| Stealth Cloak | Rare | Active | Cloak activation inferred. |
| Tactical Nuke | Rare | Active | Nuke-style activation inferred. |
| The Chonker | Rare | Passive | Mass/body-size style effect inferred. |
| Touch of Life | Rare | Uncertain | Could be active support or passive trigger. |
| Tree Prison | Rare | Active | Placement/control effect inferred. |
| Two Way Teleporter | Rare | Active | Week 1 patch changed cooldown and reactivation delay. |
| Bloodstone | Uncommon | Passive | Behavior inferred from name. |
| Brush Boar | Uncommon | Active | Summon/utility behavior inferred. |
| Burning Core | Uncommon | Passive | Behavior inferred from name. |
| Creep Cloner | Uncommon | Active | Source says input was reworked: press to capture, press again to release. |
| Delicate Barrier | Uncommon | Active | Barrier activation inferred. |
| Delicate Mana Battery | Uncommon | Passive | Mana storage/stat behavior inferred. |
| Emergency Platform | Uncommon | Active | Platform deploy inferred. |
| Fishing Pole | Uncommon | Active | Behavior inferred from name. |
| Forestwalking | Uncommon | Passive | Formerly `Forestphasing`; tree/brush hiding interactions referenced. |
| Friend of the Forest | Uncommon | Passive | Behavior inferred from name. |
| Ghillie Suit | Uncommon | Passive | Stealth/brush behavior inferred. |
| Green Thumb | Uncommon | Passive | Plant/brush utility inferred. |
| Homing Missile | Uncommon | Active | Behavior inferred from name. |
| Instant Vive | Uncommon | Active | Formerly `Water Bottle`. |
| Iron Fist | Uncommon | Active | Source says punching a tree refunds half cooldown. |
| Radar Jammer | Uncommon | Active | Behavior inferred from name. |
| Mana Cloud | Uncommon | Active | Cloud deploy inferred. |
| Money Tree Seeds | Uncommon | Active | Seed placement/growth inferred. |
| Mortar | Uncommon | Active | Behavior inferred from name. |
| Rally | Uncommon | Active | Team buff activation inferred. |
| Raven Spy | Uncommon | Active | Rework of `Tree Drone`. |
| Regenerative Armor | Uncommon | Passive | Behavior inferred from name. |
| Remote Laser | Uncommon | Active | Behavior inferred from name. |
| Rescue Grenade | Uncommon | Active | Behavior inferred from name. |
| Scuba Gear | Uncommon | Passive | Source says it works in Ice Water / Spicy Water. |
| Throwing Axe | Uncommon | Active | Formerly `Timber`. |
| Touch of Mana | Uncommon | Uncertain | Could be active support or passive trigger. |
| Touch of Speed | Uncommon | Uncertain | Could be active support or passive trigger. |
| Tree Costume | Uncommon | Active | Tree disguise/placement inferred. |
| Mana Rune | Common | Passive | Rune stat/effect inferred. |
| Omnivamp Rune | Common | Passive | Rune stat/effect inferred. |
| Ability Haste Rune | Common | Passive | Rune stat/effect inferred. |

The same source listed these as `Non-Power Exotics`, so they should not be modeled as F/G powers without later contrary evidence:

- `Endless Reaper`
- `Glide Boots`
- `Gnarl Heart`
- `Respawn Beacon`
- `Soulstealer`

`Power User` / `Power Master` is also not part of the power catalog. The October note places it under `Tech Blade Upgrades`: `Power User` replaced `Thunderbolt`, granted stacking Power Haste for opening Vaults, and transformed into `Power Master`, whose effect made Powers Soulbound.

### Verification Gaps

These items are referenced by the prompt or local design but are not numerically specified in the retrieved public patch notes:

- Default keybind text for recall, map, inventory, ping wheel, and contextual interact.
- Exact recall limitations in storm, on allies, and at captured camps.
- Base Camp capture duration, vision radius, heal radius, summon capture behavior, and exact interaction channel times.
- Resurrection Beacon channel duration, exact one-time-use rule wording, and exact resurrection sickness values.
- Deathbox revive channel time and exact item-retention / item-loss rules.
- Exact `Vulnerable` multiplier in November public notes.
- Meteor/crate mechanics as a November baseline object class.

For Breachborne implementation, keep each of those as exposed tunables and add capture-footage or packet/game-data evidence before locking exact numbers.
