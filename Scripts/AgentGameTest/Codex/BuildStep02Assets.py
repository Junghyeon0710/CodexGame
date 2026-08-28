import unreal


ROOT = "/Game/AgentGameTest/Codex"
BLUEPRINT_PATH = f"{ROOT}/Blueprints"
ABILITY_PATH = f"{ROOT}/Abilities"
EFFECT_PATH = f"{ROOT}/Effects"
LEVEL_PATH = f"{ROOT}/Levels"
TEST_LEVEL = f"{LEVEL_PATH}/L_LastStand_EnemyTest_Codex"


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


def build_enemy_assets():
    specs = [
        ("BP_Enemy_Base_Codex", BLUEPRINT_PATH, unreal.CodexLSEnemyCharacter),
        ("BP_Enemy_Grunt_Codex", BLUEPRINT_PATH, unreal.CodexLSEnemyGrunt),
        ("BP_Enemy_Runner_Codex", BLUEPRINT_PATH, unreal.CodexLSEnemyRunner),
        ("BP_Enemy_AIController_Codex", BLUEPRINT_PATH, unreal.CodexLSEnemyAIController),
        ("GA_Enemy_MeleeAttack", ABILITY_PATH, unreal.CodexLSGA_EnemyMeleeAttack),
        ("GE_Enemy_DefaultAttributes", EFFECT_PATH, unreal.CodexLSGE_EnemyDefaultAttributes),
        ("GE_Cooldown_Enemy_MeleeAttack", EFFECT_PATH, unreal.CodexLSGE_EnemyMeleeCooldown),
    ]

    assets = []
    created_count = 0
    for name, path, parent_class in specs:
        blueprint, created = create_blueprint(name, path, parent_class)
        assets.append(blueprint)
        created_count += 1 if created else 0

    return assets, created_count


def destroy_step2_level_actors(world):
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if actor.get_actor_label().startswith("CodexLS2_"):
            actor_subsystem.destroy_actor(actor)


def spawn_actor(actor_class, label, location, rotation=None):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class, location, rotation or unreal.Rotator()
    )
    if not actor:
        raise RuntimeError(f"Failed to spawn {label}")
    actor.set_actor_label(label)
    return actor


def spawn_static_mesh(label, mesh_path, location, scale):
    actor = spawn_actor(unreal.StaticMeshActor, label, location)
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    if not mesh:
        raise RuntimeError(f"Failed to load mesh: {mesh_path}")
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_collision_profile_name("BlockAll")
    actor.set_actor_scale3d(scale)
    return actor


def load_blueprint_class(asset_path):
    generated_class = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
    if not generated_class:
        raise RuntimeError(f"Failed to load Blueprint class: {asset_path}")
    return generated_class


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

    destroy_step2_level_actors(world)
    world.get_world_settings().set_editor_property(
        "default_game_mode", unreal.CodexLSGameMode
    )

    floor = spawn_static_mesh(
        "CodexLS2_TestFloor",
        "/Engine/BasicShapes/Cube.Cube",
        unreal.Vector(0.0, 0.0, -50.0),
        unreal.Vector(30.0, 30.0, 1.0),
    )
    floor.static_mesh_component.set_collision_profile_name("BlockAll")

    spawn_actor(
        unreal.PlayerStart,
        "CodexLS2_PlayerStart",
        unreal.Vector(0.0, 0.0, 100.0),
    )

    obstacle_specs = [
        ("CodexLS2_Obstacle_Wall", unreal.Vector(420.0, 0.0, 100.0), unreal.Vector(0.6, 4.2, 2.0)),
        ("CodexLS2_Obstacle_Box_NW", unreal.Vector(-350.0, 480.0, 100.0), unreal.Vector(2.0, 2.0, 2.0)),
        ("CodexLS2_Obstacle_Box_SW", unreal.Vector(-350.0, -520.0, 100.0), unreal.Vector(1.8, 1.8, 2.0)),
    ]
    for label, location, scale in obstacle_specs:
        spawn_static_mesh(
            label,
            "/Engine/BasicShapes/Cube.Cube",
            location,
            scale,
        )

    grunt_class = load_blueprint_class(f"{BLUEPRINT_PATH}/BP_Enemy_Grunt_Codex")
    runner_class = load_blueprint_class(f"{BLUEPRINT_PATH}/BP_Enemy_Runner_Codex")

    enemy_specs = [
        (grunt_class, "CodexLS2_Grunt_01", unreal.Vector(1200.0, -700.0, 100.0)),
        (grunt_class, "CodexLS2_Grunt_02", unreal.Vector(-1150.0, 650.0, 100.0)),
        (runner_class, "CodexLS2_Runner_01", unreal.Vector(1150.0, 700.0, 100.0)),
        (runner_class, "CodexLS2_Runner_02", unreal.Vector(-1050.0, -700.0, 100.0)),
    ]
    for enemy_class, label, location in enemy_specs:
        spawn_actor(enemy_class, label, location)

    nav_bounds = spawn_actor(
        unreal.NavMeshBoundsVolume,
        "CodexLS2_NavMeshBounds",
        unreal.Vector(0.0, 0.0, 250.0),
    )
    nav_bounds.set_actor_scale3d(unreal.Vector(16.0, 16.0, 5.0))

    spawn_actor(
        unreal.DirectionalLight,
        "CodexLS2_DirectionalLight",
        unreal.Vector(0.0, 0.0, 800.0),
        unreal.Rotator(pitch=-50.0, yaw=-35.0, roll=0.0),
    )
    spawn_actor(
        unreal.SkyLight,
        "CodexLS2_SkyLight",
        unreal.Vector(0.0, 0.0, 500.0),
    )

    if not level_subsystem.save_current_level():
        raise RuntimeError(f"Failed to save level: {TEST_LEVEL}")
    return world


def main():
    for directory in (ROOT, BLUEPRINT_PATH, ABILITY_PATH, EFFECT_PATH, LEVEL_PATH):
        ensure_directory(directory)

    enemy_assets, created_count = build_enemy_assets()
    build_test_level()

    unreal.log(
        "CODEX_STEP2_ASSET_BUILD_SUCCESS "
        f"enemy_assets={len(enemy_assets)} newly_created={created_count} "
        f"level={TEST_LEVEL} enemies=4 navmesh_bounds=1 obstacles=3"
    )


main()
