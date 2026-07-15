# LUDUS VFX Prompt Pack

Prepared for the July 31 playtest. These prompts are an optional authoring route for LUDUS AI 1.0
or a human Niagara artist. They do not prove that an asset was created, integrated, cooked, or
visually accepted. Keep the authored-master count at `0/8` until the assets exist and pass the audit.

Do not request character animations in this pass. Shared animations and differently colored hunter
models are the playtest baseline; these prompts cover ability readability only.

## Workflow

1. Send the project context prompt once at the start of a LUDUS session.
2. Send one master-system prompt at a time. Do not batch all eight until one complete asset works.
3. Confirm the exact Content Browser path, exposed parameters, fixed bounds, scalability, and emitter
   count before continuing.
4. After all masters exist, send one hunter variant prompt at a time.
5. Send the review prompt for every generated asset before integrating it with a GameplayCue.
6. If LUDUS cannot edit the project directly, request exact editor steps and perform them manually.
   Do not accept a text claim that an asset was created.

## Project Context Prompt

```text
We are authoring low-cost cosmetic Niagara VFX in the open Unreal Engine 5.7 Breachborne project.
Do not change C++, Blueprints that own gameplay, collision, damage, healing, movement, targeting,
replication, Gameplay Ability logic, or network RPCs. Niagara is cosmetic only. Do not replicate a
Niagara simulation and do not use Niagara collision for gameplay.

Master systems belong in /Game/GameplayCues/Templates. Hunter variants belong in
/Game/GameplayCues/Hunters/<Hunter>. Use unlit sprites, ribbons, decals, and simple meshes. Use no
fluids, no shadow-casting particles, no gameplay-significant dynamic lights, and no heavy collision
or GPU simulation. Prefer CPU emitters for these small bounded counts. Every looping system must
stop cleanly when its component/cue is removed and must have conservative fixed bounds.

Expose these exact Niagara user parameters when relevant:
User.PrimaryColor (Linear Color)
User.SecondaryColor (Linear Color)
User.TeamRelationColor (Linear Color)
User.Radius (float, Unreal units)
User.Length (float, Unreal units)
User.Width (float, Unreal units)
User.Direction (Vector3, normalized)
User.Lifetime (float, seconds)
User.Progress (float, clamped 0..1)
User.Intensity (float)
User.bEmpowered (bool)
User.bEnemy (bool)

Runtime gameplay geometry always overrides cosmetic defaults. Never enlarge Radius, Width, or
Length for style. Friendly boundaries are solid; enemy boundaries also pulse or dash so relation is
not communicated by hue alone. Empowered variants must change at least two properties, such as
shape plus density or size plus brightness, not only color.

Before making changes, repeat the requested asset path, system type, active-instance cap, draw
distance, particle budget, loop/lifetime behavior, and parameters. After making changes, report the
actual asset path, emitters, exposed parameters, fixed bounds, scalability settings, and anything I
must complete manually. If direct project editing is unavailable, say so and provide exact UE 5.7
editor steps instead of claiming completion.
```

## Master System Prompts

### 1. Projectile Body

```text
Create /Game/GameplayCues/Templates/NS_BB_ProjectileBody as a compact attached projectile-body
master. Use one low-count unlit mesh or sprite shell plus an optional small core. It must orient to
User.Direction and scale from User.Width without changing collision. Use PrimaryColor for the body,
SecondaryColor for the core, TeamRelationColor for a restrained rim, Intensity for emissive level,
and bEmpowered for a second shell plus increased density. Expose all project-context parameters that
apply. Budget: at most 12 particles per instance, 48 active instances, 3000-unit draw distance, zero
dynamic lights. It loops only while its owning projectile exists and stops immediately on removal.
Use conservative fixed bounds suitable for the maximum configured Width.
```

### 2. Projectile Trail

