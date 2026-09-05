"""Read-only, editor-only STEP 4 geometry evidence; importing runs nothing.

Run after stopping PIE: import VerifyStep04Geometry as geometry; geometry.audit()
Only JSON evidence is written. No actor, asset, collision, navigation, camera,
input or editor setting is changed. Collision rays are not a visual QA pass.
"""

import datetime
import json
import math
from pathlib import Path
import traceback

import unreal


LEVEL = "/Game/AgentGameTest/Codex/Levels/L_LastStand_Arena_Codex"
PREFIX = "CodexLS4_"
MESH_ROOT = "/Game/AgentGameTest/Codex/Environment/Meshes/"
PROJECT = Path(unreal.Paths.project_dir()).resolve()
REPORT_DIR = PROJECT / "Saved/AgentComparison/Codex"
MANIFEST = PROJECT / "ExternalAssets/LastStand/Codex/Models/Step04BlenderKitManifest.json"
CAMERA_OFFSET = (-630.934, 0.0, 901.067)
PLAYER_ROOT_Z = 90.15
# Actual current placeholder cone apex: root90.15 + relativeZ(-35) + half55.
PLAYER_HEAD_Z = 110.15
PLAYER_BODY_Z = 80.0
PROBES = (
    ("PlayerStart", -1300.0, 0.0),
    ("NorthBlueEastContact", -504.0, 1680.0),
    ("NorthBlueEastNear", -480.0, 1680.0),
    ("NorthBlueEastFar", -390.0, 1680.0),
    ("NorthRedEastContact", 996.0, 1660.0),
    ("NorthRedEastNear", 1020.0, 1660.0),
    ("NorthRedEastFar", 1120.0, 1660.0),
    ("CentralPipeEastNear", 350.0, 0.0),
    ("CentralPipeEastFar", 450.0, 0.0),
    ("WarehouseEastOutside", 2850.0, 550.0),
    ("WarehouseNorthApproach", 2300.0, 920.0),
    ("LoopSouth", -800.0, -850.0),
    ("LoopWest", -2050.0, 0.0),
)


def _vec(value):
    return [float(value.x), float(value.y), float(value.z)]


def _label(actor):
    return actor.get_actor_label() if actor else None


def _world():
    subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if subsystem.get_game_world() is not None:
        raise RuntimeError("Stop PIE before the geometry audit; no editor navigation queries during PIE.")
    world = subsystem.get_editor_world()
    if world is None or world.get_path_name().split(".")[0] != LEVEL:
        raise RuntimeError("Geometry audit requires the exact Codex STEP 4 editor Arena; no map is loaded automatically.")
    return world


def _trace(world, start, end, ignored=(), complex_trace=False):
    # Local UE 5.8 KismetSystemLibrary.h, LineTraceSingle: world, start, end,
    # channel, complex, ignored, draw, OutHit, ignore_self, optional colors/time.
    # Python returns HitResult on success / None on no hit (OutHit is returned).
    # The live UE 5.8 Python enum exposes the named built-in Visibility value.
    result = unreal.SystemLibrary.line_trace_single(
        world, unreal.Vector(*start), unreal.Vector(*end),
        unreal.TraceTypeQuery.ECC_VISIBILITY, complex_trace, list(ignored),
        unreal.DrawDebugTrace.NONE, True)
    if result is None:
        return {"blocking_hit": False}
    if isinstance(result, (tuple, list)):
        if any(isinstance(value, bool) and not value for value in result):
            return {"blocking_hit": False}
        result = next((value for value in result if isinstance(value, unreal.HitResult)), None)
        if result is None:
            raise RuntimeError("Unexpected LineTraceSingle tuple; no HitResult found")
    # GameplayStatics.h BreakHitResult has these 18 fields. The live UE 5.8
    # Python binding exposes the native break function through HitResult's
    # to_tuple(), not GameplayStatics.break_hit_result.
    values = result.to_tuple()
    if len(values) != 18:
        raise RuntimeError(f"Unexpected BreakHitResult output count: {len(values)}")
    blocking, initial, fraction, distance = values[:4]
    actor, component = values[9:11]
    return {"blocking_hit": bool(blocking), "initial_overlap": bool(initial),
            "fraction": float(fraction), "distance_cm": float(distance),
            "location_cm": _vec(values[4]), "impact_point_cm": _vec(values[5]),
            "actor": _label(actor), "actor_path": actor.get_path_name() if actor else None,
            "component": component.get_name() if component else None}


