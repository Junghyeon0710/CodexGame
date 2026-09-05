"""Author the Codex-only Last Stand industrial arena; no Gameplay rewrites."""
import json
from pathlib import Path
import unreal

PROJECT = Path(unreal.Paths.project_dir()).resolve()
ROOT = "/Game/AgentGameTest/Codex"
ENV = f"{ROOT}/Environment"
LEVEL = f"{ROOT}/Levels/L_LastStand_Arena_Codex"
PREFIX = "CodexLS4_"
REPORT_DIR = PROJECT / "Saved/AgentComparison/Codex"
IMPORT_REPORT = REPORT_DIR / "Step04_ImportReport.json"
ACTORS = []
OPTIONAL_WARNINGS = []


def prop(obj, name, value, required=False):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as error:
        message = f"{obj.get_name()}.{name}: {error}"
        if required:
            raise RuntimeError(message)
        OPTIONAL_WARNINGS.append(message)
        return False


def load(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError(f"Required asset missing: {path}")
    return asset


def bp(path):
    value = unreal.EditorAssetLibrary.load_blueprint_class(path)
    if not value:
        raise RuntimeError(f"Required class missing: {path}")
    return value


def actor(cls, name, xyz, yaw=0.0, folder="Environment"):
    value = unreal.EditorLevelLibrary.spawn_actor_from_class(
        cls, unreal.Vector(*xyz), unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0))
    if not value:
        raise RuntimeError(f"Spawn failed: {name}")
    value.set_actor_label(PREFIX + name)
    value.set_folder_path("LastStand_Codex/" + folder)
    ACTORS.append({"label": PREFIX + name, "class": value.get_class().get_name(),
                   "location": list(xyz), "yaw": yaw, "folder": folder})
    return value


def mesh(name, kind, xyz, yaw=0.0, scale=(1.0, 1.0, 1.0),
         material=None, collision=True, folder="Environment"):
    if kind == "ConcreteBarrier":
        # 112.5cm blocks the 90cm-high shot origin, while retaining camera sight.
        scale = (scale[0], scale[1], scale[2] * 1.25)
    value = actor(unreal.StaticMeshActor, name, xyz, yaw, folder)
    comp = value.static_mesh_component
    comp.set_static_mesh(load(f"{ENV}/Meshes/SM_LS_{kind}"))
    comp.set_mobility(unreal.ComponentMobility.STATIC)
    comp.set_collision_profile_name("BlockAll" if collision else "NoCollision")
    value.set_actor_scale3d(unreal.Vector(*scale))
    if material:
        # Override only the PaintedMetal slot so hinges, ribs and fittings retain contrast.
        sm = comp.static_mesh
        for index, slot in enumerate(sm.get_editor_property("static_materials")):
            slot_name = str(slot.get_editor_property("material_slot_name"))
            if "PaintedMetal" in slot_name or kind == "GroundTile":
                comp.set_material(index, MATERIALS[material])
    ACTORS[-1].update({"mesh": kind, "scale": list(scale), "collision": collision,
                      "material_override": material})
    return value


def null_navigation(name, xyz, size):
    volume = actor(unreal.NavModifierVolume, "NavNull_" + name, xyz, folder="Navigation")
    volume.set_actor_scale3d(unreal.Vector(*(v / 200.0 for v in size)))
    null_class = unreal.load_class(None, "/Script/NavigationSystem.NavArea_Null")
    prop(volume, "area_class", null_class, required=True)
    return volume


def paint(name, xyz, length, width, yaw=0.0, color="Yellow"):
    return mesh(name, "GroundTile", xyz, yaw,
                (length / 1000.0, width / 1000.0, 0.025),
                color, False, "Ground/Markings")


def ground():
    mesh("BackgroundApron", "GroundTile", (0, 0, -18), scale=(9, 9, 1),
         material="AsphaltBackground", collision=False, folder="Background")
    # Five-by-five 10m modules retain the kit's metric UV density with no stretching.
    for ix, x in enumerate(range(-2000, 2001, 1000)):
        for iy, y in enumerate(range(-2000, 2001, 1000)):
            surface = "Asphalt" if (iy == 1 or ix == 0 or (ix == 4 and iy < 3)) else "Concrete"
            mesh(f"Ground_{ix}_{iy}", "GroundTile", (x, y, 0),
                 material=surface, folder="Ground")
    for index, x in enumerate(range(-1800, 1801, 400)):
        paint(f"LaneDash_{index}", (x, -950, 1.0), 220, 12)
    for side in (-1, 1):
        paint(f"MaintenanceLine_{side}", (200, side * 590, 1.1), 840, 12)
    for index in range(7):
        paint(f"Crossing_{index}", (-1350 + index * 100, 300, 1.2), 50, 440,
              color="PaintWhite")
    for index, (x, y) in enumerate([(-1700, 2100), (1400, 2100), (-1650, -2100),
                                    (900, -2100), (-2350, 200), (1850, 50)]):
        mesh(f"Drain_{index}", "Drain", (x, y, 0.5), yaw=90 if index > 3 else 0,
             collision=False, folder="Ground/Details")


