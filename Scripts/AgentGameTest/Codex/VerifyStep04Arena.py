"""Read-only, one-shot STEP 4 editor/runtime audits; importing does not run them.

Example: import VerifyStep04Arena as qa; qa.audit_editor(); qa.audit_runtime()
The only writes are JSON evidence under Saved/AgentComparison/Codex. No actors,
assets, navigation, Gameplay state, input, timers or editor settings are changed.
Navigation paths prove snapshot connectivity, not actual Enemy arrival or play.
"""

from collections import Counter
import datetime
import json
import math
from pathlib import Path
import re
import traceback

import unreal


EXPECTED_LEVEL = "/Game/AgentGameTest/Codex/Levels/L_LastStand_Arena_Codex"
ENVIRONMENT_ROOT = "/Game/AgentGameTest/Codex/Environment/"
PROJECT = Path(unreal.Paths.project_dir()).resolve()
REPORT_DIRECTORY = PROJECT / "Saved/AgentComparison/Codex"
PROJECTION_EXTENT = unreal.Vector(250.0, 250.0, 300.0)


def _vector(value):
    return [float(value.x), float(value.y), float(value.z)]


def _label(actor):
    return actor.get_actor_label()


def _checked_world(runtime):
    subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = subsystem.get_game_world() if runtime else subsystem.get_editor_world()
    if world is None:
        raise RuntimeError("STEP 4 audit needs an active PIE world" if runtime else "Editor world is unavailable")
    package = world.get_path_name().split(".")[0]
    # PIE duplicates only prepend UEDPIE_n_ to the map leaf, not its package folder.
    normalized = re.sub(r"/UEDPIE_\d+_", "/", package)
    if normalized != EXPECTED_LEVEL:
        raise RuntimeError(f"STEP 4 audit stopped: expected {EXPECTED_LEVEL}, got {package}")
    return world


def _new_report(kind, world):
    return {
        "schema_version": 1, "agent": "Codex", "step": 4, "kind": kind,
        "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "expected_level": EXPECTED_LEVEL, "world": world.get_path_name(),
        "status": "running", "issues": [], "checks": {},
        "scope": "One-shot read-only state/assets/navigation queries; no Tick polling.",
        "not_proven_by_this_audit": ["actual AI arrival", "full Wave/Victory flow",
             "human-visible texture quality", "camera occlusion", "movement and Dash collision"],
    }


def _finish(report):
    if report["status"] == "running":
        report["status"] = "passed" if all(report["checks"].values()) and not report["issues"] else "failed"
    REPORT_DIRECTORY.mkdir(parents=True, exist_ok=True)
    filename = f"Step04_{report['kind'].title()}Audit.json"
    serialized = json.dumps(report, ensure_ascii=False, indent=2)
    (REPORT_DIRECTORY / filename).write_text(serialized, encoding="utf-8")
    archive_directory = REPORT_DIRECTORY / "Step04_AuditSnapshots"
    archive_directory.mkdir(parents=True, exist_ok=True)
    stamp = report["timestamp_utc"].replace(":", "-").replace("+", "_")
    (archive_directory / f"{report['kind']}_{stamp}.json").write_text(serialized, encoding="utf-8")
    unreal.log(f"CODEX_STEP4_{report['kind'].upper()}_AUDIT " + json.dumps(report, ensure_ascii=False, separators=(",", ":")))
    return report


def _project(world, point):
    # NavigationSystem.h: K2_ProjectPointToNavigation returns bool + FVector out;
    # Python exposes FVector on success / None on failure. Accept a tuple wrapper
    # as well, without treating a missing projection as a valid original point.
    result = unreal.NavigationSystemV1.project_point_to_navigation(
        world, point, None, None, PROJECTION_EXTENT)
    if result is None:
        return None
    if isinstance(result, unreal.Vector):
        return result
    if isinstance(result, (tuple, list)):
        if any(isinstance(item, bool) and not item for item in result):
            return None
        for item in result:
            if isinstance(item, unreal.Vector):
                return item
    raise RuntimeError(f"Unrecognized navigation projection return type: {type(result).__name__}")


def _path(world, start, end, context=None):
    path = unreal.NavigationSystemV1.find_path_to_location_synchronously(
        world, start, end, context, None)
    if path is None:
        return {"valid": False, "partial": None, "connected": False, "path_points": [], "path_length_cm": None}
    valid = bool(path.is_valid())
    partial = bool(path.is_partial())
    # NavigationPath.h exposes PathPoints as BlueprintReadOnly; no debug draw or
    # automatic path recalculation is enabled by this audit.
    points = [_vector(point) for point in path.get_editor_property("path_points")]
    return {"valid": valid, "partial": partial, "connected": valid and not partial,
            "path_points": points, "path_length_cm": float(path.get_path_length())}