def _support(world, actor, component, lowest_z, highest_z, foundation_joins=()):
    label = _label(actor)
    position = actor.get_actor_location()
    if component.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION:
        return {"classification": "nonblocking_detail_or_background", "checked": False,
                "reason": "Paint, drains and background intentionally need separate visual review."}
    if label.startswith(PREFIX + "Ground_"):
        return {"classification": "ground_top_pivot", "checked": True,
                "expected_top_z_cm": 0.0, "measured_top_z_cm": highest_z,
                "passed": abs(highest_z) <= 0.25}
    if label.startswith(PREFIX + "Curb_"):
        # Curbs straddle the ground's exact +/-2500cm edge. A ray exactly on
        # that triangle edge is numerically ambiguous; sample the adjacent
        # foundation 20cm toward the Arena instead. The outer curb overhang
        # and the warehouse foundation join remain intentional exceptions,
        # not a claim that every curb vertex has floor collision below it.
        x, y = float(position.x), float(position.y)
        if abs(x) >= 2490.0:
            x -= math.copysign(20.0, x)
        if abs(y) >= 2490.0:
            y -= math.copysign(20.0, y)
        ignored = [actor]
        for join in foundation_joins:
            center, extent = join.get_actor_bounds(False)
            if abs(x - center.x) <= extent.x and abs(y - center.y) <= extent.y:
                ignored.append(join)
        hit = _trace(world, (x, y, lowest_z + 8.0), (x, y, lowest_z - 35.0), ignored)
        ground_found = bool(hit["blocking_hit"] and (hit.get("actor") or "").startswith(PREFIX + "Ground_"))
        gap = lowest_z - hit["impact_point_cm"][2] if hit["blocking_hit"] else None
        return {"classification": "boundary_foundation_with_intentional_outer_overhang",
                "checked": True, "expected_bottom_z_cm": 0.0,
                "inward_trace_xy_cm": [x, y], "trace": hit, "gap_cm": gap,
                "ignored_intentional_foundation_joins": [_label(join) for join in ignored[1:]],
                "note": "Checks bottom z0 and nearby inward ground, not full support at the exact boundary; touching Warehouse foundation is intentional.",
                "passed": abs(lowest_z) <= 0.3 and ground_found and gap is not None and abs(gap) <= 0.5}
    # Pallet/crate stacks and curbs under fences are authored intentional supports.
    if label.startswith(PREFIX + "Crate_"):
        classification, expected_bottom, expected_support = "crate_on_pallet", 15.0, PREFIX + "Pallet_"
    elif label.startswith(PREFIX + "Fence_"):
        classification, expected_bottom, expected_support = "fence_embedded_3cm_in_curb", 22.0, PREFIX + "Curb_"
    else:
        classification, expected_bottom, expected_support = "ground_supported", 0.0, PREFIX + "Ground_"
    # Start above a 25cm curb; ignore only the object being examined. The pivot
    # XY is used deliberately (lamps have asymmetric render bounds).
    hit = _trace(world, (position.x, position.y, lowest_z + 8.0),
                 (position.x, position.y, lowest_z - 35.0), (actor,))
    gap = lowest_z - hit["impact_point_cm"][2] if hit["blocking_hit"] else None
    support_matches = bool(hit["blocking_hit"] and (hit.get("actor") or "").startswith(expected_support))
    # Expected fence embed is -3cm. This is a placement sanity test, not a proof
    # that every visible vertex or every corner is in contact with the floor.
    passed = (abs(lowest_z - expected_bottom) <= 0.3 and support_matches
              and gap is not None and -3.5 <= gap <= 0.5)
    return {"classification": classification, "checked": True,
            "expected_bottom_z_cm": expected_bottom, "expected_support_prefix": expected_support,
            "trace": hit, "gap_cm": gap, "passed": passed}


