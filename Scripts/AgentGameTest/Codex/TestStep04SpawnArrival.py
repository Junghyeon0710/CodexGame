"""Editor-only AI traversal fixture, not a Spawn or normal gameplay test.

Usage: import TestStep04SpawnArrival as arrival; arrival.arm()
arm() may run before PIE; it waits for the Codex Arena and its first live,
Wave-tracked Enemy. The fixture teleports that SAME Enemy to each existing
SpawnPoint in turn and observes ordinary AI walking toward Player. It does not
spawn/destroy enemies, grant abilities, apply damage or change Wave counters.

TEST-ONLY MUTATIONS: Player is positioned at (-1300, 0, capsule-correct ~90cm)
and boosted to 9999 Health/MaxHealth through CodexDebugSetPlayerHealth. The chosen
Enemy is teleported once per point and its old movement request is stopped to
allow normal AI replanning. Other enemies and the game loop remain untouched.
No runtime navigation queries are issued from the Slate callback. The separate
editor audit supplies path-query evidence; this fixture only observes arrival.
No level is saved. Restart PIE afterward; fixture Health/positions are not
restored automatically. Successful traversal is NOT evidence of real spawning
at that point: use the existing Wave Spawn logs for that separate requirement.
"""

from datetime import datetime, timezone
import json
import math
from pathlib import Path
import re
import time
import traceback

import unreal


EXPECTED_LEVEL = "/Game/AgentGameTest/Codex/Levels/L_LastStand_Arena_Codex"
REGISTRY_KEY = "_codex_step04_spawn_arrival_fixture"
PLAYER_XY = (-1300.0, 0.0)
FLOOR_Z = 0.0
CAPSULE_FLOOR_CLEARANCE = 2.15
ARRIVAL_DISTANCE = 170.0
POINT_TIMEOUT = 20.0
WAIT_TIMEOUT = 120.0
SAMPLE_INTERVAL = 0.15


def _xyz(value):
    return [round(float(value.x), 3), round(float(value.y), 3), round(float(value.z), 3)]


def _distance(a, b):
    return math.hypot(a.x - b.x, a.y - b.y)


def _editor_navigation_evidence():
    """Snapshot existing editor evidence, never query runtime navigation here."""
    path = (Path(unreal.Paths.project_dir()).resolve()
            / "Saved/AgentComparison/Codex/Step04_EditorAudit.json")
    evidence = {"source": str(path), "kind": "separate_editor_audit",
                "status": "unavailable", "runtime_navigation_queried": False}
    try:
        audit = json.loads(path.read_text(encoding="utf-8"))
        if audit.get("kind") != "editor" or audit.get("expected_level") != EXPECTED_LEVEL:
            raise ValueError("Editor evidence does not identify the exact Codex Arena")
        checks = audit.get("checks", {})
        evidence.update(timestamp_utc=audit.get("timestamp_utc"),
                        status=audit.get("status"),
                        preplaced_enemies_zero=checks.get("preplaced_enemies_zero"),
                        enemy_count=audit.get("enemy_count"),
                        all_spawn_projections=checks.get("all_spawn_projections"),
                        all_spawn_paths_complete=checks.get("all_spawn_paths_complete"),
                        spawn_navigation=audit.get("spawn_navigation", []))
    except (OSError, ValueError) as error:
        evidence["unavailable_reason"] = str(error)
    return evidence


def _world_if_ready():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if world is None:
        return None
    package = world.get_path_name().split(".")[0]
    normalized = re.sub(r"/UEDPIE_\d+_", "/", package)
    if normalized != EXPECTED_LEVEL or not re.search(r"/UEDPIE_\d+_", package):
        raise RuntimeError(f"Refusing fixture outside exact Codex Arena PIE: {package}")
    return world


def _location_for(actor, x, y):
    capsule = actor.get_component_by_class(unreal.CapsuleComponent)
    if capsule is None:
        raise RuntimeError(f"Missing CapsuleComponent: {actor.get_name()}")
    height = float(capsule.get_scaled_capsule_half_height())
    return unreal.Vector(x, y, FLOOR_Z + height + CAPSULE_FLOOR_CLEARANCE)


def _movement(actor):
    result = actor.get_component_by_class(unreal.CharacterMovementComponent)
    if result is None:
        raise RuntimeError(f"Missing CharacterMovementComponent: {actor.get_name()}")
    return result


