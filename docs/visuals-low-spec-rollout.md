# Visuals Low-Spec Rollout

This document tracks the content-side work for the gameplay-readable visual pass.
The C++ foundation is in place; `.uasset` authoring still happens in the editor.

## Performance Target

- 60+ FPS on roughly 10-year-old PC hardware at 1080p Low/Medium.
- Prefer readable silhouettes, decals, mesh effects, and short bursts over dense particles.
- Persistent effects need low quality variants, distance culling, instance caps, and explicit stop paths.
- Avoid Niagara fluids, heavy GPU sims, large translucent clouds, and always-on aura spam.

## Renderer Baseline

The project defaults are intentionally low-spec first:

- DX11 / Shader Model 5 is the default Windows RHI baseline.
- Lumen-style GI/reflections, hardware ray tracing, Virtual Shadow Maps, Substrate, and mesh distance fields are disabled by default.
- `DefaultScalability.ini` defines Low/Medium budgets for view distance, shadows, post, textures, effects, foliage, and Niagara quality.
- Author and test gameplay VFX with Low or Medium scalability first; only then add optional High/Epic polish.

## Data Assets

Create one `UBBAbilityVisualSet` asset per hunter/power:

- `/Game/Hunters/Hudson/VIS_Hudson`
- `/Game/Hunters/Ghost/VIS_Ghost`
- `/Game/Hunters/Eluna/VIS_Eluna`
- `/Game/Hunters/Kingpin/VIS_Kingpin`
- `/Game/Powers/VIS_<PowerName>`

Assign the visual set on each `UBBHunterDefinition`. Existing mesh, anim class,
mesh transform, and montage fields remain fallbacks while assets are migrated.

## GameplayCue Roots

- New shared templates and cue assets: `/Game/GameplayCues`
- Existing hunter cue compatibility root: `/Game/Hunters`
- Existing power cue compatibility root: `/Game/Powers`

All three roots are explicitly scanned in `Config/DefaultGame.ini`. Keep the compatibility roots
until existing assets such as `/Game/Hunters/Hudson/Cues` are moved in-editor and redirectors are
fixed. See `docs/VFX_FOUNDATION.md` for the locked template, palette, parameter, and budget contract.

## Hudson Vertical Slice

Hudson is the first complete visual slice. Author GameplayCue assets for:

- `GameplayCue.Hunter.Hudson.LMB.Fire`
- `GameplayCue.Hunter.Hudson.LMB.Impact`
- `GameplayCue.Hunter.Hudson.LMB.Heal`
- `GameplayCue.Hunter.Hudson.RMB.Start`
- `GameplayCue.Hunter.Hudson.RMB.Loop`
- `GameplayCue.Hunter.Hudson.RMB.End`
- `GameplayCue.Hunter.Hudson.Shift.Start`
- `GameplayCue.Hunter.Hudson.Shift.Loop`
- `GameplayCue.Hunter.Hudson.Shift.Impact`
- `GameplayCue.Hunter.Hudson.Q.Cast`
- `GameplayCue.Hunter.Hudson.Q.Active`
- `GameplayCue.Hunter.Hudson.Q.Tick`
- `GameplayCue.Hunter.Hudson.R.Fire`
- `GameplayCue.Hunter.Hudson.R.Tether`
- `GameplayCue.Hunter.Hudson.R.Impact`
- `GameplayCue.Hunter.Hudson.R.Reel`
- `GameplayCue.Hunter.Hudson.Passive.Pulse`

## Primitive Fallbacks

Playtest primitives are real bounded mesh actors or local batched-cue visuals. The legacy
DrawDebug multicast path and its console toggle were removed so disabled debug rendering cannot
consume network RPC budget.
