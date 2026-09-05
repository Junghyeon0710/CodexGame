"""Editor-only barrier regression fixture; never used by Shipping gameplay.

Temporarily positions one live Wave enemy and Player on opposite sides of an
Arena barrier. Restarts the level, observes real GAS/AI/input, then unregisters.
It neither writes Health nor directly applies Damage. Does not save the level.
"""
import time
import unreal

_handle = None
_stage = 0
_start = 0.0
_enemy = None


def _cmd(world, command):
    unreal.SystemLibrary.execute_console_command(world, command,
        unreal.GameplayStatics.get_player_controller(world, 0))


def stop():
    global _handle
    if _handle is not None:
        unreal.unregister_slate_post_tick_callback(_handle)
        _handle = None


def _tick(delta):
    global _stage, _start, _enemy
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if world is None:
        return
    if 'L_LastStand_Arena_Codex' not in world.get_path_name():
        stop()
        raise RuntimeError('STEP4 wall test requires the Codex Arena')
    state = unreal.GameplayStatics.get_game_state(world)
    if state.get_game_phase() == unreal.CodexLSGamePhase.GAME_OVER:
        return
    if _stage == 0:
        enemies = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CodexLSEnemyCharacter)
        if not enemies:
            return
        player = unreal.GameplayStatics.get_player_character(world, 0)
        _enemy = enemies[0]
        player.set_actor_location(unreal.Vector(-222,-400,90.15), False, True)
        _enemy.set_actor_location(unreal.Vector(-78,-400,90.15), False, True)
        _enemy.character_movement.disable_movement()
        _cmd(world, 'CodexDebugCombatSnapshot')
        _cmd(world, 'CodexDebugAttackEnemy ' + _enemy.get_name())
        unreal.log('CODEX_STEP4_WALL_FIXTURE_BEGIN player=(-222,-400,90) enemy=(-78,-400,90) barrierHeight=112.5')
        _stage = 1
        _start = time.monotonic()
    elif _stage == 1 and time.monotonic() - _start >= 1.0:
        _cmd(world, 'CodexDebugCombatSnapshot')
        unreal.log('CODEX_STEP4_WALL_FIXTURE_AFTER_1S')
        _enemy.character_movement.set_movement_mode(unreal.MovementMode.MOVE_WALKING)
        _stage = 2
    elif _stage == 2 and time.monotonic() - _start >= 3.0:
        _cmd(world, 'CodexDebugCombatSnapshot')
        unreal.log('CODEX_STEP4_WALL_FIXTURE_END')
        stop()


def start():
    global _handle, _stage
    stop()
    _stage = 0
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if world is None or 'L_LastStand_Arena_Codex' not in world.get_path_name():
        raise RuntimeError('Start PIE in Codex Arena before running this fixture')
    _cmd(world, 'CodexDebugRestartGameLoop')
    _handle = unreal.register_slate_post_tick_callback(_tick)