def _inventory(world, actors):
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8-sig"))
    authored = {entry["name"]: entry for entry in manifest["assets"]}
    mesh_editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    unique = {}
    items = []
    foundation_joins = [actor for actor in actors if _label(actor) == PREFIX + "Warehouse_Bay04"]
    for actor in sorted(actors, key=_label):
        if not _label(actor).startswith(PREFIX):
            continue
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            mesh = component.get_editor_property("static_mesh")
            if mesh is None or not mesh.get_path_name().startswith(MESH_ROOT):
                continue
            rotation = actor.get_actor_rotation()
            origin, extent = actor.get_actor_bounds(False)
            location = actor.get_actor_location()
            scale = actor.get_actor_scale3d()
            mesh_bounds = mesh.get_bounds()
            local_min = float(mesh_bounds.origin.z - mesh_bounds.box_extent.z)
            lowest, highest = float(origin.z - extent.z), float(origin.z + extent.z)
            yaw_only = abs(float(rotation.pitch)) <= 0.01 and abs(float(rotation.roll)) <= 0.01
            positive = all(math.isfinite(value) and value > 0.0 for value in _vec(scale))
            predicted_bottom = float(location.z + local_min * scale.z)
            item = {"actor": _label(actor), "mesh": mesh.get_path_name(),
                    "location_cm": _vec(location), "scale": _vec(scale),
                    "rotation_degrees": {"pitch": float(rotation.pitch), "yaw": float(rotation.yaw), "roll": float(rotation.roll)},
                    "yaw_only": yaw_only, "finite_positive_scale": positive,
                    "bounds_center_cm": _vec(origin), "bounds_extent_cm": _vec(extent),
                    "bottom_z_cm": lowest, "top_z_cm": highest,
                    "predicted_bottom_z_cm": predicted_bottom,
                    "pivot_bounds_match": yaw_only and abs(lowest - predicted_bottom) <= 0.3}
            item["support"] = _support(world, actor, component, lowest, highest, foundation_joins)
            items.append(item)
            key = mesh.get_name()
            if key not in unique:
                expected = authored.get(key)
                actual_dimensions = [value * 2.0 for value in _vec(mesh_bounds.box_extent)]
                actual_collision = int(mesh_editor.get_simple_collision_count(mesh)
                                       + mesh_editor.get_convex_collision_count(mesh))
                unique[key] = {"mesh": mesh.get_path_name(), "manifest_present": expected is not None,
                    "dimensions_cm": actual_dimensions, "simple_and_convex_collision_count": actual_collision,
                    "local_bottom_z_cm": local_min,
                    "manifest_pivot": expected.get("pivot") if expected else None,
                    "manifest_collision_count": expected.get("collision_count") if expected else None,
                    "manifest_dimensions_match": expected is not None and all(
                        abs(actual - target) <= 1.0 for actual, target in zip(actual_dimensions, expected["dimensions_cm"])),
                    "collision_present": actual_collision > 0}
    return items, list(unique.values())


def _camera_probes(world):
    probes = []
    for name, x, y in PROBES:
        inside = abs(x) < 2450.0 and abs(y) < 2450.0
        item = {"name": name, "requested_xy_cm": [x, y], "inside_playable_boundary": inside}
        if not inside:
            item.update({"tested": False, "reason": "Warehouse +X side is outside the playable boundary; not treated as a reachable camera failure."})
            probes.append(item)
            continue
        projected = unreal.NavigationSystemV1.project_point_to_navigation(
            world, unreal.Vector(x, y, 80.0), None, None, unreal.Vector(30.0, 30.0, 180.0))
        if not isinstance(projected, unreal.Vector):
            item.update({"tested": False, "navigation_projection": False,
                         "reason": "No nearby navigation position; not treated as a reachable camera failure."})
            probes.append(item)
            continue
        shift = math.hypot(projected.x - x, projected.y - y)
        item.update({"navigation_projection": True, "projected_cm": _vec(projected), "projection_xy_shift_cm": shift})
        if shift > 15.0:
            item.update({"tested": False, "reason": "Projection moved over 15cm; requested standing position was not verified navigable."})
            probes.append(item)
            continue
        # Arena floor is z=0. Nav voxel height is not a physical floor height;
        # retain the measured standing root/head heights rather than adding it.
        camera = (x + CAMERA_OFFSET[0], y + CAMERA_OFFSET[1], PLAYER_ROOT_Z + CAMERA_OFFSET[2])
        head = (x, y, PLAYER_HEAD_Z)
        body = (x, y, PLAYER_BODY_Z)
        simple = _trace(world, camera, head)
        complex_result = _trace(world, camera, head, complex_trace=True)
        body_simple = _trace(world, camera, body)
        body_complex = _trace(world, camera, body, complex_trace=True)
        item.update({"tested": True, "camera_cm": list(camera), "head_cm": list(head),
                     "simple_visibility_trace": simple, "complex_visibility_trace": complex_result,
                     "potential_head_occlusion": bool(simple["blocking_hit"] or complex_result["blocking_hit"]),
                     "body_cm": list(body), "body_simple_visibility_trace": body_simple,
                     "body_complex_visibility_trace": body_complex,
                     "potential_body_occlusion": bool(body_simple["blocking_hit"] or body_complex["blocking_hit"])})
        probes.append(item)
    return probes


