import unreal


ROOT = "/Game/AgentGameTest/Codex"
BLUEPRINT_PATH = f"{ROOT}/Blueprints"
LEVEL_PATH = f"{ROOT}/Levels"
TEST_LEVEL = f"{LEVEL_PATH}/L_LastStand_GameLoopTest_Codex"

GAME_MODE_BLUEPRINT = f"{BLUEPRINT_PATH}/BP_GameMode_STEP3_Codex"
SPAWNER_BLUEPRINT = f"{BLUEPRINT_PATH}/BP_EnemySpawner_Codex"
SPAWN_POINT_BLUEPRINT = f"{BLUEPRINT_PATH}/BP_EnemySpawnPoint_Codex"
GRUNT_BLUEPRINT = f"{BLUEPRINT_PATH}/BP_Enemy_Grunt_Codex"
RUNNER_BLUEPRINT = f"{BLUEPRINT_PATH}/BP_Enemy_Runner_Codex"


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


def build_step3_blueprints():
    specs = [
        ("BP_GameMode_STEP3_Codex", unreal.CodexLSGameMode),
        ("BP_EnemySpawner_Codex", unreal.CodexLSEnemySpawner),
        ("BP_EnemySpawnPoint_Codex", unreal.CodexLSEnemySpawnPoint),
    ]

    assets = []
    created_count = 0
    for name, parent_class in specs:
        blueprint, created = create_blueprint(name, BLUEPRINT_PATH, parent_class)
        assets.append(blueprint)
        created_count += 1 if created else 0

    return assets, created_count


def destroy_step3_level_actors(world):
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if actor.get_actor_label().startswith("CodexLS3_"):
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


def set_optional_editor_property(target, property_name, value):
    try:
        target.get_editor_property(property_name)
        target.set_editor_property(property_name, value)
        return True
    except Exception as error:
        unreal.log_warning(
            f"STEP3 optional property unavailable: "
            f"{target.get_name()}.{property_name} ({error})"
        )
        return False


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

    destroy_step3_level_actors(world)

    game_mode_class = load_blueprint_class(GAME_MODE_BLUEPRINT)
    spawner_class = load_blueprint_class(SPAWNER_BLUEPRINT)
    spawn_point_class = load_blueprint_class(SPAWN_POINT_BLUEPRINT)
    grunt_class = load_blueprint_class(GRUNT_BLUEPRINT)
    runner_class = load_blueprint_class(RUNNER_BLUEPRINT)

    world.get_world_settings().set_editor_property(
        "default_game_mode", game_mode_class
    )

    floor = spawn_static_mesh(
        "CodexLS3_TestFloor",
        "/Engine/BasicShapes/Cube.Cube",
        unreal.Vector(0.0, 0.0, -50.0),
        unreal.Vector(60.0, 60.0, 1.0),
    )
    floor.static_mesh_component.set_collision_profile_name("BlockAll")

    spawn_actor(
        unreal.PlayerStart,
        "CodexLS3_PlayerStart",
        unreal.Vector(0.0, 0.0, 100.0),
    )

    obstacle_specs = [
        (
            "CodexLS3_Obstacle_CenterWall",
            unreal.Vector(500.0, 0.0, 100.0),
            unreal.Vector(0.7, 5.0, 2.0),
        ),
        (
            "CodexLS3_Obstacle_NorthWest",
            unreal.Vector(-650.0, 700.0, 100.0),
            unreal.Vector(2.2, 2.0, 2.0),
        ),
        (
            "CodexLS3_Obstacle_SouthWest",
            unreal.Vector(-700.0, -750.0, 100.0),
            unreal.Vector(2.0, 2.4, 2.0),
        ),
    ]
    for label, location, scale in obstacle_specs:
        spawn_static_mesh(
            label,
            "/Engine/BasicShapes/Cube.Cube",
            location,
            scale,
        )

    spawner = spawn_actor(
        spawner_class,
        "CodexLS3_EnemySpawner",
        unreal.Vector(0.0, 0.0, 100.0),
    )
    set_optional_editor_property(spawner, "grunt_class", grunt_class)
    set_optional_editor_property(spawner, "runner_class", runner_class)
    set_optional_editor_property(spawner, "minimum_player_distance", 1100.0)
    set_optional_editor_property(spawner, "max_spawn_attempts", 12)

    spawn_point_specs = [
        ("CodexLS3_SpawnPoint_N", unreal.Vector(0.0, 2300.0, 100.0), -90.0),
        ("CodexLS3_SpawnPoint_NE", unreal.Vector(1900.0, 1600.0, 100.0), -140.0),
        ("CodexLS3_SpawnPoint_SE", unreal.Vector(2100.0, -1500.0, 100.0), 144.0),
        ("CodexLS3_SpawnPoint_S", unreal.Vector(0.0, -2300.0, 100.0), 90.0),
        ("CodexLS3_SpawnPoint_SW", unreal.Vector(-1900.0, -1600.0, 100.0), 40.0),
        ("CodexLS3_SpawnPoint_NW", unreal.Vector(-2100.0, 1500.0, 100.0), -36.0),
    ]
    for label, location, yaw in spawn_point_specs:
        spawn_actor(
            spawn_point_class,
            label,
            location,
            unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0),
        )

    nav_bounds = spawn_actor(
        unreal.NavMeshBoundsVolume,
        "CodexLS3_NavMeshBounds",
        unreal.Vector(0.0, 0.0, 250.0),
    )
    nav_bounds.set_actor_scale3d(unreal.Vector(31.0, 31.0, 5.0))

    spawn_actor(
        unreal.DirectionalLight,
        "CodexLS3_DirectionalLight",
        unreal.Vector(0.0, 0.0, 900.0),
        unreal.Rotator(pitch=-50.0, yaw=-35.0, roll=0.0),
    )
    spawn_actor(
        unreal.SkyLight,
        "CodexLS3_SkyLight",
        unreal.Vector(0.0, 0.0, 600.0),
    )

    if not level_subsystem.save_current_level():
        raise RuntimeError(f"Failed to save level: {TEST_LEVEL}")

    return len(spawn_point_specs)


def main():
    for directory in (ROOT, BLUEPRINT_PATH, LEVEL_PATH):
        ensure_directory(directory)

    blueprint_assets, created_count = build_step3_blueprints()
    spawn_point_count = build_test_level()

    unreal.log(
        "CODEX_STEP3_ASSET_BUILD_SUCCESS "
        f"bp_assets={len(blueprint_assets)} newly_created={created_count} "
        f"levels=1 level={TEST_LEVEL} spawnpoints={spawn_point_count} enemies=0 "
        "navmesh_bounds=1 obstacles=3"
    )


main()
