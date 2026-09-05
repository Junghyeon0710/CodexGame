"""Editor-only STEP 4 input-driven Arena play harness. Importing does not run it.

Usage in Unreal Editor while the Codex Arena is already running in PIE:
    import PlayStep04Arena as play
    play.start()
    play.status()
    play.stop()

The only gameplay mutations are existing WASD/Space/primary-attack input Execs
and PlayerController mouse-position input for aim correction.
No health changes, teleports, forced deaths, phase/counter writes or restarts are
performed. Static bounds, screen projection and state reads guide those inputs.
No NavigationSystem query is made from a runtime Slate callback.
Attack/dash counts mean INPUT REQUESTS, not confirmed ability activations or hits.
"""

from collections import Counter
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import re
import time
import traceback

import unreal


EXPECTED_LEVEL = "/Game/AgentGameTest/Codex/Levels/L_LastStand_Arena_Codex"
REGISTRY_KEY = "_codex_step04_arena_input_harness"
WAYPOINTS = ((-1100.0, -800.0), (900.0, -800.0),
             (900.0, 800.0), (-1100.0, 800.0))
DECISION_INTERVAL = 0.15
GAME_INTERVAL = 0.15
MOVEMENT_HOLD = 0.48
PATH_REFRESH = 1.05
ATTACK_INTERVAL = 0.32  # Existing GAS primary-attack cooldown is 0.3s.
ATTACK_RANGE = 1700.0  # Existing ability range is 1800cm; allow an aiming margin.
DASH_DANGER_DISTANCE = 520.0
DASH_REQUEST_COOLDOWN = 3.15  # Existing GAS dash cooldown remains authoritative.
SCREEN_MARGIN = 28.0
MAX_WALL_SECONDS = 300.0
PLAYER_SPEED = 500.0
BODY_CLEARANCE = 49.0  # Existing Player capsule radius42 plus small planning margin.
ARENA_LIMIT = 2420.0
DIRECTIONS = (("W", 1.0, 0.0), ("WD", 0.7071, 0.7071), ("D", 0.0, 1.0),
              ("SD", -0.7071, 0.7071), ("S", -1.0, 0.0), ("SA", -0.7071, -0.7071),
              ("A", 0.0, -1.0), ("WA", 0.7071, -0.7071))


def _xyz(vector):
    return [round(float(vector.x), 2), round(float(vector.y), 2), round(float(vector.z), 2)]


def _distance(a, b):
    return math.hypot(a.x - b.x, a.y - b.y)


def _phase_name(phase):
    normalized = re.sub(r"[^A-Z]", "", str(phase).upper())
    for key, label in (("WAVEINPROGRESS", "WaveInProgress"), ("WAVECLEAR", "WaveClear"),
                       ("GAMEOVER", "GameOver"), ("PREPARING", "Preparing"),
                       ("VICTORY", "Victory"), ("NONE", "None")):
        if key in normalized:
            return label
    return str(phase)


def _checked_context():
    subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = subsystem.get_game_world()
    if world is None:
        raise RuntimeError("STEP 4 play harness requires an active PIE world.")
    package = world.get_path_name().split(".")[0]
    normalized = re.sub(r"/UEDPIE_\d+_", "/", package)
    if normalized != EXPECTED_LEVEL or not re.search(r"/UEDPIE_\d+_", package):
        raise RuntimeError(f"Refusing input outside Codex Arena PIE: {package}")
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    player = unreal.GameplayStatics.get_player_character(world, 0)
    state = unreal.GameplayStatics.get_game_state(world)
    if not isinstance(pc, unreal.CodexLSPlayerController):
        raise RuntimeError("PIE controller is not CodexLSPlayerController.")
    if not isinstance(player, unreal.CodexLSPlayerCharacter):
        raise RuntimeError("PIE player is not CodexLSPlayerCharacter.")
    if not isinstance(state, unreal.CodexLSGameState):
        raise RuntimeError("PIE state is not CodexLSGameState.")
    return world, pc, player, state


def _screen_position(pc, position):
    result = pc.project_world_location_to_screen(position, True)
    if isinstance(result, unreal.Vector2D):
        return result
    if isinstance(result, (tuple, list)):
        if any(isinstance(item, bool) and not item for item in result):
            return None
        return next((item for item in result if isinstance(item, unreal.Vector2D)), None)
    return None


