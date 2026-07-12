# VFX Foundation Contract

This is the locked engineering/content contract for the July 31 playtest. It does not claim that
the Niagara master assets are authored or visually accepted.

## Content Roots

- GameplayCue scan root: `/Game/GameplayCues`
- Existing cue compatibility roots: `/Game/Hunters` and `/Game/Powers`
- Master systems: `/Game/GameplayCues/Templates`
- Hunter cue assets: `/Game/GameplayCues/Hunters/<Hunter>`
- Power cue assets: `/Game/GameplayCues/Powers/<Power>`
- Per-hunter visual sets: `/Game/Hunters/<Hunter>/VIS_<Hunter>`

`Config/DefaultGame.ini` must keep all three roots in `GameplayCueNotifyPaths` through the July 31
playtest. New cue assets go under `/Game/GameplayCues`; existing Hudson cues remain under
`/Game/Hunters/Hudson/Cues` until they can be moved and redirectors fixed in-editor.

## Master Templates

| Type | Intended master asset | Primitive fallback |
|---|---|---|
| ProjectileBody | `NS_BB_ProjectileBody` | Colored sphere/capsule/cone mesh attached to projectile |
| ProjectileTrail | `NS_BB_ProjectileTrail` | Thin replicated beam segments or no trail |
| BeamTether | `NS_BB_BeamTether` | `ABBPrimitiveBeamActor` cylinder |
| GroundTelegraph | `NS_BB_GroundTelegraph` | Thin cylinder/disc or range debug circle |
| PersistentGroundZone | `NS_BB_PersistentGroundZone` | Flat colored cylinder matching gameplay radius |
| ConeWedge | `NS_BB_ConeWedge` | Oriented cone mesh matching gameplay direction |
| ImpactBurst | `NS_BB_ImpactBurst` | Short sphere/line flash |
| CharacterAura | `NS_BB_CharacterAura` | Small colored mesh/status marker |

Each cue entry in `UBBAbilityVisualSet` identifies one `EBBVFXTemplateType`, concrete assets,
parameter defaults, and an `FBBVisualBudget`.

## Parameter Contract

Niagara parameters use the exact `User.` names below. GameplayCue Blueprints read cue parameters,
the owning visual-set palette, and `FBBVFXParameterDefaults`, then write:

| Niagara parameter | Meaning |
|---|---|
| `User.PrimaryColor` | Hunter identity color |
| `User.SecondaryColor` | Hunter supporting color |
| `User.TeamRelationColor` | Friendly/enemy rim color |
| `User.Radius` | Actual gameplay radius in Unreal units |
| `User.Length` | Actual gameplay/cosmetic length in Unreal units |
| `User.Width` | Actual collision/readability width in Unreal units |
| `User.Direction` | Normalized cast/travel direction |
| `User.Lifetime` | Bounded visual lifetime in seconds |
| `User.Progress` | Normalized warning/channel progress, 0 through 1 |
| `User.Intensity` | Scalar brightness/density control |
| `User.bEmpowered` | Enables the empowered shape/brightness variant |
| `User.bEnemy` | Enables enemy pulse/dash treatment |

Parameter precedence is runtime gameplay geometry, then cue-entry defaults, then conservative
template defaults. Visuals must never increase the represented gameplay radius or width merely
for style. Ground-target cast range indicators are separate from effect footprint geometry.

## Locked Palettes

Colors are sRGB authoring values. Team relation remains a separate rim/pulse channel.

| Hunter | Primary | Secondary | Empowered/accent |
|---|---|---|---|
| Ghost | `#22E6B8` | `#E8FFFF` | `#FFFFFF` |
| Eluna | `#DCEBFF` | `#6EC8FF` | `#FFF2B3` |
| Kingpin | `#E53B2F` | `#FFB020` | `#FFF0D2` |
| Hudson | `#FF8A2A` | `#4FA3C7` | `#FFD166` |
| Crysta | `#22D3EE` | `#F044D2` | `#FFE66D` |
| Void | `#8B5CF6` | `#59F08E` | `#FF4FD8` |

- Friendly relation: `#4CC9F0`, solid rim.
- Enemy relation: `#FF4D5A`, pulsing or dashed rim.
- Neutral range/aim indicator: `#B8BEC7`.
- Effects cannot rely on hue alone; enemy/friendly treatments also differ by pulse/dash behavior.

## Low/Medium Budgets

| Template | Max active | Max draw distance | Estimated particles per instance | Dynamic lights |
|---|---:|---:|---:|---:|
| ProjectileBody | 48 | 3000 | 12 | 0 |
| ProjectileTrail | 32 | 3000 | 24 | 0 |
| BeamTether | 16 | 4000 | 24 | 0 |
| GroundTelegraph | 24 | 4500 | 8 | 0 |
| PersistentGroundZone | 12 | 3500 | 48 | 0 |
| ConeWedge | 16 | 3000 | 16 | 0 |
| ImpactBurst | 48 | 3000 | 32 | 0 |
| CharacterAura | 12 | 2500 | 24 | 0 |

Use unlit meshes, decals, ribbons, and sprites. Persistent systems must cull offscreen, stop on
remove/destroy, and expose a Low variant. No fluids, collision-heavy GPU simulation, shadow-casting
particles, or gameplay-significant dynamic lights.

## Acceptance

1. Warning, active state, and resolution are distinguishable.
2. Visible geometry matches server gameplay geometry.
3. Owner and observer see the same gameplay-relevant sequence.
4. Empowered variants change at least two properties, not only color.
5. Persistent cues have explicit remove and destruction cleanup paths.
6. The primitive fallback remains readable with Niagara disabled or missing.
7. Worst-case overlapping effects remain readable on Low and Medium.
