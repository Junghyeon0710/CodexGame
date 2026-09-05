"""PIE-only movement/dash collision fixtures; teleports are setup, never gameplay.

Boosts test HP using the existing GAS debug command. No game counters are edited.
The final normal gameplay run is separate and never uses these fixtures.
"""
import json
import math
import re
import time
import traceback
from datetime import datetime, timezone
from pathlib import Path
import unreal

EXPECTED_LEVEL = '/Game/AgentGameTest/Codex/Levels/L_LastStand_Arena_Codex'
WAIT_TIMEOUT = 120.0
CASE_WALL_TIMEOUT = 20.0
SAMPLE_INTERVAL = 0.05
CASES = [
    ("FenceWest", (-2250, 0), "S", "min_x", -2500),
    ("FenceEast", (2250, -400), "W", "max_x", 2500),
    ("FenceNorth", (0, 2250), "D", "max_y", 2500),
    ("FenceSouth", (-200, -2250), "A", "min_y", -2500),
    ("Container", (-850, 1200), "D", "max_y", 1430),
    ("CentralPipe", (-350, 0), "W", "max_x", 0),
    ("Barrier", (-1600, 950), "W", "max_x", -1400),
    ("OpenRoute", (-1000, -800), "W", "travel", 350),
]
_handle = None
_index = 0
_begin = 0.0
_active = False
_results = []
_world = None
_samples = []
_errors = []
_armed_wall = _begin_wall = _last_sample_wall = 0.0
_report_path = None
_start_position = None


def _cmd(world, text):
    unreal.SystemLibrary.execute_console_command(world, text,
        unreal.GameplayStatics.get_player_controller(world, 0))


def stop():
    """Release fixture input, unregister the callback, and preserve partial evidence."""
    return _finish('ManualStop') if _handle is not None else {'running': False}


def _finish(reason):
    global _handle, _active
    if _handle is not None:
        unreal.unregister_slate_post_tick_callback(_handle)
        _handle = None
    try:
        current = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if _world is not None and current == _world:
            _cmd(_world, 'CodexDebugInputChord NONE false 0.12')
    except Exception as error:
        _errors.append({'cleanup_error': str(error)})
    report = {'test_only_teleport_setup': True, 'test_only_health_boost': 9999,
        'expected_level': EXPECTED_LEVEL, 'runtime_navigation_queried': False,
        'stop_reason': reason, 'elapsed_wall_seconds': round(time.monotonic() - _armed_wall, 3),
        'results': _results, 'errors': _errors,
        'incomplete_case': CASES[_index][0] if _active and _index < len(CASES) else None,
        'incomplete_samples': _samples if _active else [],
        'passed': reason == 'Completed' and len(_results) == len(CASES)
                  and all(result['passed'] for result in _results) and not _errors,
        'limitations': ['Test setup teleports Player and boosts Health/MaxHealth.',
                        'Normal gameplay, AI navigation and visual camera QA are separate tests.',
                        'Dash is requested through the real input path; confirm activation in GAS logs.',
                        'Samples verify collision throughout movement, not only the ending position.',
                        'Other Wave enemies remain live and can physically obstruct movement.',
                        'Restart PIE after this fixture to restore normal runtime state.']}
    _active = False
    if _report_path is not None:
        _report_path.parent.mkdir(parents=True, exist_ok=True)
        text = json.dumps(report, indent=2)
        _report_path.write_text(text, encoding='utf-8')
        (_report_path.parent / 'Step04_Traversal.json').write_text(text, encoding='utf-8')
    unreal.log(f'CODEX_STEP4_TRAVERSAL_END reason={reason} passed={report["passed"]} report={_report_path}')
    return report


def _record_sample(player, now):
    pos = player.get_actor_location()
    velocity = player.get_velocity()
    sample = {'game_s': round(now - _begin, 3),
              'position_cm': [pos.x, pos.y, pos.z],
              'speed_2d_cm_s': math.hypot(velocity.x, velocity.y)}
    _samples.append(sample)
    return sample