def boundary():
    for axis in ("X", "Y"):
        for side in (-1, 1):
            for index, t in enumerate(range(-2400, 2401, 400)):
                xyz = (side * 2500, t, 0) if axis == "X" else (t, side * 2500, 0)
                yaw = 90 if axis == "X" else 0
                mesh(f"Curb_{axis}_{side}_{index}", "Curb", xyz, yaw,
                     folder="Boundary")
                # Warehouse itself closes this part of the eastern perimeter.
                if axis == "X" and side == 1 and t in (400, 800):
                    continue
                mesh(f"Fence_{axis}_{side}_{index}", "FenceSection",
                     (xyz[0], xyz[1], 22), yaw, folder="Boundary")


def structures():
    # Central low, roof-free island: readable in the -X to +X player camera.
    mesh("CentralPipeSkid", "PipeSkid", (150, 0, 0), yaw=90,
         folder="CentralMaintenance")
    null_navigation("CentralPipeSkid", (150, 0, 150), (290, 640, 400))
    mesh("CentralUtility_A", "UtilityBox", (380, -410, 0), yaw=180,
         material="PaintedGreen", folder="CentralMaintenance")
    mesh("CentralUtility_B", "UtilityBox", (390, 390, 0), yaw=180,
         material="PaintedBlue", folder="CentralMaintenance")
    mesh("CentralBarrier_A", "ConcreteBarrier", (-150, -400, 0), yaw=90,
         folder="CentralMaintenance")
    mesh("CentralBarrier_B", "ConcreteBarrier", (-150, 400, 0), yaw=90,
         folder="CentralMaintenance")
    container_specs = [
        ("NorthBlue", (-850, 1580, 0), 0, "PaintedBlue"),
        ("NorthRed", (650, 1550, 0), 0, "PaintedRed"),
        ("SouthGreen", (-1180, -1500, 0), 0, "PaintedGreen"),
        ("EastRed", (1350, -1400, 0), 90, "PaintedRed"),
    ]
    for name, xyz, yaw, mat in container_specs:
        # Low cargo units keep a player touching the far side visible at pitch -55.
        mesh("Container_" + name, "Container", xyz, yaw, scale=(1.0, 1.0, 0.45), material=mat,
             folder="Containers")
        size = (646, 284, 340) if yaw == 0 else (284, 646, 340)
        null_navigation(name, (xyz[0], xyz[1], 160), size)
    mesh("Warehouse_Bay04", "Warehouse", (2470, 550, 0), yaw=180,
         material="PaintedBlue", folder="Warehouse")
    null_navigation("Warehouse", (2470, 550, 220), (660, 440, 540))
    # Low barriers establish choices without creating dead ends or <3m corridors.
    for name, xyz, yaw in [
        ("WestLoading", (-1350, 950, 0), 90),
        ("SouthLoading", (-400, -1650, 0), 0),
        ("EastApproach", (1500, 450, 0), 90),
        ("NorthApproach", (1100, 1050, 0), 0),
    ]:
        mesh("Barrier_" + name, "ConcreteBarrier", xyz, yaw, folder="Cover")
    for index, (xyz, yaw, mat) in enumerate([
        ((3350, -1200, 0), 180, "PaintedGreen"),
        ((3400, 2050, 0), 180, "PaintedRed"),
        ((1500, 3250, 0), -90, "PaintedBlue"),
    ]):
        mesh(f"BackgroundWarehouse_{index}", "Warehouse", xyz, yaw,
             scale=(1.6, 1.6, 1.35), material=mat, collision=False, folder="Background")
    for index, (x, y, yaw, mat) in enumerate([
        (-1800, 3000, 0, "PaintedGreen"), (-1100, 3000, 0, "PaintedRed"),
        (3000, -2300, 90, "PaintedBlue"), (800, -3050, 0, "PaintedGreen"),
    ]):
        mesh(f"BackgroundContainer_{index}", "Container", (x, y, 0), yaw,
             material=mat, collision=False, folder="Background")
        if index in (1, 2):
            mesh(f"BackgroundContainerStack_{index}", "Container", (x + 12, y - 8, 259),
                 yaw, material="PaintedBlue" if index == 1 else "PaintedRed",
                 collision=False, folder="Background")


