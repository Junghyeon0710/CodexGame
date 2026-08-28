import unreal


ROOT = "/Game/AgentGameTest/Codex"
INPUT_PATH = f"{ROOT}/Input"
BLUEPRINT_PATH = f"{ROOT}/Blueprints"
ABILITY_PATH = f"{ROOT}/Abilities"
EFFECT_PATH = f"{ROOT}/Effects"
LEVEL_PATH = f"{ROOT}/Levels"
TEST_LEVEL = f"{LEVEL_PATH}/L_LastStand_PlayerTest_Codex"


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.make_directory(path):
            raise RuntimeError(f"Failed to create directory: {path}")


def create_or_load_asset(asset_name, package_path, asset_class, factory):
    asset_path = f"{package_path}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        existing = unreal.EditorAssetLibrary.load_asset(asset_path)
        if existing:
            return existing, False

    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, asset_class, factory
    )
    if not asset:
        raise RuntimeError(f"Failed to create asset: {asset_path}")
    return asset, True


def create_blueprint(asset_name, package_path, parent_class):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    blueprint, created = create_or_load_asset(
        asset_name, package_path, unreal.Blueprint, factory
    )
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    return blueprint, created


def make_key(key_name):
    key = unreal.Key()
    key.set_editor_property("key_name", key_name)
    return key


def make_mapping(action, key_name, mapping_context, modifier_classes=()):
    mapping = unreal.EnhancedActionKeyMapping()
    mapping.set_editor_property("action", action)
    mapping.set_editor_property("key", make_key(key_name))
    mapping.set_editor_property(
        "modifiers",
        [unreal.new_object(modifier_class, outer=mapping_context)
         for modifier_class in modifier_classes],
    )
    return mapping


def build_input_assets():
    action_factory = unreal.InputAction_Factory()
    move, _ = create_or_load_asset(
        "IA_Move", INPUT_PATH, unreal.InputAction, action_factory
    )
    attack, _ = create_or_load_asset(
        "IA_PrimaryAttack", INPUT_PATH, unreal.InputAction, unreal.InputAction_Factory()
    )
    dash, _ = create_or_load_asset(
        "IA_Dash", INPUT_PATH, unreal.InputAction, unreal.InputAction_Factory()
    )

    move.set_editor_property("value_type", unreal.InputActionValueType.AXIS2D)
    attack.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    dash.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)

    mapping_context, _ = create_or_load_asset(
        "IMC_Player",
        INPUT_PATH,
        unreal.InputMappingContext,
        unreal.InputMappingContext_Factory(),
    )

    mapping_data = unreal.InputMappingContextMappingData()
    mapping_data.set_editor_property(
        "mappings",
        [
            make_mapping(
                move, "W", mapping_context,
                (unreal.InputModifierSwizzleAxis,)
            ),
            make_mapping(
                move, "S", mapping_context,
                (unreal.InputModifierNegate, unreal.InputModifierSwizzleAxis)
            ),
            make_mapping(
                move, "A", mapping_context,
                (unreal.InputModifierNegate,)
            ),
            make_mapping(move, "D", mapping_context),
            make_mapping(attack, "LeftMouseButton", mapping_context),
            make_mapping(dash, "SpaceBar", mapping_context),
        ],
    )
    mapping_context.set_editor_property("default_key_mappings", mapping_data)

    for asset in (move, attack, dash, mapping_context):
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

    return [move, attack, dash, mapping_context]