def _spawn_navigation(world, spawn_points, destination, context=None):
    destination_projected = _project(world, destination)
    items = []
    for spawn in sorted(spawn_points, key=_label):
        position = spawn.get_actor_location()
        entry = {"name": spawn.get_name(), "label": _label(spawn), "position_cm": _vector(position)}
        try:
            projected = _project(world, position)
            entry["projection_success"] = projected is not None
            entry["projected_cm"] = _vector(projected) if projected is not None else None
            entry["destination_projected_cm"] = _vector(destination_projected) if destination_projected is not None else None
            if projected is not None and destination_projected is not None:
                entry["path"] = _path(world, projected, destination_projected, context)
            else:
                entry["path"] = {"valid": False, "partial": None, "connected": False, "path_points": []}
        except Exception as error:
            entry["error"] = str(error)
            entry["projection_success"] = False
            entry["path"] = {"valid": False, "partial": None, "connected": False, "path_points": []}
        items.append(entry)
    return items


def _static_mesh_inventory(actors):
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    inventory = []
    assets = {}
    missing_meshes = []
    missing_materials = []
    fallback_materials = []
    collision_failures = []
    scale_failures = []
    for actor in sorted(actors, key=_label):
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            scale = component.get_world_scale()
            if any(not math.isfinite(value) or value <= 0 for value in _vector(scale)):
                scale_failures.append({"actor": _label(actor), "component": component.get_name(), "scale": _vector(scale)})
            mesh = component.get_editor_property("static_mesh")
            if mesh is None:
                missing_meshes.append({"actor": _label(actor), "component": component.get_name()})
                continue
            path = mesh.get_path_name()
            owned = path.startswith(ENVIRONMENT_ROOT)
            collision = component.get_collision_enabled()
            if path not in assets:
                extent = mesh.get_bounds().box_extent
                assets[path] = {"asset": path, "codex_step4": owned,
                    "dimensions_cm": [float(extent.x * 2), float(extent.y * 2), float(extent.z * 2)],
                    "simple_collision_count": int(subsystem.get_simple_collision_count(mesh)
                                                  + subsystem.get_convex_collision_count(mesh)),
                    "uv_channels": int(subsystem.get_num_uv_channels(mesh, 0)),
                    "vertices_lod0": int(subsystem.get_number_verts(mesh, 0))}
            if collision != unreal.CollisionEnabled.NO_COLLISION and assets[path]["simple_collision_count"] <= 0:
                collision_failures.append({"actor": _label(actor), "mesh": path, "collision": str(collision)})
            materials = []
            for index in range(component.get_num_materials()):
                material = component.get_material(index)
                material_path = material.get_path_name() if material else None
                materials.append(material_path)
                if material is None:
                    missing_materials.append({"actor": _label(actor), "slot": index, "mesh": path})
                elif owned and material_path.startswith("/Engine/EngineMaterials/DefaultMaterial"):
                    fallback_materials.append({"actor": _label(actor), "slot": index, "material": material_path})
            inventory.append({"actor": _label(actor), "component": component.get_name(), "mesh": path,
                "position_cm": _vector(actor.get_actor_location()), "world_scale": _vector(scale),
                "collision": str(collision), "materials": materials})
    return {"components": inventory, "unique_meshes": list(assets.values()),
            "component_count": len(inventory), "unique_mesh_count": len(assets),
            "missing_meshes": missing_meshes, "missing_materials": missing_materials,
            "default_fallback_materials": fallback_materials,
            "collision_failures": collision_failures, "invalid_scales": scale_failures,
            "scale_note": "Positive finite scale only; intentional markings/background scales are reported, not normalized."}


def audit_editor():
    """Inspect the loaded Arena without loading, saving or changing any level."""
    world = _checked_world(False)
    report = _new_report("editor", world)
    try:
        actors = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor))
        spawns = [actor for actor in actors if isinstance(actor, unreal.CodexLSEnemySpawnPoint)]
        enemies = [actor for actor in actors if isinstance(actor, unreal.CodexLSEnemyCharacter)]
        starts = [actor for actor in actors if isinstance(actor, unreal.PlayerStart)]
        spawners = [actor for actor in actors if isinstance(actor, unreal.CodexLSEnemySpawner)]
        landscape_class = unreal.load_class(None, "/Script/Landscape.LandscapeProxy")
        landscapes = list(unreal.GameplayStatics.get_all_actors_of_class(world, landscape_class)) if landscape_class else []
        report["actor_count"] = len(actors)
        report["actor_counts_by_class"] = dict(sorted(Counter(actor.get_class().get_name() for actor in actors).items()))
        report["spawn_point_count"] = len(spawns)
        report["enemy_count"] = len(enemies)
        report["player_start_count"] = len(starts)
        report["spawner_count"] = len(spawners)
        report["landscape_count"] = len(landscapes)
        report["checks"].update({"spawn_points_exactly_six": len(spawns) == 6,
            "preplaced_enemies_zero": len(enemies) == 0, "player_start_exactly_one": len(starts) == 1,
            "spawner_exactly_one": len(spawners) == 1, "landscape_zero": landscape_class is not None and len(landscapes) == 0})
        if starts:
            destination = starts[0].get_actor_location()
            report["player_start"] = {"label": _label(starts[0]), "position_cm": _vector(destination)}
            report["spawn_navigation"] = _spawn_navigation(world, spawns, destination)
        else:
            report["spawn_navigation"] = []
        report["checks"]["all_spawn_projections"] = len(report["spawn_navigation"]) == 6 and all(
            item["projection_success"] for item in report["spawn_navigation"])
        report["checks"]["all_spawn_paths_complete"] = len(report["spawn_navigation"]) == 6 and all(
            item["path"]["connected"] for item in report["spawn_navigation"])
        meshes = _static_mesh_inventory(actors)
        report["static_meshes"] = meshes
        report["checks"].update({"no_missing_meshes": not meshes["missing_meshes"],
            "no_missing_materials": not meshes["missing_materials"],
            "no_default_fallback_materials": not meshes["default_fallback_materials"],
            "blocking_meshes_have_simple_collision": not meshes["collision_failures"],
            "finite_positive_scales": not meshes["invalid_scales"],
            "environment_mesh_uv0_present": all(item["uv_channels"] >= 1 for item in meshes["unique_meshes"] if item["codex_step4"])})
    except Exception as error:
        report["status"] = "error"
        report["issues"].append({"message": str(error), "traceback": traceback.format_exc()})
    return _finish(report)