def props():
    # Detail clusters sit against existing blockers, not in the traversal routes.
    clusters = [(-470, 1550), (1000, 1550), (-1550, -1470), (1530, -1450), (2080, 570)]
    for index, (x, y) in enumerate(clusters):
        mesh(f"Pallet_{index}", "Pallet", (x, y, 0), yaw=(index % 3 - 1) * 12,
             folder="Props/Loading")
        mesh(f"Crate_{index}", "Crate", (x, y, 15), yaw=(index % 3 - 1) * 12,
             folder="Props/Loading")
        for j, (dx, dy) in enumerate([(100, -30), (165, -20), (132, 40)]):
            # Keep these two barrel groups outside container/warehouse collision.
            dx += -230 if index == 2 else (-340 if index == 4 else 0)
            mesh(f"Barrel_{index}_{j}", "Barrel", (x + dx, y + dy, 0),
                 yaw=index * 47 + j * 21,
                 material=["PaintedBlue", "PaintedRed", "PaintedGreen"][(index + j) % 3],
                 folder="Props/Barrels")
    for index, (x, y, yaw) in enumerate([(-2300, 700, 0), (-2300, -700, 0),
                                        (2200, 1250, 180), (2200, -700, 180)]):
        mesh(f"IndustrialLamp_{index}", "IndustrialLamp", (x, y, 0), yaw,
             folder="Lighting/Fixtures")


def lighting():
    sunlight = actor(unreal.DirectionalLight, "AfternoonSun", (0, 0, 1500), folder="Lighting")
    sunlight.set_actor_rotation(unreal.Rotator(pitch=-42, yaw=-28, roll=0), False)
    comp = sunlight.get_component_by_class(unreal.DirectionalLightComponent)
    comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    comp.set_intensity(12000.0)
    comp.set_light_color(unreal.LinearColor(1.0, 0.94, 0.83, 1.0))
    prop(comp, "atmosphere_sun_light", True)
    actor(unreal.SkyAtmosphere, "SkyAtmosphere", (0, 0, 0), folder="Lighting")
    sky = actor(unreal.SkyLight, "SkyFill", (0, 0, 800), folder="Lighting")
    sky_comp = sky.get_component_by_class(unreal.SkyLightComponent)
    sky_comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky_comp.set_intensity(1.25)
    prop(sky_comp, "real_time_capture", True)
    fog = actor(unreal.ExponentialHeightFog, "LightAtmosphere", (0, 0, -250), folder="Lighting")
    fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    prop(fog_comp, "fog_density", 0.003)
    prop(fog_comp, "start_distance", 3500.0)
    post = actor(unreal.PostProcessVolume, "ReadabilityPostProcess", (0, 0, 0), folder="Lighting")
    prop(post, "unbound", True, required=True)
    settings = post.get_editor_property("settings")
    for name, value in {
        "auto_exposure_min_brightness": 11.4,
        "auto_exposure_max_brightness": 11.4,
        "auto_exposure_bias": 0.0,
        "bloom_intensity": 0.12,
        "vignette_intensity": 0.05,
        "scene_fringe_intensity": 0.0,
        "motion_blur_amount": 0.0,
    }.items():
        settings.set_editor_property("override_" + name, True)
        settings.set_editor_property(name, value)
    prop(post, "settings", settings, required=True)
    # Only two local lights; the emissive fixtures carry most of the daytime detail.
    for index, xyz in enumerate([(2140, 550, 270), (350, -350, 150)]):
        light = actor(unreal.PointLight, f"MaintenanceLight_{index}", xyz, folder="Lighting")
        point = light.get_component_by_class(unreal.PointLightComponent)
        point.set_mobility(unreal.ComponentMobility.MOVABLE)
        point.set_intensity(750.0)
        point.set_light_color(unreal.LinearColor(1.0, 0.65, 0.28, 1.0))
        prop(point, "attenuation_radius", 450.0)
        prop(point, "cast_shadows", False)