```text
Create /Game/GameplayCues/Templates/NS_BB_ProjectileTrail as an attached directional ribbon/trail
master. The trail follows owner movement, aligns to User.Direction, uses User.Width and User.Length,
and fades within User.Lifetime after emission stops. PrimaryColor drives the ribbon; SecondaryColor
drives sparse accents. bEnemy adds a dashed/pulsing edge and bEmpowered adds a brighter secondary
ribbon plus higher spawn spacing, not just a color change. Budget: at most 24 live particles per
instance, 32 active instances, 3000-unit draw distance, zero lights and zero collision. It must not
leave ribbon segments after owner destruction or cue removal.
```

### 3. Beam/Tether

```text
Create /Game/GameplayCues/Templates/NS_BB_BeamTether as a straight beam/tether master whose visible
length and width are exactly User.Length and User.Width and whose axis follows User.Direction. Use a
thin unlit core and sparse directional particles; no volumetric fog. User.Progress may increase core
intensity without changing geometry. bEnemy adds a moving dashed rim; bEmpowered adds a second core
and higher pulse rate. Budget: at most 24 particles, 16 active instances, 4000-unit draw distance,
zero lights. Support persistent looping with immediate cleanup when the cue is removed. Provide
fixed bounds covering the maximum User.Length without making the beam thicker than User.Width.
```

### 4. Ground Telegraph

```text
Create /Game/GameplayCues/Templates/NS_BB_GroundTelegraph as a thin world-space ground warning ring.
Its outer radius must be exactly User.Radius. Use User.Progress for a fill/sweep or inward countdown
that never implies a different radius. PrimaryColor is the inner identity line and
TeamRelationColor is the boundary. bEnemy makes the boundary dashed and pulsing; friendly remains
solid. bEmpowered adds a second inner ring and greater brightness. Budget: at most 8 particles, 24
active instances, 4500-unit draw distance, zero lights. Support bounded one-shot warnings and
persistent warnings, both with explicit removal cleanup and flat conservative fixed bounds.
```

### 5. Persistent Ground Zone

```text
Create /Game/GameplayCues/Templates/NS_BB_PersistentGroundZone as a flat persistent area effect.
The boundary radius must equal User.Radius. Use a sparse boundary, low-opacity interior motion, and
occasional readable pulses; do not create an opaque screen-filling dome. User.Progress drives timer
motion, not radius. bEnemy adds a dashed/pulsing danger edge. bEmpowered adds a secondary rim and
changes interior motion density. Budget: at most 48 particles, 12 active instances, 3500-unit draw
distance, zero lights. Provide Low and Medium scalability variants, distance culling, fixed bounds,
and immediate stop/cleanup on cue removal or owner destruction.
```

### 6. Cone/Wedge

```text
Create /Game/GameplayCues/Templates/NS_BB_ConeWedge as a flat directional wedge master. The tip must
point along User.Direction. User.Length controls reach and User.Width controls the far-edge width;
do not rotate the cone upward. Use an unlit translucent mesh or sparse edge ribbons. bEnemy adds a
dashed/pulsing boundary and bEmpowered adds a second shell plus increased size only when runtime
passes larger gameplay dimensions. Budget: at most 16 particles, 16 active instances, 3000-unit
draw distance, zero lights and collision. It must be readable from a top-down camera and clean up at
the end of its bounded User.Lifetime.
```

### 7. Impact Burst

```text
Create /Game/GameplayCues/Templates/NS_BB_ImpactBurst as a short one-shot impact/detonation master.
Use a compact flash, directional shards/sparks aligned to User.Direction, and an optional ring whose
maximum radius is User.Radius. PrimaryColor and SecondaryColor define identity; TeamRelationColor is
only a brief rim. bEmpowered changes shard count and adds a second shock ring. Budget: at most 32
particles, 48 active instances, 3000-unit draw distance, zero lights and collision. Lifetime must be
bounded, normally 0.15 to 0.6 seconds, and the system must self-complete without pooling stale state.
```

### 8. Character Aura

```text
Create /Game/GameplayCues/Templates/NS_BB_CharacterAura as a restrained status/readiness marker that
does not cover the character model or resemble a large floor zone. Use a small orbit, hand/weapon
glow, overhead mote, or narrow silhouette rim driven by User.Radius and User.Intensity. bEnemy adds
a pulsing/dashed treatment and bEmpowered adds a completed outer orbit plus higher density. Budget:
at most 24 particles, 12 active instances, 2500-unit draw distance, zero lights. It loops only while
the status cue exists and must stop immediately on status removal, death, or owner destruction.
```

