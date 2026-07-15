# Ability Range Indicators

## Playtest Behavior

Range indicators are owner-only primitive visuals. They do not replicate and do not affect
ability targeting, collision, damage, or movement.

- Hover an ability slot to show its maximum cast/travel range around the hunter.
- Hold LMB or RMB after a successful activation to show the cast ring and live aim direction.
- Press Shift, Q, R, F, or G to show the live preview for 0.85 seconds after activation.
- Tactical Nuke keeps its preview visible for the full targeting state and removes it on
  confirm or cancel.
- Targeted areas clamp the target center to cast range. The effect footprint is drawn around
  that center and may extend beyond the cast ring. Void Q intentionally behaves this way.

The HUD draws these with `DrawDebugHelpers`, so the current primitive implementation is for
Editor and Development playtests. Replace it with decals, Niagara, or a dedicated line renderer
before a Shipping build.

## Indicator Modes

| Mode | Hover | Active preview |
|---|---|---|
| Directional | Maximum range circle | Range circle and aim line |
| TargetedArea | Maximum center-cast circle | Cast circle, aim line, and target footprint |
| Movement | Maximum travel circle | Travel circle, direction line, and landing marker |
| SelfCentered | Effect radius | Effect radius |

## Hunter Matrix

All values are Unreal units. Area values are `cast range / effect radius`.

| Hunter | LMB | RMB | Shift | Q | R |
|---|---:|---:|---:|---:|---:|
| Ghost | Dir 1500 | Area 3750 / 400 | Move 600 / 55 | Dir 5000 | Area 5000 / 500 |
| Eluna | Dir 1600 | Area 2400 / 300 | Move 600 or 900 / 55 | Area 2000 / 400 | Dir 1500 |
| Kingpin | Dir 250 | Dir 2500 | Move 960 / 70; dash variants 650 / 55 | Dir 300 | Dir 900 |
| Hudson | Dir 1300 | Dir 1300 | Move 1900 / 105 | Area 900 / 360 | Dir 1800 |
| Crysta | Dir 1800 | Dir 1350 | Move 520 / 55 | Area 1700 / 420 | Area 2300 / 55 |
| Void | Dir 1800 | Dir 1725 | Area 1900 / 430 | Area 1650 / 620 | Area 2300 / 620 |

Empowered Void Shift and Q use their larger radii so the preview never understates the possible
footprint. Ghost R now clamps its actual target center to the documented 5000-unit maximum so
gameplay and the indicator agree.

## Power Matrix

| Power | Indicator |
|---|---:|
| Bungee Shot | Directional 2600 |
| Grappling Hook | Directional 2400 |
| Emergency Platform | Area 260 / 180 |
| Regenerative Armor | Self 120 |
| Tactical Nuke | Area 5200 / 850 |

## Manual Test Matrix

Run these checks in the recorded packaged owner/observer sessions from
`docs/plans/july-31-manual-vfx-acceptance.md`. The current acceptance candidate uses a hidden
dedicated server; do not substitute PIE or infer a PASS from source geometry.

Hover and aiming are deliberately separate interactions. Leave the pointer over a HUD ability
slot only to inspect the owner-centered maximum-range ring; no world target is expected while the
pointer remains on the slot. Then move the pointer back into the world and activate the hotkey to
inspect the short live aim/landing/footprint preview. The player never needs the pointer over the
HUD and world target simultaneously.

1. `RI-01` Select each hunter and hover LMB, RMB, Shift, Q, and R. Confirm exactly one thin grey range
   circle appears around the owning hunter and no circle appears on the other client.
2. `RI-02` Hold LMB and RMB. Confirm the aim line follows the world cursor and disappears on release.
3. `RI-03` Press Shift, Q, and R. Confirm the active preview lasts briefly and does not remain stuck after
   death, respawn, or pawn replacement.
4. `RI-04` For ground areas, aim inside and beyond maximum range. Confirm the center stops at the outer
   cast circle while the footprint remains centered on the clamped point.
5. `RI-05` For Void Q, confirm the 620-unit footprint extends outside the 1650-unit cast circle at maximum
   range.
6. `RI-06` Equip each F/G power and hover both slots. Confirm the correct power metadata follows the slot,
   including after swapping F and G.
7. `RI-07` Start Tactical Nuke targeting. Confirm the cast ring and impact footprint remain until LMB
   confirms or RMB/Escape cancels.
8. `RI-08` Confirm cooldown failures do not create a live firing preview; hover previews should still work.
9. `RI-09` Retest Hudson RMB press/release and Crysta held RMB to ensure the preview lifecycle does not
   interfere with gameplay input release.

Run hunter-specific portions during the three recorded pair sessions. Run F/G powers, Tactical
Nuke, cooldown rejection, death, and pawn-replacement cases during cleanup/stress. A row is not
complete until every hunter or power named by that row has passed.