def _obstacle_bounds(world):
    """Read actual PIE collision bounds once; no NavigationSystem CDO calls."""
    bounds = []
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor):
        if not actor.get_actor_label().startswith("CodexLS4_"):
            continue
        result = actor.get_actor_bounds(True, False)
        if not isinstance(result, (tuple, list)) or len(result) != 2:
            raise RuntimeError(f"Unexpected Actor bounds result for {actor.get_name()}: {result}")
        origin, extent = result
        # Ground/paint/drains/low curbs do not block a standing capsule. Background
        # actors with no collision return zero extent and are excluded as well.
        if extent.x <= 0 or extent.y <= 0 or origin.z + extent.z < 45.0 or origin.z - extent.z > 170.0:
            continue
        bounds.append({"label": actor.get_actor_label(),
                       "box_xy": [origin.x - extent.x, origin.y - extent.y,
                                  origin.x + extent.x, origin.y + extent.y]})
    return bounds


def _box_clearance(x, y, box):
    left, bottom, right, top = box
    outside_x, outside_y = max(left - x, 0.0, x - right), max(bottom - y, 0.0, y - top)
    if outside_x or outside_y:
        return math.hypot(outside_x, outside_y)
    return -min(x - left, right - x, y - bottom, top - y)


def _clear_distance(position, dx, dy, obstacles, maximum=600.0):
    """Conservative sampled capsule corridor; this guides input, not collision."""
    initial = [_box_clearance(position.x, position.y, item["box_xy"]) for item in obstacles]
    distance = 0.0
    while distance < maximum:
        next_distance = min(maximum, distance + 35.0)
        x, y = position.x + dx * next_distance, position.y + dy * next_distance
        if max(abs(x), abs(y)) > ARENA_LIMIT:
            break
        blocked = False
        for item, start_clearance in zip(obstacles, initial):
            clearance = _box_clearance(x, y, item["box_xy"])
            # A capsule already close to an AABB may move away from it. Requiring
            # full initial clearance here would trap the planner beside a wall.
            if clearance < BODY_CLEARANCE and clearance <= start_clearance + 1.0:
                blocked = True
                break
        if blocked:
            break
        distance = next_distance
    return distance


def _optional_player_health(player):
    """Read existing AttributeSet data only; never substitute a presumed HP value."""
    try:
        player_state = player.get_editor_property("player_state")
        attributes = player_state.get_editor_property("attribute_set")
        attribute = attributes.get_editor_property("health")
        return float(attribute.get_editor_property("current_value"))
    except Exception:
        return None