## Hunter Variant Prompts

Use these only after the relevant masters exist. Recommended variant naming is
`NS_BB_<Hunter>_<Input>_<Stage>`. Do not replace the primitive fallback until the variant is cooked
and accepted. GameplayCue tags are declared in `BBGameplayTags.cpp`; wiring them is a separate
integration step.

### Ghost

```text
Create Ghost Niagara variants under /Game/GameplayCues/Hunters/Ghost using the existing masters.
Palette: Primary #22E6B8, Secondary #E8FFFF, empowered #FFFFFF. LMB uses a compact rifle tracer,
muzzle flash, and directional impact sparks. RMB uses a readable spiked-grenade body, thin flight
trail, and expanding detonation ring. Shift uses a low horizontal ground streak and brief departure
afterimage. Q uses a neutral-grey thin warning followed by a fast bright piercing beam and separate
impact bursts on each hit. R uses a clear exact-radius warning, descending marker, and orange active
floor zone with a pulsing relation boundary. Passive is one brief reset pulse. Keep each stage as a
separate bounded asset where its GameplayCue lifecycle differs.
```

### Eluna

```text
Create Eluna variants under /Game/GameplayCues/Hunters/Eluna. Palette: Primary #DCEBFF, Secondary
#6EC8FF, empowered #FFF2B3. LMB is a crescent projectile; repeated-fire stacks increase brightness
and density without increasing collision width. RMB is a slow dark sticky orb with a heavy trail,
then an exact-radius delayed warning and vertical binding bands for root. Shift is a pale lunar
ribbon with an extra friendly-colored refund pulse. Q is an exact-radius healing disc with rhythmic
pulses, a readable travel state, and one final burst. R is a narrow translucent beam with particles
moving toward the target, Progress-driven channel intensity, a success/revive burst, and immediate
cancel cleanup. Passive uses a small floating healing/status marker and a restrained carried-wisp
state, never a large floor bubble.
```

### Kingpin

```text
Create Kingpin variants under /Game/GameplayCues/Hunters/Kingpin. Palette: Primary #E53B2F,
Secondary #FFB020, empowered #FFF0D2. LMB is a broad melee swipe with one chain arc from primary to
secondary target. RMB uses a visible hook head, persistent heavy tether, latch flash, and one pull
streak. Shift is a heavy forward wedge, dust trail, and bounded shoulder-impact burst. Q is a short
exact cone telegraph, overhead strike, ground shock, and clear stun marker. R has two individually
readable shotgun flashes/cones and a small anti-heal aura on affected targets. Passive is one small
response pulse when CC reduces cooldowns. Favor heavy silhouettes and low particle counts.
```

### Hudson

```text
Create Hudson variants under /Game/GameplayCues/Hunters/Hudson without deleting the existing 17 cue
assets. Palette: Primary #FF8A2A, Secondary #4FA3C7, empowered #FFD166. LMB uses compact muzzle,
tracer, and metallic impact effects; spun-up adds density and heat while heal feedback stays subtle.
RMB has a one-second Progress-driven spin-up ring, bounded active heat, and wind-down steam. Shift
uses twin jet trails and one collision burst. Q keeps the physical wire readable and adds a striped
exact-boundary danger pulse. R separates fire, latch, tether, reel, and execute stages. Passive uses
small scrap particles moving toward Hudson. Sustained LMB must stay under the project cue and
particle budgets; do not add a Niagara or GameplayCue network event per cosmetic sub-particle.
```

### Crysta