def audit_runtime():
    """Snapshot current PIE GameState/Player/Enemy/path data without mutations."""
    world = _checked_world(True)
    report = _new_report("runtime", world)
    try:
        player = unreal.GameplayStatics.get_player_character(world, 0)
        state = unreal.GameplayStatics.get_game_state(world)
        if player is None or not isinstance(player, unreal.CodexLSPlayerCharacter):
            raise RuntimeError("PIE Player is missing or is not CodexLSPlayerCharacter")
        if state is None or not isinstance(state, unreal.CodexLSGameState):
            raise RuntimeError("PIE GameState is missing or is not CodexLSGameState")
        destination = player.get_actor_location()
        report["player"] = {"name": player.get_name(), "position_cm": _vector(destination),
                            "velocity_cm_s": _vector(player.get_velocity())}
        report["game_state"] = {"game_phase": str(state.get_game_phase()),
            "current_wave": int(state.get_current_wave()), "max_wave": int(state.get_max_wave()),
            "alive_enemy_count": int(state.get_alive_enemy_count()),
            "total_spawned_enemy_count": int(state.get_total_spawned_enemy_count()),
            "remaining_spawn_count": int(state.get_remaining_spawn_count()), "score": int(state.get_score())}
        enemies = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CodexLSEnemyCharacter))
        details = []
        for enemy in sorted(enemies, key=lambda value: value.get_name()):
            dead = bool(enemy.is_dead())
            position = enemy.get_actor_location()
            controller = enemy.get_controller()
            ai_state = controller.get_enemy_ai_state_name() if isinstance(controller, unreal.CodexLSEnemyAIController) else None
            item = {"name": enemy.get_name(), "type": enemy.get_enemy_archetype_name(),
                "dead": dead, "health": float(enemy.get_health()), "max_health": float(enemy.get_max_health()),
                "position_cm": _vector(position), "velocity_cm_s": _vector(enemy.get_velocity()),
                "distance_to_player_cm": math.hypot(position.x - destination.x, position.y - destination.y),
                "ai_state": ai_state, "controller": controller.get_name() if controller else None}
            item["path_to_player"] = _path(world, position, destination, enemy) if not dead else None
            details.append(item)
        live = [item for item in details if not item["dead"]]
        report["enemies"] = details
        report["enemy_counts"] = {"actors_total": len(details), "alive": len(live), "dead_pending_destroy": len(details) - len(live),
            "alive_by_type": dict(Counter(item["type"] for item in live)),
            "all_by_type": dict(Counter(item["type"] for item in details))}
        spawns = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CodexLSEnemySpawnPoint))
        report["spawn_point_count"] = len(spawns)
        report["checks"] = {"spawn_points_exactly_six": len(spawns) == 6,
            "canonical_alive_count_matches": len(live) == report["game_state"]["alive_enemy_count"],
            "counts_non_negative": all(report["game_state"][name] >= 0 for name in
                ("alive_enemy_count", "total_spawned_enemy_count", "remaining_spawn_count", "score")),
            "wave_in_range": 0 <= report["game_state"]["current_wave"] <= report["game_state"]["max_wave"] == 3,
            "living_enemies_have_controller": all(item["controller"] is not None for item in live),
            "living_enemy_paths_complete": all(item["path_to_player"]["connected"] for item in live),
            "player_inside_boundary": abs(destination.x) <= 2500.0 and abs(destination.y) <= 2500.0}
        report["navigation_note"] = "One synchronous path per live Enemy; dead actors are intentionally skipped. No AI movement is issued."
    except Exception as error:
        report["status"] = "error"
        report["issues"].append({"message": str(error), "traceback": traceback.format_exc()})
    return _finish(report)