class _ArenaInputRun:
    def __init__(self, timeout_seconds):
        self.world, self.pc, self.player, self.state = _checked_context()
        self.world_path = self.world.get_path_name()
        self.started_wall = time.monotonic()
        self.started_game = float(unreal.GameplayStatics.get_time_seconds(self.world))
        self.last_wall = self.started_wall - DECISION_INTERVAL
        self.last_game = self.started_game - GAME_INTERVAL
        self.last_path_game = -1000.0
        self.last_dash_game = -1000.0
        self.last_attack_game = -1000.0
        self.last_aim_game = -1000.0
        self.aim_target = None
        self.aim_until_game = -1000.0
        self.last_move_chord = None
        self.last_move_game = -1000.0
        self.last_move_hold = 0.0
        self.last_move_direction = (0.0, 0.0)
        self.stuck_since_game = None
        self.previous_position = self.player.get_actor_location()
        self.obstacles = _obstacle_bounds(self.world)
        self.timeout_seconds = max(1.0, min(float(timeout_seconds), MAX_WALL_SECONDS))
        self.handle = None
        self.running = False
        self.path_points = []
        self.direction = 1
        self.last_phase = None
        self.last_wave = None
        self.last_checkpoint_wall = self.started_wall
        self.stall_anchor = self.player.get_actor_location()
        self.stall_game = self.started_game
        self.observed_enemies = {}
        self.warnings_once = set()
        position = self.player.get_actor_location()
        self.waypoint_index = min(range(len(WAYPOINTS)), key=lambda index:
            math.hypot(position.x - WAYPOINTS[index][0], position.y - WAYPOINTS[index][1]))
        stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S_%fZ")
        directory = Path(unreal.Paths.project_dir()).resolve() / "Saved/AgentComparison/Codex"
        self.report_path = directory / f"Step04_AutoPlay_{stamp}.json"
        self.report = {
            "schema_version": 2, "agent": "Codex", "step": 4,
            "kind": "input_driven_arena_play", "started_utc": datetime.now(timezone.utc).isoformat(),
            "world": self.world_path, "expected_level": EXPECTED_LEVEL,
            "status": "running", "stop_reason": None,
            "maximum_wall_seconds": self.timeout_seconds,
            "decision_wall_interval_seconds": DECISION_INTERVAL,
            "minimum_game_interval_seconds": GAME_INTERVAL,
            "movement_hold_game_seconds": MOVEMENT_HOLD,
            "waypoints_xy_cm": [list(point) for point in WAYPOINTS],
            "allowed_execs": ["CodexDebugInputChord", "CodexDebugAttackEnemy"],
            "allowed_input_apis": ["PlayerController.set_mouse_location"],
            "gameplay_bypasses_used": [],
            "route_policy": "Authored loop is a guide, not a claimed NavMesh path. Eight-direction local steering uses actual static bounds and live Enemy position/velocity reads. No runtime NavigationSystem CDO calls.",
            "aim_policy": "Mouse projects to Player root-height aim plane, with short velocity lead, and refreshes while the existing 0.06s input press is pending. No Ability or AimDirection writes.",
            "static_obstacle_bounds": self.obstacles,
            "counter_meaning": "Shots and dashes are queued real-input requests, not confirmed hits or activations.",
            "timing_policy": "Movement decisions at >=0.15s; unchanged chords held continuously. Separate >=0.32s attack gate respects existing0.3s GAS cooldown and0.06/0.12s input timers.",
            "initial": {}, "final": {}, "phase_changes": [], "wave_maximums": {},
            "trajectory": [], "inputs": [], "shots": [], "navigation": [], "warnings": [],
            "errors": [], "dash_calls": 0, "input_calls": 0, "shot_calls": 0,
            "waypoints_reached": 0, "stall_replans": 0,
            "mouse_aim_updates": 0, "steering_decisions": [],
        }

    def _warning(self, key, message):
        if key not in self.warnings_once:
            self.warnings_once.add(key)
            self.report["warnings"].append(message)
            unreal.log_warning(f"CODEX_STEP4_AUTOPLAY {message}")

    def _elapsed(self, game_time):
        return {"wall_s": round(time.monotonic() - self.started_wall, 3),
                "game_s": round(game_time - self.started_game, 3)}

    def _send(self, command):
        # This narrow allowlist cannot issue health, transform, wave or kill Execs.
        valid = (re.fullmatch(r"CodexDebugInputChord (?:[WASD]{1,2}|NONE) (?:true|false) \d+\.\d+", command)
                 or re.fullmatch(r"CodexDebugAttackEnemy [A-Za-z0-9_]+", command))
        if not valid:
            raise RuntimeError(f"Command outside input-only allowlist: {command}")
        unreal.SystemLibrary.execute_console_command(self.world, command, self.pc)

    def _snapshot(self, game_time, enemies):
        phase, wave = _phase_name(self.state.get_game_phase()), int(self.state.get_current_wave())
        position = self.player.get_actor_location()
        snapshot = dict(self._elapsed(game_time), phase=phase, wave=wave,
            max_wave=int(self.state.get_max_wave()), alive=int(self.state.get_alive_enemy_count()),
            remaining=int(self.state.get_remaining_spawn_count()),
            total_spawned=int(self.state.get_total_spawned_enemy_count()), score=int(self.state.get_score()),
            player_location_cm=_xyz(position), player_velocity_cm_s=_xyz(self.player.get_velocity()),
            player_health=_optional_player_health(self.player), waypoint_index=self.waypoint_index)
        if phase != self.last_phase or wave != self.last_wave:
            self.report["phase_changes"].append(dict(snapshot, previous_phase=self.last_phase,
                                                      previous_wave=self.last_wave))
            unreal.log(f"CODEX_STEP4_AUTOPLAY_PHASE {phase} wave={wave} score={snapshot['score']}")
            self.last_phase, self.last_wave = phase, wave
        living = [enemy for enemy in enemies if not enemy.is_dead()]
        by_type = Counter(enemy.get_enemy_archetype_name() for enemy in living)
        snapshot["observed_alive_by_type"] = dict(by_type)
        snapshot["nearest_enemy_cm"] = round(min((_distance(position, enemy.get_actor_location())
                                                 for enemy in living), default=-1), 2)
        wave_key = str(wave)
        maxima = self.report["wave_maximums"].setdefault(wave_key, {
            "max_alive": 0, "max_remaining": 0, "max_total_spawned": 0,
            "max_score": 0, "max_alive_by_type": {}, "observed_spawned_by_type": {}})
        for key, value in (("max_alive", snapshot["alive"]), ("max_remaining", snapshot["remaining"]),
                           ("max_total_spawned", snapshot["total_spawned"]), ("max_score", snapshot["score"])):
            maxima[key] = max(maxima[key], value)
        for kind, count in by_type.items():
            maxima["max_alive_by_type"][kind] = max(maxima["max_alive_by_type"].get(kind, 0), count)
        for enemy in enemies:
            identity = enemy.get_path_name()
            if identity not in self.observed_enemies:
                kind = enemy.get_enemy_archetype_name()
                self.observed_enemies[identity] = {"name": enemy.get_name(), "wave_first_seen": wave,
                                                   "type": kind, "first_seen": self._elapsed(game_time)}
                counts = maxima["observed_spawned_by_type"]
                counts[kind] = counts.get(kind, 0) + 1
        self.report["trajectory"].append(snapshot)
        if not self.report["initial"]:
            self.report["initial"] = dict(snapshot)
        self.report["final"] = dict(snapshot)
        return snapshot, living

    def _refresh_path(self, game_time, position):
        if self.path_points and game_time - self.last_path_game < PATH_REFRESH:
            return
        self.last_path_game = game_time
        destination_xy = WAYPOINTS[self.waypoint_index]
        destination = unreal.Vector(destination_xy[0], destination_xy[1], position.z)
        entry = dict(self._elapsed(game_time), waypoint_index=self.waypoint_index,
                     from_cm=_xyz(position), destination_xy_cm=list(destination_xy),
                     source="authored_open_loop", runtime_nav_query=False)
        # This is a preferred destination only. Actual local steering below
        # checks live geometry bounds and enemies before choosing WASD input.
        self.path_points = [destination]
        entry["path_points_cm"] = [_xyz(point) for point in self.path_points]
        self.report["navigation"].append(entry)

    def _movement(self, game_time, position, enemies):
        destination = WAYPOINTS[self.waypoint_index]
        if math.hypot(position.x - destination[0], position.y - destination[1]) < 145:
            self.waypoint_index = (self.waypoint_index + self.direction) % len(WAYPOINTS)
            self.report["waypoints_reached"] += 1
            self.path_points = []
        moved = _distance(position, self.previous_position)
        self.previous_position = position
        if moved < 12.0 and self.last_move_chord not in (None, "NONE"):
            if self.stuck_since_game is None:
                self.stuck_since_game = game_time
        else:
            self.stuck_since_game = None
        stuck = self.stuck_since_game is not None and game_time - self.stuck_since_game >= 0.45
        if stuck and game_time - self.stall_game >= 0.75:
            self.direction *= -1
            self.waypoint_index = (self.waypoint_index + self.direction) % len(WAYPOINTS)
            self.path_points = []
            self.report["stall_replans"] += 1
            self.stall_game = game_time
        self._refresh_path(game_time, position)
        destination = self.path_points[0]
        goal_x, goal_y = destination.x - position.x, destination.y - position.y
        goal_length = max(math.hypot(goal_x, goal_y), 1.0)
        goal_x, goal_y = goal_x / goal_length, goal_y / goal_length
        near = min((_distance(position, enemy.get_actor_location()) for enemy in enemies), default=math.inf)
        threats = [(enemy.get_actor_location(), enemy.get_velocity(), enemy.get_enemy_archetype_name())
                   for enemy in enemies if _distance(position, enemy.get_actor_location()) < 1100.0]
        options = []
        for chord, dx, dy in DIRECTIONS:
            corridor = _clear_distance(position, dx, dy, self.obstacles)
            if corridor < 100.0:
                continue
            score = 1.8 * (goal_x * dx + goal_y * dy)
            score += 0.30 * (self.last_move_direction[0] * dx + self.last_move_direction[1] * dy)
            score += min(corridor, 450.0) / 900.0
            predicted_min = math.inf
            for enemy_position, velocity, archetype in threats:
                for horizon in (0.25, 0.55):
                    length = min(PLAYER_SPEED * horizon, max(corridor - 35.0, 0.0))
                    future_x, future_y = position.x + dx * length, position.y + dy * length
                    enemy_x = enemy_position.x + velocity.x * horizon
                    enemy_y = enemy_position.y + velocity.y * horizon
                    separation = math.hypot(future_x - enemy_x, future_y - enemy_y)
                    predicted_min = min(predicted_min, separation)
                    danger = max(0.0, (420.0 - separation) / 420.0)
                    score -= danger * danger * (9.0 if archetype == "Runner" else 7.0)
                    if separation < 155.0:
                        score -= 7.0
            if threats:
                score += min(predicted_min, 650.0) / 220.0
            if stuck and chord == self.last_move_chord:
                score -= 8.0
            options.append({"chord": chord, "dx": dx, "dy": dy, "score": score,
                            "corridor_cm": corridor, "predicted_nearest_cm": predicted_min})
        if options:
            best = max(options, key=lambda option: option["score"])
            chord, dx, dy = best["chord"], best["dx"], best["dy"]
            # Dash is requested before melee contact, only into a clear static
            # corridor and a direction with improving separation. Existing GAS
            # cooldown/collision still decides activation and final movement.
            dash = (near <= DASH_DANGER_DISTANCE and best["corridor_cm"] >= 350.0
                    and best["predicted_nearest_cm"] > near + 35.0
                    and game_time - self.last_dash_game >= DASH_REQUEST_COOLDOWN)
            hold = min(MOVEMENT_HOLD, max(0.16, (best["corridor_cm"] - 45.0) / PLAYER_SPEED))
            self.last_move_direction = (dx, dy)
        else:
            best = None
            chord, hold, dash = "NONE", 0.12, False
        self.report["steering_decisions"].append(dict(self._elapsed(game_time), chord=chord,
            stuck=stuck, nearest_enemy_cm=round(near, 2) if math.isfinite(near) else None,
            corridor_cm=best["corridor_cm"] if best else 0,
            predicted_nearest_cm=round(best["predicted_nearest_cm"], 2)
                if best and math.isfinite(best["predicted_nearest_cm"]) else None))
        # Avoid letting a0.34s key hold expire before a0.35s decision as in the
        # earlier harness. Continuous input is renewed before its timer ends.
        if (chord == self.last_move_chord and not dash
                and game_time - self.last_move_game < max(0.10, self.last_move_hold - 0.13)):
            return
        command = f"CodexDebugInputChord {chord} {'true' if dash else 'false'} {hold:.2f}"
        self._send(command)
        self.last_move_chord, self.last_move_game = chord, game_time
        self.last_move_hold = hold
        self.report["input_calls"] += 1
        self.report["inputs"].append(dict(self._elapsed(game_time), chord=chord, dash=dash,
            hold_game_seconds=round(hold, 2), location_cm=_xyz(position), command=command))
        if dash:
            self.last_dash_game = game_time
            self.report["dash_calls"] += 1

    def _attack(self, game_time, position, enemies):
        if game_time - self.last_attack_game < ATTACK_INTERVAL:
            return
        viewport = self.pc.get_viewport_size()
        if not isinstance(viewport, (tuple, list)) or len(viewport) < 2:
            raise RuntimeError(f"Unexpected viewport-size response: {viewport}")
        width, height = float(viewport[0]), float(viewport[1])
        if width <= SCREEN_MARGIN * 2 or height <= SCREEN_MARGIN * 2:
            self._warning("viewport", "PIE viewport is unavailable or too small; attacks are not queued off-screen.")
            return
        distance_ordered = sorted(enemies, key=lambda enemy: _distance(position, enemy.get_actor_location()))
        def priority(enemy):
            distance = _distance(position, enemy.get_actor_location())
            urgency = distance - (220.0 if enemy.get_enemy_archetype_name() == "Runner" else 0.0)
            return urgency + float(enemy.get_health()) * 1.5
        ordered = sorted(distance_ordered, key=priority)
        for enemy in ordered:
            enemy_position = enemy.get_actor_location()
            distance = _distance(position, enemy_position)
            if distance > ATTACK_RANGE:
                continue
            screen = _screen_position(self.pc, unreal.Vector(enemy_position.x, enemy_position.y, position.z))
            if screen is None or not (SCREEN_MARGIN <= screen.x <= width - SCREEN_MARGIN
                                      and SCREEN_MARGIN <= screen.y <= height - SCREEN_MARGIN):
                continue
            name = enemy.get_name()
            if not re.fullmatch(r"[A-Za-z0-9_]+", name):
                self._warning("actor_name", "An actor name is not safe for the input Exec argument and was skipped.")
                continue
            # C++ uses NameContains even when passed the full name. Verify its
            # nearest matching result is precisely the selected visible Actor.
            matching = [item for item in distance_ordered if name.upper() in
                        (item.get_name() + " " + item.get_enemy_archetype_name()).upper()]
            if not matching or matching[0] != enemy:
                continue
            line_of_sight = None
            try:
                line_of_sight = bool(self.pc.line_of_sight_to(enemy,
                    unreal.Vector(position.x, position.y, position.z), False))
            except AttributeError:
                self._warning("los_api", "LineOfSightTo is not exposed; visible in-range targets still use normal collision-tested attacks.")
            if line_of_sight is False:
                continue
            command = f"CodexDebugAttackEnemy {name}"
            self._send(command)
            self.last_attack_game = game_time
            self.aim_target, self.aim_until_game = enemy, game_time + 0.13
            self._update_aim(game_time)
            self.report["shot_calls"] += 1
            self.report["shots"].append(dict(self._elapsed(game_time), target=name,
                target_type=enemy.get_enemy_archetype_name(), target_location_cm=_xyz(enemy_position),
                target_health_before=float(enemy.get_health()), distance_cm=round(distance, 2),
                screen_xy=[round(float(screen.x), 2), round(float(screen.y), 2)],
                viewport_xy=[int(width), int(height)], line_of_sight=line_of_sight,
                full_name_selector_verified=True, command=command))
            return

    def _update_aim(self, game_time):
        if self.aim_target is None or game_time > self.aim_until_game:
            self.aim_target = None
            return
        if game_time - self.last_aim_game < 0.012 or self.aim_target.is_dead():
            return
        player_position = self.player.get_actor_location()
        enemy_position = self.aim_target.get_actor_location()
        velocity = self.aim_target.get_velocity()
        # UpdateMouseAim intersects the mouse ray with Player ROOT height, not
        # EnemyRoot+40. Projecting the same plane avoids a systematic offset.
        aim = unreal.Vector(enemy_position.x + velocity.x * 0.025,
                            enemy_position.y + velocity.y * 0.025, player_position.z)
        screen = _screen_position(self.pc, aim)
        width, height = self.pc.get_viewport_size()
        if screen is not None and 2 <= screen.x < width - 2 and 2 <= screen.y < height - 2:
            self.pc.set_mouse_location(int(round(screen.x)), int(round(screen.y)))
            self.last_aim_game = game_time
            self.report["mouse_aim_updates"] += 1

    def tick(self, unused_delta_seconds):
        if not self.running:
            return
        wall = time.monotonic()
        if wall - self.started_wall >= self.timeout_seconds:
            self.finish("Timeout")
            return
        try:
            world, pc, player, state = _checked_context()
            if world != self.world or pc != self.pc or player != self.player:
                self.finish("PIEWorldChanged")
                return
            game_time = float(unreal.GameplayStatics.get_time_seconds(world))
            # A queued0.06s attack must receive mouse correction on intervening
            # frames, rather than waiting for the next0.15s movement decision.
            self._update_aim(game_time)
            if wall - self.last_wall < DECISION_INTERVAL:
                return
            if game_time - self.last_game < GAME_INTERVAL:
                return
            self.last_wall, self.last_game = wall, game_time
            enemies = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CodexLSEnemyCharacter))
            snapshot, living = self._snapshot(game_time, enemies)
            if snapshot["phase"] in ("Victory", "GameOver"):
                self.finish(snapshot["phase"])
                return
            position = player.get_actor_location()
            self._movement(game_time, position, living)
            if snapshot["phase"] == "WaveInProgress":
                self._attack(game_time, position, living)
            if wall - self.last_checkpoint_wall >= 10:
                self._write_report()
                self.last_checkpoint_wall = wall
        except Exception as error:
            self.report["errors"].append({"message": str(error), "traceback": traceback.format_exc()})
            self.finish("Error")

    def _write_report(self):
        self.report["elapsed_wall_seconds"] = round(time.monotonic() - self.started_wall, 3)
        self.report["observed_enemies"] = list(self.observed_enemies.values())
        self.report_path.parent.mkdir(parents=True, exist_ok=True)
        self.report_path.write_text(json.dumps(self.report, ensure_ascii=False, indent=2), encoding="utf-8")

    def finish(self, reason):
        if not self.running:
            return self.report
        self.running = False
        if self.handle is not None:
            unreal.unregister_slate_post_tick_callback(self.handle)
            self.handle = None
        try:
            world, pc, player, state = _checked_context()
            if world == self.world and pc == self.pc:
                # NONE releases held WASD/Space through the existing helper.
                # A previously queued attack may finish its own <=0.12s timer;
                # no further shots are queued and no ability state is modified.
                self._send("CodexDebugInputChord NONE false 0.12")
        except Exception as error:
            self._warning("release", f"Input release skipped because PIE ended or changed: {error}")
        self.report["status"] = "finished"
        self.report["stop_reason"] = reason
        self.report["ended_utc"] = datetime.now(timezone.utc).isoformat()
        seen_waves = {entry["wave"] for entry in self.report["phase_changes"]
                      if entry["phase"] == "WaveInProgress"}
        self.report["observed_full_victory_flow"] = reason == "Victory" and {1, 2, 3}.issubset(seen_waves)
        self._write_report()
        unreal.log(f"CODEX_STEP4_AUTOPLAY_STOP reason={reason} inputs={self.report['input_calls']} "
                   f"shots={self.report['shot_calls']} dashes={self.report['dash_calls']} "
                   f"report={self.report_path}")
        return self.report


