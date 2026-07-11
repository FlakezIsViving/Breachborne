"""
Creates the DA_Eluna data asset in /Game/Hunters/Eluna.

Run this from the Unreal Editor Python console:
  1. Open Editor
  2. Window > Developer Tools > Python Console
  3. Paste the script below and press Enter
  4. Save the asset (Ctrl+S)
  5. Configure abilities in the asset's Details panel

Or run via File > Execute Python Script.
"""

import unreal

PACKAGE_PATH = "/Game/Hunters/Eluna"
ASSET_NAME = "DA_Eluna"
FULL_PATH = PACKAGE_PATH + "/" + ASSET_NAME

# Check if asset already exists
if unreal.EditorAssetLibrary.does_asset_exist(FULL_PATH):
    unreal.log_warning("DA_Eluna already exists at " + FULL_PATH)
    existing = unreal.EditorAssetLibrary.load_asset(FULL_PATH)
    unreal.EditorAssetLibrary.sync_browser_to_objects([existing])
    print("Asset already exists — selected in Content Browser.")
else:
    # Load the HunterDefinition class
    hunter_def_class = unreal.load_class(None, "/Script/Breachborne.BBHunterDefinition")
    if not hunter_def_class:
        unreal.log_error("Could not load UBBHunterDefinition class. Make sure the project is compiled.")
        print("ERROR: UBBHunterDefinition class not found. Compile the project first.")
    else:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

        # Create a Blueprint-based data asset (same workflow as Ghost/Kingpin)
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", hunter_def_class)

        new_asset = asset_tools.create_asset(
            asset_name=ASSET_NAME,
            package_path=PACKAGE_PATH,
            asset_class=unreal.Blueprint,
            factory=factory
        )

        if new_asset:
            unreal.EditorAssetLibrary.save_loaded_asset(new_asset)
            unreal.EditorAssetLibrary.sync_browser_to_objects([new_asset])
            print("SUCCESS: Created DA_Eluna at " + FULL_PATH)
            print("Next steps:")
            print("  1. Open DA_Eluna")
            print("  2. Set Hunter Name = 'Eluna'")
            print("  3. Set Role = Protector")
            print("  4. Add 7 abilities to Abilities To Grant:")
            print("       GA_Eluna_LMB, GA_Eluna_RMB, GA_Eluna_AerialDash,")
            print("       GA_Eluna_GroundDash, GA_Eluna_Q, GA_Eluna_R, GA_Eluna_Passive")
            print("  5. Set base stats (Health=500, AttackPower=40, AbilityPower=60, MoveSpeed=600)")
            print("  6. Save")
            print("  7. Add HunterID 3 -> DA_Eluna to GameMode's HunterDefinitions map")
        else:
            unreal.log_error("Failed to create DA_Eluna")
            print("ERROR: Asset creation failed.")