def gameplay(world):
    blueprint_root = ROOT + "/Blueprints"
    world.get_world_settings().set_editor_property(
        "default_game_mode", bp(blueprint_root + "/BP_GameMode_Arena_Codex"))
    actor(unreal.PlayerStart, "PlayerStart", (-1300, 0, 100), folder="Gameplay")
    spawner = actor(bp(blueprint_root + "/BP_EnemySpawner_Codex"),
                    "EnemySpawner", (0, 0, 100), folder="Gameplay")
    prop(spawner, "grunt_class", bp(blueprint_root + "/BP_Grunt_Arena_Codex"), True)
    prop(spawner, "runner_class", bp(blueprint_root + "/BP_Runner_Arena_Codex"), True)
    prop(spawner, "minimum_player_distance", 1100.0, True)
    prop(spawner, "max_spawn_attempts", 12, True)
    spawn_class = bp(blueprint_root + "/BP_EnemySpawnPoint_Codex")
    points = [
        ("NW", (-2050, 1700, 100), -40),
        ("N", (0, 2150, 100), -90),
        ("NE", (2050, 1850, 100), -140),
        ("SE", (2100, -1850, 100), 140),
        ("S", (-200, -2150, 100), 90),
        ("SW", (-2050, -1750, 100), 40),
    ]
    for name, xyz, yaw in points:
        actor(spawn_class, "Spawn_" + name, xyz, yaw, "Gameplay/SpawnPoints")
    bounds = actor(unreal.NavMeshBoundsVolume, "NavMeshBounds", (0, 0, 180), folder="Navigation")
    bounds.set_actor_scale3d(unreal.Vector(26.0, 26.0, 4.0))
    # Explicit dynamic generation supports freshly imported geometry and Level Reload.
    navigation_data = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.RecastNavMesh)
    if navigation_data:
        recast = navigation_data[0]
        recast.set_actor_label(PREFIX + "RecastNavMesh")
        recast.set_folder_path("LastStand_Codex/Navigation")
    else:
        recast = actor(unreal.RecastNavMesh, "RecastNavMesh", (0, 0, 0), folder="Navigation")
    prop(recast, "runtime_generation", unreal.RuntimeGenerationType.DYNAMIC, True)
    prop(recast, "force_rebuild_on_load", True)
    prop(recast, "agent_height", 176.0)
    return points


def main():
    global MATERIALS
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is not None:
        raise RuntimeError("Stop PIE before building the Codex STEP 4 Arena")
    report = json.loads(IMPORT_REPORT.read_text(encoding="utf-8"))
    MATERIALS = {key: load(path) for key, path in report["logical_materials"].items()}
    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    existed = unreal.EditorAssetLibrary.does_asset_exist(LEVEL)
    if not (subsystem.load_level(LEVEL) if existed else subsystem.new_level(LEVEL)):
        raise RuntimeError(f"Cannot create/load isolated Arena: {LEVEL}")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    existing = actor_subsystem.get_all_level_actors()
    if existed and not any(a.get_actor_label().startswith(PREFIX) for a in existing):
        raise RuntimeError("Existing Arena has no Codex STEP4 ownership labels; refusing replacement")
    for value in existing:
        if value.get_actor_label().startswith(PREFIX):
            actor_subsystem.destroy_actor(value)
    ground()
    boundary()
    structures()
    props()
    lighting()
    points = gameplay(world)
    unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
    subsystem.save_current_level()
    # New NavMeshBounds/geometry are registered after this editor frame. Rebuild
    # once more after registration, then save only once six projections work.
    # No gameplay Tick or project-wide navigation setting is introduced.
    tick_state = {"frames": 0, "handle": None}
    def finish_navigation(delta):
        tick_state["frames"] += 1
        editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        current = editor_subsystem.get_editor_world()
        if current != world or editor_subsystem.get_game_world() is not None:
            unreal.unregister_slate_post_tick_callback(tick_state["handle"])
            unreal.log_warning("CODEX_STEP4_NAV_FINALIZE_CANCELLED: editor world changed or PIE started")
            return
        if tick_state["frames"] == 3:
            unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
        if tick_state["frames"] >= 10:
            ready = all(unreal.NavigationSystemV1.project_point_to_navigation(
                world, unreal.Vector(*xyz), None, None, unreal.Vector(250, 250, 300))
                is not None for _, xyz, _ in points)
            if ready or tick_state["frames"] >= 300:
                unreal.unregister_slate_post_tick_callback(tick_state["handle"])
                subsystem.save_current_level()
                unreal.log(f"CODEX_STEP4_NAV_FINALIZED ready={ready}")
    tick_state["handle"] = unreal.register_slate_post_tick_callback(finish_navigation)
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    output = {"level": LEVEL, "playable_size_cm": [5000, 5000],
              "created_actor_count": len(ACTORS), "actors": ACTORS,
              "spawn_points": [{"name": n, "position": list(p)} for n, p, _ in points],
              "optional_warnings": OPTIONAL_WARNINGS,
              "gameplay_code_modified": False, "preplaced_enemy_count": 0}
    (REPORT_DIR / "Step04_LevelReport.json").write_text(
        json.dumps(output, indent=2), encoding="utf-8")
    for warning in OPTIONAL_WARNINGS:
        unreal.log_warning("CODEX_STEP4_OPTIONAL " + warning)
    unreal.log(f"CODEX_STEP4_LEVEL_SUCCESS level={LEVEL} actors={len(ACTORS)} "
               f"spawnpoints={len(points)} enemies=0 size=5000x5000")


main()
