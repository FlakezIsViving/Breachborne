"""
Creates the DA_Hudson data asset in /Game/Hunters/Hudson.

Run from the Unreal Editor Python console or File > Execute Python Script.
Hudson is also registered as a native C++ fallback on HunterID 4, so this asset
is for editor-driven tuning and final art assignment.
"""

import unreal

PACKAGE_PATH = "/Game/Hunters/Hudson"
ASSET_NAME = "DA_Hudson"
FULL_PATH = PACKAGE_PATH + "/" + ASSET_NAME

if unreal.EditorAssetLibrary.does_asset_exist(FULL_PATH):
    existing = unreal.EditorAssetLibrary.load_asset(FULL_PATH)
    unreal.EditorAssetLibrary.sync_browser_to_objects([existing])
    print("DA_Hudson already exists - selected in Content Browser.")
else:
    hunter_def_class = unreal.load_class(None, "/Script/Breachborne.BBHunterDefinition")
    if not hunter_def_class:
        unreal.log_error("Could not load UBBHunterDefinition class. Compile the project first.")
    else:
        unreal.EditorAssetLibrary.make_directory(PACKAGE_PATH)
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", hunter_def_class)
        new_asset = asset_tools.create_asset(
            asset_name=ASSET_NAME,
            package_path=PACKAGE_PATH,
            asset_class=hunter_def_class,
            factory=factory,
        )
        if new_asset:
            unreal.EditorAssetLibrary.save_loaded_asset(new_asset)
            unreal.EditorAssetLibrary.sync_browser_to_objects([new_asset])
            print("SUCCESS: Created DA_Hudson at " + FULL_PATH)
            print("Set Hunter Name=Hudson, Role=Controller, Health=600, AP=50, AbilityPower=50, MoveSpeed=575.")
            print("Add abilities: GA_Hudson_LMB_Minigun, GA_Hudson_RMB_SpinBarrels, GA_Hudson_Shift_HoverJets, GA_Hudson_Q_BarbedWire, GA_Hudson_R_SalvageHook, GA_Hudson_Passive_Salvager.")
            print("Assign Hudson skeletal mesh, ABP_Hudson, and optional montages after importing the model/animations.")
            print("Optionally add HunterID 4 -> DA_Hudson to BP_BreachborneGameMode.HunterDefinitions.")
        else:
            unreal.log_error("Failed to create DA_Hudson")
