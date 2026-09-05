"""Arena-only visual variants. Existing STEP 1-3 Blueprints stay unchanged."""
import json
from pathlib import Path
import unreal

ROOT = "/Game/AgentGameTest/Codex"
BP_ROOT = ROOT + "/Blueprints"
MAT_ROOT = ROOT + "/Environment/Materials"


def variant(name, parent_path, values):
    path = BP_ROOT + "/" + name
    blueprint = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if blueprint is None:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.EditorAssetLibrary.load_blueprint_class(parent_path))
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, BP_ROOT, unreal.Blueprint, factory)
    cls = unreal.EditorAssetLibrary.load_blueprint_class(path)
    cdo = unreal.get_default_object(cls)
    for key, value in values.items():
        cdo.set_editor_property(key, value)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError("Blueprint save failed: " + path)
    return unreal.EditorAssetLibrary.load_blueprint_class(path)


def material(name, color):
    path = MAT_ROOT + "/" + name
    value = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if value is None:
        value = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, MAT_ROOT,
            unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    lib = unreal.MaterialEditingLibrary
    lib.set_material_instance_parent(value, unreal.load_asset(MAT_ROOT + "/M_LastStand_Surface_Codex"))
    for key, number in {"BaseColorTexWeight": 0, "NormalStrength": 0, "Metallic": 0,
                        "MetallicMapWeight": 0, "RoughnessMultiplier": 1.4,
                        "EmissiveStrength": 0.15}.items():
        lib.set_material_instance_scalar_parameter_value(value, key, number)
    lib.set_material_instance_vector_parameter_value(value, "ColorTint", unreal.LinearColor(*color, 1))
    lib.set_material_instance_vector_parameter_value(value, "EmissiveColor", unreal.LinearColor(*color, 1))
    lib.update_material_instance(value)
    unreal.EditorAssetLibrary.save_loaded_asset(value, only_if_is_dirty=False)
    return value


def main():
    player_material = material("MI_LS_PlayerReadability_Codex", (0.02, 0.8, 1.0))
    enemy_material = material("MI_LS_EnemyReadability_Codex", (1.0, 0.3, 0.1))
    player = variant("BP_Player_Arena_Codex", BP_ROOT + "/BP_LastStand_Player_Codex",
                     {"visual_material": player_material})
    variant("BP_Grunt_Arena_Codex", BP_ROOT + "/BP_Enemy_Grunt_Codex",
            {"visual_material": enemy_material, "enemy_color": unreal.LinearColor(0.7, 0.045, 0.035, 1)})
    variant("BP_Runner_Arena_Codex", BP_ROOT + "/BP_Enemy_Runner_Codex",
            {"visual_material": enemy_material, "enemy_color": unreal.LinearColor(1.0, 0.48, 0.025, 1)})
    variant("BP_GameMode_Arena_Codex", BP_ROOT + "/BP_GameMode_STEP3_Codex",
            {"default_pawn_class": player})
    result = {"blueprints": 4, "material_instances": 2, "original_step123_assets_modified": False,
              "purpose": "Arena-only Player/Grunt/Runner readability; inherited gameplay unchanged"}
    path = Path(unreal.Paths.project_dir()).resolve() / "Saved/AgentComparison/Codex/Step04_Readability.json"
    path.write_text(json.dumps(result, indent=2), encoding="utf-8")
    unreal.log("CODEX_STEP4_READABILITY_SUCCESS " + json.dumps(result))


main()
