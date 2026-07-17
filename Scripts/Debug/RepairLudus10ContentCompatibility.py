import unreal


SOURCE_DIRECTORY = "/Game/Fab/Stylized_Medieval_Well_–_Low_Poly_3D_Asset"
DESTINATION_DIRECTORY = "/Game/Fab/Stylized_Medieval_Well_Low_Poly_3D_Asset"
HUDSON_TPOSE_SOURCE = "/Game/Hunters/Hudson/Hudson_TPOSE"
HUDSON_TPOSE_DESTINATION = "/Game/Hunters/Hudson/Meshes/Hudson_TPOSE"


def delete_unreferenced_redirector(asset_path):
    asset_data = unreal.EditorAssetLibrary.find_asset_data(asset_path)
    if (
        not asset_data.is_valid()
        or str(asset_data.asset_class_path.asset_name) != "ObjectRedirector"
    ):
        raise RuntimeError(f"Expected an object redirector at {asset_path}")

    referencers = unreal.EditorAssetLibrary.find_package_referencers_for_asset(
        asset_path, load_assets_to_confirm=True
    )
    if referencers:
        raise RuntimeError(
            f"Redirector still has package referencers at {asset_path}: {referencers}"
        )
    if not unreal.EditorAssetLibrary.delete_asset(asset_path):
        raise RuntimeError(f"Failed to delete unreferenced redirector: {asset_path}")


def repair_ascii_package_path():
    if not unreal.EditorAssetLibrary.does_directory_exist(DESTINATION_DIRECTORY):
        if not unreal.EditorAssetLibrary.does_directory_exist(SOURCE_DIRECTORY):
            raise RuntimeError(f"Source directory not found: {SOURCE_DIRECTORY}")

        if not unreal.EditorAssetLibrary.rename_directory(
            SOURCE_DIRECTORY, DESTINATION_DIRECTORY
        ):
            raise RuntimeError(
                f"Failed to rename {SOURCE_DIRECTORY} to {DESTINATION_DIRECTORY}"
            )

    unreal.EditorAssetLibrary.save_directory(
        DESTINATION_DIRECTORY, only_if_is_dirty=False, recursive=True
    )

    if unreal.EditorAssetLibrary.does_directory_exist(SOURCE_DIRECTORY):
        remaining_assets = unreal.EditorAssetLibrary.list_assets(
            SOURCE_DIRECTORY, recursive=True, include_folder=False
        )
        non_redirectors = []
        for asset_path in remaining_assets:
            asset_data = unreal.EditorAssetLibrary.find_asset_data(asset_path)
            if (
                not asset_data.is_valid()
                or str(asset_data.asset_class_path.asset_name) != "ObjectRedirector"
            ):
                non_redirectors.append(asset_path)

        if non_redirectors:
            raise RuntimeError(
                "Non-redirector assets remain in the source directory after rename: "
                f"{non_redirectors}"
            )
        for asset_path in remaining_assets:
            delete_unreferenced_redirector(asset_path)
        if remaining_assets:
            unreal.log(
                "BB_LUDUS_REPAIR|ASCII_REDIRECTORS|REMOVED="
                f"{len(remaining_assets)}"
            )

    unreal.log("BB_LUDUS_REPAIR|ASCII_PATH|RENAMED")


def repair_hudson_tpose_package_path():
    source_exists = unreal.EditorAssetLibrary.does_asset_exist(HUDSON_TPOSE_SOURCE)
    destination_exists = unreal.EditorAssetLibrary.does_asset_exist(
        HUDSON_TPOSE_DESTINATION
    )

    if source_exists and destination_exists:
        source_data = unreal.EditorAssetLibrary.find_asset_data(HUDSON_TPOSE_SOURCE)
        source_class = (
            str(source_data.asset_class_path.asset_name)
            if source_data.is_valid()
            else "Missing"
        )
        if source_class != "ObjectRedirector":
            raise RuntimeError(
                "Both Hudson T-pose package paths contain real assets; refusing an "
                "ambiguous repair"
            )
        delete_unreferenced_redirector(HUDSON_TPOSE_SOURCE)
        source_exists = False
        unreal.log("BB_LUDUS_REPAIR|HUDSON_TPOSE_REDIRECTOR|REMOVED")

    if not destination_exists:
        if not source_exists:
            raise RuntimeError(
                "Hudson T-pose asset is missing from both expected package paths"
            )

        unreal.EditorAssetLibrary.make_directory("/Game/Hunters/Hudson/Meshes")
        if not unreal.EditorAssetLibrary.rename_asset(
            HUDSON_TPOSE_SOURCE, HUDSON_TPOSE_DESTINATION
        ):
            raise RuntimeError(
                f"Failed to rename {HUDSON_TPOSE_SOURCE} to "
                f"{HUDSON_TPOSE_DESTINATION}"
            )

    unreal.EditorAssetLibrary.save_asset(
        HUDSON_TPOSE_DESTINATION, only_if_is_dirty=False
    )
    unreal.log("BB_LUDUS_REPAIR|HUDSON_TPOSE_PATH|READY")


repair_ascii_package_path()
repair_hudson_tpose_package_path()
unreal.log("BB_LUDUS_REPAIR|COMPLETE")