def audit():
    """Return/save conservative geometry evidence for the exact loaded editor map."""
    world = _world()
    timestamp = datetime.datetime.now(datetime.timezone.utc)
    report = {"schema_version": 1, "agent": "Codex", "step": 4,
              "timestamp_utc": timestamp.isoformat(), "level": LEVEL,
              "world": world.get_path_name(), "status": "running", "issues": [], "checks": {},
              "scope": "Read-only editor snapshot; writes only this JSON evidence.",
              "api_source": ["UE_5.8 KismetSystemLibrary.h LineTraceSingle", "UE_5.8 GameplayStatics.h BreakHitResult", "UE_5.8 CollisionProfile.cpp built-in trace mapping"],
              "caveats": ["No proof of visual quality, texture stretching or absence of Z-fighting.",
                          "Bounds and pivot-center support rays are sanity checks, not exhaustive triangle contact tests.",
                          "Camera probes use the fixed current camera offset and actual placeholder cone apex z110.15cm; they do not render a frame.",
                          "An unobstructed apex ray proves neither whole-body visibility nor readable player silhouette. Body z80cm rays are reported separately.",
                          "An apex or body ray hit is potential occlusion requiring actual player-camera visual review.",
                          "Navigation projection tests are editor-only snapshots, not actual walking or Dash."]}
    try:
        actors = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor))
        items, assets = _inventory(world, actors)
        probes = _camera_probes(world)
        report.update({"mesh_actor_count": len(items), "mesh_actors": items, "unique_meshes": assets,
                       "camera_probes": probes})
        supports = [item["support"] for item in items if item["support"]["checked"]]
        tested = [item for item in probes if item.get("tested")]
        report["checks"] = {
            "own_mesh_actors_present": len(items) > 0,
            "all_owned_mesh_actors_yaw_only": all(item["yaw_only"] for item in items),
            "all_positive_finite_scale": all(item["finite_positive_scale"] for item in items),
            "all_pivot_bounds_match": all(item["pivot_bounds_match"] for item in items),
            "checked_supports_match_authored_expectations": bool(supports) and all(item["passed"] for item in supports),
            "all_meshes_have_manifest_and_dimensions_match": bool(assets) and all(item["manifest_present"] and item["manifest_dimensions_match"] for item in assets),
            "all_meshes_have_simple_or_convex_collision": bool(assets) and all(item["collision_present"] for item in assets),
            "reachable_camera_probes_tested": len(tested) > 0,
            "no_potential_head_occlusion_in_tested_probes": bool(tested) and not any(item["potential_head_occlusion"] for item in tested)}
        report["status"] = "passed_geometry_checks_only" if all(report["checks"].values()) else "needs_review"
    except Exception as error:
        report["status"] = "error"
        report["issues"].append({"message": str(error), "traceback": traceback.format_exc()})
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    serialized = json.dumps(report, ensure_ascii=False, indent=2)
    (REPORT_DIR / "Step04_GeometryAudit.json").write_text(serialized, encoding="utf-8")
    archive = REPORT_DIR / "Step04_AuditSnapshots"
    archive.mkdir(parents=True, exist_ok=True)
    (archive / ("geometry_" + timestamp.strftime("%Y%m%dT%H%M%S_%fZ") + ".json")).write_text(serialized, encoding="utf-8")
    unreal.log("CODEX_STEP4_GEOMETRY_AUDIT " + json.dumps({"status": report["status"], "checks": report["checks"], "issues": report["issues"]}))
    return report
