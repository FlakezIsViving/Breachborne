import unreal


CUES = (
    (
        "/Game/GameplayCues/Hunters/Ghost/GC_Hunter_Ghost_Q_Telegraph",
        "/Script/Breachborne.BBGCN_GhostQTelegraph",
    ),
    (
        "/Game/GameplayCues/Hunters/Ghost/GC_Hunter_Ghost_Q_Fire",
        "/Script/Breachborne.BBGCN_GhostQFire",
    ),
)

SYSTEMS = (
    "NS_GhostRailGunTelegraph",
    "NS_GhostRailGunLaser",
)


def create_or_validate_cue(asset_path, parent_class_path):
    parent_class = unreal.load_class(None, parent_class_path)
    if not parent_class:
        raise RuntimeError(
            f"Native cue class is unavailable: {parent_class_path}. Build BreachborneEditor first."
        )

    existing_asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if existing_asset:
        if not isinstance(existing_asset, unreal.Blueprint):
            raise RuntimeError(f"Expected a Blueprint GameplayCue at {asset_path}")

        existing_parent = existing_asset.get_editor_property("parent_class")
        if existing_parent != parent_class:
            raise RuntimeError(
                f"GameplayCue {asset_path} does not derive from {parent_class_path}"
            )
        unreal.log(f"BB_GHOST_Q_VFX|CueValid asset={asset_path}")
        return existing_asset

    package_path, asset_name = asset_path.rsplit("/", 1)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, unreal.Blueprint, factory
    )
    if not asset:
        raise RuntimeError(f"Failed to create GameplayCue Blueprint {asset_path}")

    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    unreal.log(f"BB_GHOST_Q_VFX|CueCreated asset={asset_path}")
    return asset


def normalize_system_location(asset_name):
    canonical_path = f"/Game/GameplayCues/Hunters/Ghost/FX/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(canonical_path):
        return canonical_path

    for candidate_path in (
        f"/Game/FX/{asset_name}",
        f"/Game/Hunters/Ghost/FX/{asset_name}",
    ):
        if unreal.EditorAssetLibrary.does_asset_exist(candidate_path):
            if not unreal.EditorAssetLibrary.rename_asset(candidate_path, canonical_path):
                raise RuntimeError(
                    f"Failed to move Niagara system {candidate_path} to {canonical_path}"
                )
            unreal.log(
                f"BB_GHOST_Q_VFX|SystemMoved from={candidate_path} to={canonical_path}"
            )
            return canonical_path

    return None


def main():
    missing_systems = [name for name in SYSTEMS if not normalize_system_location(name)]
    for cue_asset_path, cue_parent_class_path in CUES:
        create_or_validate_cue(cue_asset_path, cue_parent_class_path)

    unreal.log("BB_GHOST_Q_VFX|CUES_READY")
    if missing_systems:
        raise RuntimeError(
            "GameplayCues are ready, but LUDUS did not save these Niagara systems: "
            + ", ".join(missing_systems)
        )

    unreal.log("BB_GHOST_Q_VFX|READY")


main()