def build_blueprint_assets():
    assets = []
    specs = [
        ("BP_LastStand_Player_Codex", BLUEPRINT_PATH, unreal.CodexLSPlayerCharacter),
        ("BP_GAS_TestTarget_Codex", BLUEPRINT_PATH, unreal.CodexLSGASTestTarget),
        ("BP_LastStand_GameMode_Codex", BLUEPRINT_PATH, unreal.CodexLSGameMode),
        ("GA_Player_PrimaryAttack", ABILITY_PATH, unreal.CodexLSGA_PrimaryAttack),
        ("GA_Player_Dash", ABILITY_PATH, unreal.CodexLSGA_Dash),
        ("GE_Player_DefaultAttributes", EFFECT_PATH, unreal.CodexLSGE_DefaultAttributes),
        ("GE_Damage", EFFECT_PATH, unreal.CodexLSGE_Damage),
        ("GE_Cooldown_PrimaryAttack", EFFECT_PATH, unreal.CodexLSGE_PrimaryAttackCooldown),
        ("GE_Cooldown_Dash", EFFECT_PATH, unreal.CodexLSGE_DashCooldown),
    ]

    for name, path, parent_class in specs:
        blueprint, _ = create_blueprint(name, path, parent_class)
        assets.append(blueprint)
    return assets


def destroy_owned_level_actors(world):
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        label = actor.get_actor_label()
        if label.startswith("CodexLS_"):
            unreal.EditorLevelLibrary.destroy_actor(actor)


def spawn_static_mesh(label, mesh_path, location, scale):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, location, unreal.Rotator()
    )
    if not actor:
        raise RuntimeError(f"Failed to spawn {label}")
    actor.set_actor_label(label)
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_scale3d(scale)
    return actor


def build_test_level():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(TEST_LEVEL):
        if not level_subsystem.load_level(TEST_LEVEL):
            raise RuntimeError(f"Failed to load existing level: {TEST_LEVEL}")
    else:
        if not level_subsystem.new_level(TEST_LEVEL):
            raise RuntimeError(f"Failed to create level: {TEST_LEVEL}")

    editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = editor_subsystem.get_editor_world()
    if not world:
        raise RuntimeError("Editor world unavailable")

    destroy_owned_level_actors(world)
    world.get_world_settings().set_editor_property(
        "default_game_mode", unreal.CodexLSGameMode
    )

    floor = spawn_static_mesh(
        "CodexLS_TestFloor",
        "/Engine/BasicShapes/Cube.Cube",
        unreal.Vector(0.0, 0.0, -50.0),
        unreal.Vector(20.0, 20.0, 1.0),
    )
    floor.static_mesh_component.set_collision_profile_name("BlockAll")

    player_start = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0.0, 0.0, 120.0), unreal.Rotator()
    )
    player_start.set_actor_label("CodexLS_PlayerStart")

    target_locations = [
        unreal.Vector(600.0, 0.0, 75.0),
        unreal.Vector(-600.0, 0.0, 75.0),
        unreal.Vector(0.0, 600.0, 75.0),
    ]
    for index, location in enumerate(target_locations, start=1):
        target = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.CodexLSGASTestTarget, location, unreal.Rotator()
        )
        if not target:
            raise RuntimeError(f"Failed to spawn GAS Test Target {index}")
        target.set_actor_label(f"CodexLS_GAS_TestTarget_{index:02d}")

    directional_light = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(0.0, 0.0, 600.0),
        unreal.Rotator(pitch=-50.0, yaw=-35.0, roll=0.0),
    )
    directional_light.set_actor_label("CodexLS_DirectionalLight")

    sky_light = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0.0, 0.0, 400.0), unreal.Rotator()
    )
    sky_light.set_actor_label("CodexLS_SkyLight")

    if not level_subsystem.save_current_level():
        raise RuntimeError(f"Failed to save level: {TEST_LEVEL}")
    return world


def main():
    for directory in (
        ROOT,
        INPUT_PATH,
        BLUEPRINT_PATH,
        ABILITY_PATH,
        EFFECT_PATH,
        LEVEL_PATH,
    ):
        ensure_directory(directory)

    input_assets = build_input_assets()
    blueprint_assets = build_blueprint_assets()
    build_test_level()

    unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
    unreal.log(
        "CODEX_STEP1_ASSET_BUILD_SUCCESS "
        f"input_assets={len(input_assets)} blueprint_assets={len(blueprint_assets)} "
        f"level={TEST_LEVEL}"
    )


main()