def start(timeout_seconds=MAX_WALL_SECONDS):
    """Start input-only play in the current Codex Arena PIE; never starts PIE itself."""
    existing = getattr(unreal, REGISTRY_KEY, None)
    if existing is not None and existing.running:
        raise RuntimeError("A STEP 4 input harness is already running. Call stop() first.")
    run = _ArenaInputRun(timeout_seconds)
    run.running = True
    try:
        run.handle = unreal.register_slate_post_tick_callback(run.tick)
        setattr(unreal, REGISTRY_KEY, run)
        run._write_report()
    except Exception:
        run.running = False
        if run.handle is not None:
            unreal.unregister_slate_post_tick_callback(run.handle)
        raise
    unreal.log(f"CODEX_STEP4_AUTOPLAY_START input_only=true report={run.report_path}")
    return {"running": True, "world": run.world_path, "report_path": str(run.report_path)}


def stop():
    """Unregister the callback, release injected movement, and save accumulated QA."""
    run = getattr(unreal, REGISTRY_KEY, None)
    return run.finish("ManualStop") if run is not None else {"running": False, "reason": "NotStarted"}


def status():
    """Return a compact status without starting play or changing gameplay state."""
    run = getattr(unreal, REGISTRY_KEY, None)
    if run is None:
        return {"running": False, "reason": "NotStarted"}
    return {"running": run.running, "elapsed_wall_seconds": round(time.monotonic() - run.started_wall, 2),
            "last_snapshot": run.report["final"], "inputs": run.report["input_calls"],
            "shots_requested": run.report["shot_calls"], "dashes_requested": run.report["dash_calls"],
            "stop_reason": run.report["stop_reason"], "report_path": str(run.report_path)}


def arm():
    """Wait for a newly started PIE so tool latency cannot consume Player HP."""
    started = time.monotonic()
    holder = []
    def wait_for_pie(delta):
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if world is not None and unreal.GameplayStatics.get_time_seconds(world) > 0.4:
            unreal.unregister_slate_post_tick_callback(holder[0])
            start()
        elif time.monotonic() - started > 60:
            unreal.unregister_slate_post_tick_callback(holder[0])
            unreal.log_warning("CODEX_STEP4_AUTOPLAY_ARM_TIMEOUT")
    holder.append(unreal.register_slate_post_tick_callback(wait_for_pie))
    unreal.log("CODEX_STEP4_AUTOPLAY_ARMED")