def _tick(delta):
    global _index, _begin, _active, _world, _samples, _begin_wall, _last_sample_wall, _start_position
    wall = time.monotonic()
    if wall - _last_sample_wall < SAMPLE_INTERVAL:
        return
    _last_sample_wall = wall
    try:
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if _world is None and wall - _armed_wall >= WAIT_TIMEOUT:
            _finish('WaitForReadyPIETimeout')
            return
        if _world is not None and world != _world:
            _finish('PIEEndedOrWorldChanged')
            return
        if world is None:
            if wall - _armed_wall >= WAIT_TIMEOUT:
                _finish('WaitForPIETimeout')
            return
        package = world.get_path_name().split('.')[0]
        if (re.sub(r'/UEDPIE_\d+_', '/', package) != EXPECTED_LEVEL
                or not re.search(r'/UEDPIE_\d+_', package)):
            raise RuntimeError(f'Traversal fixture requires exact Codex Arena PIE: {package}')
        player = unreal.GameplayStatics.get_player_character(world, 0)
        state = unreal.GameplayStatics.get_game_state(world)
        if player is None or state is None:
            if wall - _armed_wall >= WAIT_TIMEOUT:
                _finish('WaitForPlayerTimeout')
            return
        if not isinstance(player, unreal.CodexLSPlayerCharacter) or not isinstance(state, unreal.CodexLSGameState):
            raise RuntimeError('Traversal requires the existing Codex Player and GameState')
        if state.get_game_phase() in (unreal.CodexLSGamePhase.GAME_OVER, unreal.CodexLSGamePhase.VICTORY):
            _finish('TerminalGamePhase')
            return
        now = float(unreal.GameplayStatics.get_time_seconds(world))
        if not _active:
            if now < 0.5:
                return
            _world = world
            name, xy, chord, rule, limit = CASES[_index]
            capsule = player.get_component_by_class(unreal.CapsuleComponent)
            movement = player.get_component_by_class(unreal.CharacterMovementComponent)
            if capsule is None or movement is None:
                raise RuntimeError('Player capsule or CharacterMovementComponent is missing')
            _cmd(world, 'CodexDebugSetPlayerHealth 9999')
            player.set_actor_location(unreal.Vector(xy[0], xy[1],
                capsule.get_scaled_capsule_half_height() + 2.15), False, True)
            movement.stop_movement_immediately()
            start = player.get_actor_location()
            if math.hypot(start.x - xy[0], start.y - xy[1]) > 1.0:
                raise RuntimeError(f'Player test setup teleport failed: {name}')
            _start_position = [start.x, start.y, start.z]
            _begin, _begin_wall, _active, _samples = now, wall, True, []
            _record_sample(player, now)
            _cmd(world, f'CodexDebugInputChord {chord} true 1.2')
            unreal.log('CODEX_STEP4_TRAVERSAL_BEGIN ' + name)
        else:
            sample = _record_sample(player, now)
            if now - _begin < 3.5:
                if wall - _begin_wall >= CASE_WALL_TIMEOUT:
                    _finish('CaseWallTimeout')
                return
            name, xy, chord, rule, limit = CASES[_index]
            pos = sample['position_cm']
            positions = [item['position_cm'] for item in _samples]
            travel = math.hypot(pos[0] - xy[0], pos[1] - xy[1])
            direction = {'W': (1, 0), 'S': (-1, 0), 'D': (0, 1), 'A': (0, -1)}[chord]
            max_progress = max((p[0] - xy[0]) * direction[0] + (p[1] - xy[1]) * direction[1]
                               for p in positions)
            checks = {'min_x': all(p[0] >= limit for p in positions),
                      'max_x': all(p[0] <= limit for p in positions),
                      'min_y': all(p[1] >= limit for p in positions),
                      'max_y': all(p[1] <= limit for p in positions), 'travel': travel >= limit}
            ground_ok = all(70 <= p[2] <= 130 for p in positions)
            movement_observed = max_progress >= 25.0
            result = {'case': name, 'start_xy': xy, 'setup_actual_cm': _start_position, 'end': pos,
                      'input': chord, 'dash_requested': True, 'travel_cm': travel,
                      'maximum_input_direction_progress_cm': max_progress,
                      'maximum_speed_2d_cm_s': max(item['speed_2d_cm_s'] for item in _samples),
                      'movement_observed': movement_observed, 'ground_height_valid': ground_ok,
                      'collision_or_travel_rule_passed': checks[rule],
                      'samples': _samples, 'elapsed_game_seconds': now - _begin,
                      'elapsed_wall_seconds': wall - _begin_wall,
                      'passed': checks[rule] and ground_ok and movement_observed}
            _results.append(result)
            unreal.log('CODEX_STEP4_TRAVERSAL_RESULT ' + json.dumps(
                {key: value for key, value in result.items() if key != 'samples'}))
            _index += 1
            _active = False
            if _index == len(CASES):
                _finish('Completed')
    except Exception as error:
        _errors.append({'message': str(error), 'traceback': traceback.format_exc()})
        _finish('Error')


def arm():
    global _handle, _index, _active, _results, _world, _samples, _errors, _armed_wall, _report_path
    for key in ('_codex_step04_arena_input_harness', '_codex_step04_spawn_arrival_fixture'):
        other = getattr(unreal, key, None)
        if other is not None and other.running:
            raise RuntimeError('Stop the other STEP 4 input/arrival harness before this isolated fixture')
    stop()
    _index, _active, _results, _world, _samples, _errors = 0, False, [], None, [], []
    _armed_wall = time.monotonic()
    stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S_%fZ')
    _report_path = (Path(unreal.Paths.project_dir()).resolve() / 'Saved/AgentComparison/Codex'
                   / f'Step04_Traversal_{stamp}.json')
    _handle = unreal.register_slate_post_tick_callback(_tick)
    unreal.log('CODEX_STEP4_TRAVERSAL_ARMED')
    return {'running': True, 'report_path': str(_report_path)}
