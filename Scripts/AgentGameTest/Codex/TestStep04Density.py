"""Editor-only 16-Enemy density fixture. Import does not execute.

Uses debug Health9999 and GAS force-defeat for Wave1/2 setup, then leaves all16
Wave3 enemies alive while real movement/Dash inputs run for20seconds. This is
not the normal Victory test. It never writes Wave/Score/Enemy counters.
"""
import json
import math
from pathlib import Path
import time
import unreal
import PlayStep04Arena as play

_handle = None
_report = None
_run = None
_dense_start = None
_last = 0.0
_start = 0.0
_world = None


def _cmd(command):
    unreal.SystemLibrary.execute_console_command(_world, command,
        unreal.GameplayStatics.get_player_controller(_world, 0))


def _write():
    path = Path(unreal.Paths.project_dir()).resolve() / 'Saved/AgentComparison/Codex/Step04_Density.json'
    path.write_text(json.dumps(_report, indent=2), encoding='utf-8')


def stop(reason='ManualStop'):
    global _handle
    if _handle is not None:
        unreal.unregister_slate_post_tick_callback(_handle)
        _handle = None
    if _run is not None and _run.running:
        _run.finish('DensityFixtureComplete')
    if _report is not None:
        _report['status'] = reason
        _report['passed'] = reason == 'Completed' and len(_report['samples']) >= 10 and all(
            s['alive'] == 16 and s['remaining'] == 0 and s['types'] == {'Grunt':10,'Runner':6}
            and s['outside_bounds'] == 0 for s in _report['samples'])
        _write()
        unreal.log('CODEX_STEP4_DENSITY_END ' + json.dumps({k:v for k,v in _report.items() if k!='samples'}))


def _tick(delta):
    global _last, _dense_start, _run
    if time.monotonic() - _start > 100:
        stop('Timeout'); return
    if time.monotonic() - _last < 0.7:
        return
    _last = time.monotonic()
    try:
        current=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if current != _world:
            stop('WorldChanged'); return
        state=unreal.GameplayStatics.get_game_state(_world)
        wave=state.get_current_wave()
        if state.get_game_phase() in (unreal.CodexLSGamePhase.VICTORY,unreal.CodexLSGamePhase.GAME_OVER):
            stop('UnexpectedTerminal'); return
        if state.get_game_phase()!=unreal.CodexLSGamePhase.WAVE_IN_PROGRESS or state.get_remaining_spawn_count()!=0:
            return
        if wave<3:
            _report['setup_forced_wave_defeats'].append(wave)
            _cmd('CodexDebugDefeatEnemies All -1'); return
        if _dense_start is None:
            _dense_start=unreal.GameplayStatics.get_time_seconds(_world)
            play.start()
            _run=getattr(unreal,play.REGISTRY_KEY)
            _run._attack=lambda *args: None
            _run.report['test_fixture_context']='Health9999, setup Wave1/2 forced GAS kills; attacks disabled for16Enemy density test'
            _cmd('CodexDebugGameLoopSnapshot')
        enemies=[e for e in unreal.GameplayStatics.get_all_actors_of_class(_world,unreal.CodexLSEnemyCharacter) if not e.is_dead()]
        types={kind:sum(e.get_enemy_archetype_name()==kind for e in enemies) for kind in ('Grunt','Runner')}
        player=unreal.GameplayStatics.get_player_character(_world,0)
        pos=player.get_actor_location()
        locations=[e.get_actor_location() for e in enemies]
        minimum=min((math.hypot(a.x-b.x,a.y-b.y) for i,a in enumerate(locations) for b in locations[i+1:]),default=0)
        elapsed=unreal.GameplayStatics.get_time_seconds(_world)-_dense_start
        _report['samples'].append({'game_s':elapsed,'alive':state.get_alive_enemy_count(),'remaining':state.get_remaining_spawn_count(),
            'types':types,'player_cm':[pos.x,pos.y,pos.z],'minimum_enemy_center_distance_cm':minimum,
            'outside_bounds':sum(abs(p.x)>2500 or abs(p.y)>2500 or p.z<60 for p in locations+[pos])})
        _write()
        if len(_report['samples'])==7:
            _cmd('HighResShot 1 filename="D:/Unreal Projects/CodexGame/Docs/AgentComparison/Codex/Evidence/Step04_Density.png"')
        if elapsed>=20:
            stop('Completed')
    except Exception as error:
        _report['errors'].append(str(error)); stop('Error')


def start():
    global _handle,_report,_world,_dense_start,_start,_last,_run
    if _handle is not None:
        raise RuntimeError('Density fixture already running')
    _world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if _world is None or '/UEDPIE_0_L_LastStand_Arena_Codex.' not in _world.get_path_name():
        raise RuntimeError('Exact Codex Arena PIE is required')
    _start=time.monotonic(); _last=0; _dense_start=None; _run=None
    _report={'kind':'test_only_16_enemy_density','health_boost':9999,'setup_forced_wave_defeats':[],
             'attacks_during_density':False,'samples':[],'errors':[],'status':'running'}
    _cmd('CodexDebugSetPlayerHealth 9999')
    _handle=unreal.register_slate_post_tick_callback(_tick)