```text
Create Crysta variants under /Game/GameplayCues/Hunters/Crysta. Palette: Primary #22D3EE, Secondary
#F044D2, empowered #FFE66D. LMB is a sharp cyan crystal; empowered shows two aligned front-to-back
projectiles with gold/white cores, not a spread. Passive is a small attached/orbiting mark that
shatters and disappears on detonation. RMB has distinct cyan outbound and orange/magenta return
trails that can bend, with a return impact streak pointing toward Crysta. Shift is a sharp gold dash
trail plus a restrained empowered-LMB hand/weapon aura. Q uses the exact-radius ground circle,
accumulating crystal growth, timer motion, inward collapse, and one burst. R keeps neutral-grey
full-length lane indicators separate from alternating magenta beams; close convergence continues
past the aim point by remaining range to form a long X, far convergence remains a V, and the final
salvo shows both beams. Do not make beam origin or endpoint logic inside Niagara.
```

### Void

```text
Create Void variants under /Game/GameplayCues/Hunters/Void. Palette: Primary #8B5CF6, Secondary
#59F08E, empowered #FF4FD8. LMB is a dense violet orb; charged adds a white core and phase trail
without owning wall collision. Passive has three small orbiting motes and a completed empowered
ring. RMB is a moving cone whose tip points along User.Direction; empowered adds a larger runtime-
driven shell and brightness. Shift shows a source ring around Void and a destination ring beneath
the arcing orb, then collapse/reopen bursts at both endpoints. Q is an exact-radius dark puddle with
inward timer ring, tick ripples, and one expiry/recast burst; empowered adds a larger runtime-driven
secondary rim. R uses an exact warning ring, inward spirals, active core, actor pull trails, and one
sharp end implosion. Niagara must not select, move, swap, stun, damage, or detect actors.
```

## Cascade Conversion Fallback

The July 15 content inventory contains no existing Niagara systems. It does contain these usable
Cascade seeds:

- `/Game/StarterContent/Particles/P_Sparks` for `NS_BB_ImpactBurst`.
- `/Game/StarterContent/Particles/P_Explosion` for a second impact/detonation reference.
- `/Game/StarterContent/Particles/P_Steam_Lit` and
  `/Game/Fantastic_Village_Pack/effects/PS_FX_particle_windtrail` for projectile trails.
- `/Game/StarterContent/Particles/P_Ambient_Dust` for a character-aura reference.
- `/Game/StarterContent/Particles/P_Smoke` for a persistent-zone reference.

Source UE 5.7.4 includes Epic's beta `CascadeToNiagaraConverter` plugin. It is disabled by default
and requires an editor restart, so enable it only after saving and closing the current authoring
session. Once enabled, select one or more Cascade particle systems in the Content Browser, right-
click, and choose **Convert To Niagara System**. Engine source confirms that action duplicates the
selected Cascade system and runs Epic's converter; it does not alter the original asset.

Converted output is only a seed. Duplicate/move it to the exact master path, remove unsupported or
expensive modules, add the required `User.*` parameters, set fixed bounds and Low/Medium
scalability, enforce the prompt's emitter/particle/instance budgets, and run the same audit and
manual acceptance. A successful conversion does not by itself advance the authored count.

## Asset Review Prompt

```text
Audit the selected Niagara asset against the Breachborne VFX foundation. Do not modify gameplay.
Report PASS or FAIL for: exact asset path and naming; every required User.* parameter and type;
geometry driven by runtime Radius/Length/Width/Direction; empowered changes at least two visual
properties; friendly solid versus enemy dashed/pulsing treatment; particle count; active-instance
cap; draw distance; fixed bounds; Low and Medium scalability; zero gameplay collision; zero dynamic
lights; bounded one-shot lifetime or explicit looping cleanup; and top-down readability. List every
failure and the exact editor change needed. Do not claim owner/observer, cooking, network, or manual
visual acceptance from an editor-only inspection.
```

## Evidence Required After Authoring

- Run `Scripts/Playtest/AuditVfxFoundation.ps1`; authored masters must advance from `0/8` by exact
  asset path, not by screenshots or LUDUS text output.
- Inspect the cooked client container for every integrated system and GameplayCue.
- Run the matching owner/observer rows in `july-31-manual-vfx-acceptance.md` on Low and Medium.
- Preserve LUDUS prompts, generated settings, asset names, and any manual corrections in the status
  tracker. A generated asset that fails geometry, cleanup, budget, or readability uses the existing
  primitive fallback for the playtest.