def _state_snapshot(state):
    return {"phase": str(state.get_game_phase()), "wave": int(state.get_current_wave()),
            "alive": int(state.get_alive_enemy_count()), "score": int(state.get_score()),
            "remaining": int(state.get_remaining_spawn_count()),
            "total_spawned": int(state.get_total_spawned_enemy_count())}


class _ArrivalFixture:
    def __init__(self):
        self.armed_wall = time.monotonic()
        self.last_sample_wall = 0.0
        self.handle = None
        self.running = False
        self.world = self.pc = self.player = self.state = self.enemy = None
        self.spawn_points = []
        self.point_index = -1
        self.point_wall = self.point_game = 0.0
        self.enemy_start = None
        stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S_%fZ")
        self.report_path = (Path(unreal.Paths.project_dir()).resolve()
            / "Saved/AgentComparison/Codex" / f"Step04_SpawnArrivalFixture_{stamp}.json")
        self.report = {
            "schema_version": 1, "agent": "Codex", "step": 4,
            "kind": "teleported_wave_enemy_ai_traversal_fixture",
            "armed_utc": datetime.now(timezone.utc).isoformat(), "status": "waiting_for_pie",
            "expected_level": EXPECTED_LEVEL, "world": None, "selected_enemy": None,
            "spawn_point_count_expected": 6, "arrival_distance_cm": ARRIVAL_DISTANCE,
            "per_point_timeout_wall_and_game_seconds": POINT_TIMEOUT,
            "wait_for_pie_and_enemy_timeout_wall_seconds": WAIT_TIMEOUT,
            "attempts": [], "errors": [], "health_boost_command": None,
            "editor_navigation_evidence": _editor_navigation_evidence(),
            "mutations": [], "initial_game_state": None, "final_game_state": None,
            "test_limitations": [
                "The same already Wave-spawned Enemy is teleported to six SpawnPoint locations.",
                "This proves AI traversal only, not actual spawning at those six points.",
                "Runtime arrival is independent of the separate editor path-query audit.",
                "No NavigationSystemV1 static query is executed from the Slate callback.",
                "Actual SpawnPoint use must be verified from separate Wave Spawn logs.",
                "Wave membership uses Spawner ownership, no-preplaced-enemy audit and alive count consistency; the private ActiveEnemies set is not inspected.",
                "Player Health/MaxHealth is boosted to 9999, so this is not normal GameOver or balance QA.",
                "Other Wave enemies remain live and may physically obstruct the selected Enemy.",
                "No Wave counter, phase, ability, damage, Enemy spawning or destruction is performed.",
                "Restart PIE after this fixture; modified runtime Health and positions are not restored.",
            ],
        }

    def _write(self):
        self.report["elapsed_wall_seconds"] = round(time.monotonic() - self.armed_wall, 3)
        self.report_path.parent.mkdir(parents=True, exist_ok=True)
        self.report_path.write_text(json.dumps(self.report, ensure_ascii=False, indent=2), encoding="utf-8")

    def _begin_when_ready(self, world):
        player = unreal.GameplayStatics.get_player_character(world, 0)
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        state = unreal.GameplayStatics.get_game_state(world)
        mode = unreal.GameplayStatics.get_game_mode(world)
        if player is None or pc is None or state is None or mode is None:
            return
        if not (isinstance(player, unreal.CodexLSPlayerCharacter)
                and isinstance(pc, unreal.CodexLSPlayerController)
                and isinstance(state, unreal.CodexLSGameState)
                and isinstance(mode, unreal.CodexLSGameMode)):
            raise RuntimeError("The PIE Player/Controller/GameState/GameMode is not the Codex game loop")
        if state.get_game_phase() in (unreal.CodexLSGamePhase.GAME_OVER, unreal.CodexLSGamePhase.VICTORY):
            raise RuntimeError("Fixture requires a live game; restart PIE before retrying")
        if state.get_game_phase() != unreal.CodexLSGamePhase.WAVE_IN_PROGRESS:
            return
        # ActiveEnemies is not exposed to Python. Possession changes Pawn Owner
        # from Spawner to Controller. Use Codex AI possession, the zero-preplaced
        # audit, alive-count agreement and matching native Spawn logs instead.
        evidence = self.report["editor_navigation_evidence"]
        if not (evidence.get("status") == "passed"
                and evidence.get("preplaced_enemies_zero") is True
                and evidence.get("enemy_count") == 0):
            raise RuntimeError("A passing Codex Arena editor audit with zero preplaced enemies is required")
        enemies = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CodexLSEnemyCharacter)
        live = [enemy for enemy in enemies if not enemy.is_dead()]
        candidates = [enemy for enemy in live if isinstance(enemy.get_controller(), unreal.CodexLSEnemyAIController)]
        if not candidates:
            return
        if len(candidates) != len(live) or len(candidates) != int(state.get_alive_enemy_count()):
            raise RuntimeError(f"Wave ownership/count mismatch: owned={len(candidates)} "
                               f"live={len(live)} state_alive={state.get_alive_enemy_count()}")
        points = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CodexLSEnemySpawnPoint))
        if len(points) != 6:
            raise RuntimeError(f"Expected exactly six existing SpawnPoints, found {len(points)}")
        if any(not point.get_actor_label().startswith("CodexLS4_Spawn_") for point in points):
            raise RuntimeError("A SpawnPoint does not belong to the Codex STEP 4 Arena fixture")
        self.world, self.pc, self.player, self.state = world, pc, player, state
        self.enemy = candidates[0]
        # Exercise the NW approach first on retry, before other live Grunts
        # gather around the stationary Player. Prior failed reports are retained.
        self.spawn_points = sorted(points, key=lambda point:
            (point.get_actor_label() != "CodexLS4_Spawn_NW", point.get_actor_label()))
        self.report["world"] = world.get_path_name()
        self.report["initial_game_state"] = _state_snapshot(state)
        self.report["selected_enemy"] = {"name": self.enemy.get_name(),
            "path": self.enemy.get_path_name(), "archetype": self.enemy.get_enemy_archetype_name(),
            "owner": self.enemy.get_owner().get_path_name(), "private_active_enemies_inspected": False,
            "owned_live_enemy_count": len(candidates), "all_live_enemy_count": len(live),
            "game_state_alive_enemy_count": int(state.get_alive_enemy_count()),
            "alive_counts_match": True, "editor_preplaced_enemy_count": evidence["enemy_count"],
            "selection": "first live Codex-AI-possessed Enemy, zero-preplaced audit and canonical alive-count agreement; correlate native Spawn logs",
            "log_correlation": {"spawn_marker": "CODEX_STEP3_SPAWN", "enemy": self.enemy.get_name(),
                                "snapshot_marker": "CODEX_STEP3_SNAPSHOT",
                                "status": "verify matching Spawn and Snapshot logs separately"}}
        unreal.SystemLibrary.execute_console_command(world, "CodexDebugGameLoopSnapshot", pc)
        unreal.log(f"CODEX_STEP4_ARRIVAL_SELECTION Enemy={self.enemy.get_name()} "
                   f"OwnedLive={len(candidates)} AllLive={len(live)} "
                   f"StateAlive={state.get_alive_enemy_count()} PrivateSetInspected=false")
        player_position = _location_for(player, *PLAYER_XY)
        player.set_actor_location(player_position, False, True)
        _movement(player).stop_movement_immediately()
        command = "CodexDebugSetPlayerHealth 9999"
        unreal.SystemLibrary.execute_console_command(world, command, pc)
        self.report["health_boost_command"] = command
        self.report["mutations"].append({"kind": "player_test_setup",
            "position_cm": _xyz(player_position), "health_and_max_health_requested": 9999})
        self.report["status"] = "running"
        unreal.log(f"CODEX_STEP4_ARRIVAL_FIXTURE_BEGIN enemy={self.enemy.get_name()} "
                   "points=6 test_only_health=9999 actual_spawn_test=false")
        self._next_point()

    def _next_point(self):
        self.point_index += 1
        if self.point_index >= len(self.spawn_points):
            self.finish("Completed")
            return
        point = self.spawn_points[self.point_index]
        point_position = point.get_actor_location()
        editor_entry = next((entry for entry in self.report["editor_navigation_evidence"].get(
            "spawn_navigation", []) if entry.get("label") == point.get_actor_label()), None)
        editor_reference = {"kind": "separate_editor_audit_reference",
            "runtime_navigation_queried": False, "matched_spawn_label": editor_entry is not None,
            "editor_entry": editor_entry}
        if editor_entry is not None:
            editor_xy = editor_entry.get("position_cm", [])
            editor_reference["position_matches_current_spawn"] = len(editor_xy) >= 2 and (
                math.hypot(editor_xy[0] - point_position.x, editor_xy[1] - point_position.y) <= 1.0)
        self.enemy_start = _location_for(self.enemy, point_position.x, point_position.y)
        controller = self.enemy.get_controller()
        if controller is None:
            raise RuntimeError("Selected Wave Enemy has no AI controller")
        controller.stop_movement()
        _movement(self.enemy).stop_movement_immediately()
        self.enemy.set_actor_location(self.enemy_start, False, True)
        self.point_wall = time.monotonic()
        self.point_game = float(unreal.GameplayStatics.get_time_seconds(self.world))
        attempt = {"index": self.point_index + 1, "spawn_point": point.get_name(),
            "spawn_point_label": point.get_actor_label(), "spawn_point_cm": _xyz(point_position),
            "enemy_reused": self.enemy.get_name(), "teleport_requested_cm": _xyz(self.enemy_start),
            "teleport_actual_cm": _xyz(self.enemy.get_actor_location()),
            "editor_navigation_reference": editor_reference,
            "status": "running", "arrived": False, "walking_observed": False,
            "minimum_distance_cm": None, "samples": [], "game_state_before": _state_snapshot(self.state)}
        self.report["attempts"].append(attempt)
        self.report["mutations"].append({"kind": "selected_enemy_teleport_and_stop_old_move",
            "attempt": attempt["index"], "position_cm": _xyz(self.enemy_start)})
        unreal.log(f"CODEX_STEP4_ARRIVAL_POINT_BEGIN point={point.get_actor_label()} "
                   f"enemy={self.enemy.get_name()} runtime_nav_query=false")
        self._write()

    def tick(self, unused_delta):
        if not self.running:
            return
        wall = time.monotonic()
        if wall - self.last_sample_wall < SAMPLE_INTERVAL:
            return
        self.last_sample_wall = wall
        try:
            world = _world_if_ready()
            if self.world is None:
                if wall - self.armed_wall > WAIT_TIMEOUT:
                    self.finish("WaitForPIEOrWaveEnemyTimeout")
                elif world is not None:
                    self._begin_when_ready(world)
                return
            if world != self.world:
                self.finish("PIEEndedOrWorldChanged")
                return
            if self.state.get_game_phase() in (unreal.CodexLSGamePhase.GAME_OVER, unreal.CodexLSGamePhase.VICTORY):
                self.finish("TerminalGamePhase")
                return
            if self.enemy.is_dead():
                self.finish("SelectedEnemyDied")
                return
            if unreal.GameplayStatics.get_player_character(world, 0) != self.player:
                self.finish("PlayerChanged")
                return
            position = self.enemy.get_actor_location()
            player_position = self.player.get_actor_location()
            distance = _distance(position, player_position)
            mode = _movement(self.enemy).get_editor_property("movement_mode")
            walking = mode == unreal.MovementMode.MOVE_WALKING
            game_seconds = float(unreal.GameplayStatics.get_time_seconds(world)) - self.point_game
            wall_seconds = wall - self.point_wall
            attempt = self.report["attempts"][-1]
            sample = {"wall_s": round(wall_seconds, 3), "game_s": round(game_seconds, 3),
                "enemy_cm": _xyz(position), "player_cm": _xyz(player_position),
                "distance_cm": round(distance, 3), "movement_mode": str(mode), "walking": walking,
                "velocity_cm_s": _xyz(self.enemy.get_velocity())}
            attempt["samples"].append(sample)
            attempt["walking_observed"] = attempt["walking_observed"] or walking
            attempt["minimum_distance_cm"] = min(distance, attempt["minimum_distance_cm"]
                if attempt["minimum_distance_cm"] is not None else distance)
            # Teleporting cannot satisfy arrival: the actor must travel from its
            # distant SpawnPoint, under normal movement, and be walking at arrival.
            arrived = distance <= ARRIVAL_DISTANCE and walking and _distance(position, self.enemy_start) > 500.0
            timed_out = wall_seconds >= POINT_TIMEOUT or game_seconds >= POINT_TIMEOUT
            if arrived or timed_out:
                attempt["arrived"] = arrived
                attempt["status"] = "passed" if arrived else "failed"
                attempt["stop_reason"] = "Arrived" if arrived else "Timeout"
                attempt["elapsed_wall_seconds"] = round(wall_seconds, 3)
                attempt["elapsed_game_seconds"] = round(game_seconds, 3)
                attempt["game_state_after"] = _state_snapshot(self.state)
                unreal.log(f"CODEX_STEP4_ARRIVAL_POINT_END point={attempt['spawn_point_label']} "
                           f"status={attempt['status']} arrived={arrived} distance={distance:.1f} "
                           f"walking={walking} wall={wall_seconds:.2f}")
                self._next_point()
        except Exception as error:
            self.report["errors"].append({"message": str(error), "traceback": traceback.format_exc()})
            self.finish("Error")

    def finish(self, reason):
        if not self.running:
            return self.report
        self.running = False
        if self.handle is not None:
            unreal.unregister_slate_post_tick_callback(self.handle)
            self.handle = None
        if self.report["attempts"] and self.report["attempts"][-1]["status"] == "running":
            self.report["attempts"][-1].update(status="incomplete", stop_reason=reason)
        self.report["ended_utc"] = datetime.now(timezone.utc).isoformat()
        self.report["stop_reason"] = reason
        self.report["attempt_count"] = len(self.report["attempts"])
        self.report["passed_count"] = sum(item["status"] == "passed" for item in self.report["attempts"])
        self.report["actual_arrivals"] = sum(item["arrived"] for item in self.report["attempts"])
        self.report["all_six_traversals_passed"] = (reason == "Completed"
            and self.report["attempt_count"] == 6 and self.report["passed_count"] == 6)
        self.report["status"] = "passed" if self.report["all_six_traversals_passed"] else "failed"
        self.report["callback_cleaned_up"] = self.handle is None
        try:
            if self.world is not None and _world_if_ready() == self.world:
                self.report["final_game_state"] = _state_snapshot(self.state)
        except Exception:
            pass
        self._write()
        unreal.log(f"CODEX_STEP4_ARRIVAL_FIXTURE_END reason={reason} "
                   f"passed={self.report['passed_count']}/6 report={self.report_path}")
        return self.report


