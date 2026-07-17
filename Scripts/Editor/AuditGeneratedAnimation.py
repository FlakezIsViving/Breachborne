import os

import unreal


MARKER = "BB_ANIMATION_AUDIT"


def fail(message):
    unreal.log_error(f"{MARKER}|FAIL|{message}")
    raise RuntimeError(message)


def object_path(value):
    return value.get_path_name() if value else "None"


def editor_property(value, name, default=None):
    try:
        return value.get_editor_property(name)
    except Exception:
        return default


def sequence_uses_root_motion(sequence):
    return bool(editor_property(sequence, "enable_root_motion", False))


def montage_referenced_sequences(montage):
    sequences = []
    for slot_track in editor_property(montage, "slot_anim_tracks", []) or []:
        anim_track = editor_property(slot_track, "anim_track")
        for segment in editor_property(anim_track, "anim_segments", []) or []:
            reference = editor_property(segment, "anim_reference")
            if isinstance(reference, unreal.AnimSequence):
                sequences.append(reference)
    return sequences


def main():
    asset_path = os.environ.get("BB_ANIMATION_ASSET", "").strip()
    expected_skeleton_path = os.environ.get("BB_ANIMATION_SKELETON", "").strip()
    expected_class = os.environ.get("BB_ANIMATION_CLASS", "").strip()
    required_slot = os.environ.get("BB_ANIMATION_SLOT", "").strip()
    require_loop = os.environ.get("BB_ANIMATION_REQUIRE_LOOP", "0") == "1"

    if not asset_path or not expected_skeleton_path or not expected_class:
        fail("missing_required_environment")

    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        fail(f"asset_missing path={asset_path}")

    expected_type = {
        "AnimSequence": unreal.AnimSequence,
        "AnimMontage": unreal.AnimMontage,
    }.get(expected_class)
    if not expected_type:
        fail(f"unsupported_expected_class class={expected_class}")
    if not isinstance(asset, expected_type):
        fail(
            f"class_mismatch path={asset_path} expected={expected_class} "
            f"actual={asset.get_class().get_name()}"
        )

    expected_skeleton = unreal.EditorAssetLibrary.load_asset(expected_skeleton_path)
    if not expected_skeleton:
        fail(f"expected_skeleton_missing path={expected_skeleton_path}")

    skeleton = editor_property(asset, "skeleton")
    actual_skeleton_path = object_path(skeleton)
    if skeleton != expected_skeleton:
        fail(
            f"skeleton_mismatch path={asset_path} expected={object_path(expected_skeleton)} "
            f"actual={actual_skeleton_path}"
        )

    root_motion_sequences = []
    if isinstance(asset, unreal.AnimSequence):
        if sequence_uses_root_motion(asset):
            root_motion_sequences.append(asset)
    else:
        for sequence in montage_referenced_sequences(asset):
            if sequence_uses_root_motion(sequence):
                root_motion_sequences.append(sequence)

        slot_names = {
            str(editor_property(track, "slot_name", ""))
            for track in editor_property(asset, "slot_anim_tracks", []) or []
        }
        if required_slot and required_slot not in slot_names:
            fail(
                f"slot_missing path={asset_path} required={required_slot} "
                f"actual={','.join(sorted(slot_names)) or 'None'}"
            )

        if require_loop:
            has_self_loop = any(
                str(editor_property(section, "section_name", ""))
                == str(editor_property(section, "next_section_name", ""))
                and str(editor_property(section, "section_name", "")) not in ("", "None")
                for section in editor_property(asset, "composite_sections", []) or []
            )
            if not has_self_loop:
                fail(f"loop_section_missing path={asset_path}")

    if root_motion_sequences:
        paths = ",".join(object_path(sequence) for sequence in root_motion_sequences)
        fail(f"root_motion_not_allowed path={asset_path} sequences={paths}")

    unreal.log(
        f"{MARKER}|PASS|asset={asset_path}|class={expected_class}|"
        f"skeleton={actual_skeleton_path}|slot={required_slot or 'None'}|"
        f"loop_required={int(require_loop)}"
    )


main()