def arm():
    """Wait for Arena PIE, then test one existing Wave Enemy from all six points."""
    old = getattr(unreal, REGISTRY_KEY, None)
    if old is not None and old.running:
        raise RuntimeError("The arrival fixture is already running; call stop() first")
    input_harness = getattr(unreal, "_codex_step04_arena_input_harness", None)
    if input_harness is not None and input_harness.running:
        raise RuntimeError("Stop the input-driven play harness before arming this isolated fixture")
    run = _ArrivalFixture()
    run.running = True
    try:
        run.handle = unreal.register_slate_post_tick_callback(run.tick)
        setattr(unreal, REGISTRY_KEY, run)
        run._write()
    except Exception:
        run.running = False
        if run.handle is not None:
            unreal.unregister_slate_post_tick_callback(run.handle)
        raise
    unreal.log(f"CODEX_STEP4_ARRIVAL_FIXTURE_ARMED wait_for_pie=true report={run.report_path}")
    return {"running": True, "status": "waiting_for_pie", "report_path": str(run.report_path)}


def stop():
    """Unregister this fixture's callback and save evidence without restarting PIE."""
    run = getattr(unreal, REGISTRY_KEY, None)
    return run.finish("ManualStop") if run is not None else {"running": False, "reason": "NotArmed"}


def status():
    """Return a compact read-only progress snapshot."""
    run = getattr(unreal, REGISTRY_KEY, None)
    if run is None:
        return {"running": False, "reason": "NotArmed"}
    return {"running": run.running, "status": run.report["status"],
            "point_attempt": run.point_index + 1,
            "completed_attempts": sum(item["status"] != "running" for item in run.report["attempts"]),
            "passed_count": sum(item["status"] == "passed" for item in run.report["attempts"]),
            "stop_reason": run.report.get("stop_reason"), "report_path": str(run.report_path)}
