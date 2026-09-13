"""Compile and validate PROJECT: LAST STAND STEP 6 deliverable assets.

This script is intentionally read-mostly: Blueprint compilation is persisted so
the report reflects the exact packages that will be opened by PIE.
"""

from __future__ import annotations

import json
import struct
import wave
from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir())
REPORT_PATH = PROJECT_ROOT / "Saved" / "AgentGameTest" / "Codex" / "Step06" / "AssetValidationReport.json"
WAV_ROOT = PROJECT_ROOT / "Saved" / "AgentGameTest" / "Codex" / "Step06" / "AudioSource"

BLUEPRINT_ASSETS = (
    "/Game/AgentGameTest/Codex/Abilities/GA_Enemy_MeleeAttack",
    "/Game/AgentGameTest/Codex/Abilities/GA_Player_Dash",
    "/Game/AgentGameTest/Codex/Abilities/GA_Player_PrimaryAttack",
    "/Game/AgentGameTest/Codex/Effects/GE_Cooldown_Dash",
    "/Game/AgentGameTest/Codex/Effects/GE_Cooldown_Enemy_MeleeAttack",
    "/Game/AgentGameTest/Codex/Effects/GE_Cooldown_PrimaryAttack",
    "/Game/AgentGameTest/Codex/Effects/GE_Damage",
    "/Game/AgentGameTest/Codex/Effects/GE_Enemy_DefaultAttributes",
    "/Game/AgentGameTest/Codex/Effects/GE_Player_DefaultAttributes",
    "/Game/AgentGameTest/Codex/Blueprints/BP_Enemy_AIController_Codex",
    "/Game/AgentGameTest/Codex/Blueprints/BP_Enemy_Base_Codex",
    "/Game/AgentGameTest/Codex/Blueprints/BP_Enemy_Grunt_Codex",
    "/Game/AgentGameTest/Codex/Blueprints/BP_Enemy_Runner_Codex",
    "/Game/AgentGameTest/Codex/Blueprints/BP_EnemySpawner_Codex",
    "/Game/AgentGameTest/Codex/Blueprints/BP_EnemySpawnPoint_Codex",
    "/Game/AgentGameTest/Codex/Blueprints/BP_GameMode_Arena_Codex",
    "/Game/AgentGameTest/Codex/Blueprints/BP_GameMode_STEP3_Codex",
    "/Game/AgentGameTest/Codex/Blueprints/BP_GAS_TestTarget_Codex",
    "/Game/AgentGameTest/Codex/Blueprints/BP_Grunt_Arena_Codex",
    "/Game/AgentGameTest/Codex/Blueprints/BP_LastStand_GameMode_Codex",
    "/Game/AgentGameTest/Codex/Blueprints/BP_LastStand_Player_Codex",
    "/Game/AgentGameTest/Codex/Blueprints/BP_Player_Arena_Codex",
    "/Game/AgentGameTest/Codex/Blueprints/BP_Runner_Arena_Codex",
    "/Game/AgentGameTest/Codex/UI/Common/W_CommonButton_Codex",
    "/Game/AgentGameTest/Codex/UI/HUD/W_GameHUD_Codex",
    "/Game/AgentGameTest/Codex/UI/Layout/W_PrimaryGameLayout_Codex",
    "/Game/AgentGameTest/Codex/UI/Menu/W_MainMenu_Codex",
    "/Game/AgentGameTest/Codex/UI/Menu/W_PauseMenu_Codex",
    "/Game/AgentGameTest/Codex/UI/Result/W_GameOver_Codex",
    "/Game/AgentGameTest/Codex/UI/Result/W_Victory_Codex",
    "/Game/AgentGameTest/Codex/UI/Styles/BP_UI_LastStandButtonStyle_Codex",
)

AUDIO_ASSETS = (
    "/Game/AgentGameTest/Codex/Audio/Player/S_Player_Attack_Codex",
    "/Game/AgentGameTest/Codex/Audio/Player/S_Player_Dash_Codex",
    "/Game/AgentGameTest/Codex/Audio/Player/S_Player_Damage_Codex",
    "/Game/AgentGameTest/Codex/Audio/Enemy/S_Enemy_Hit_Codex",
    "/Game/AgentGameTest/Codex/Audio/Enemy/S_Enemy_Death_Codex",
    "/Game/AgentGameTest/Codex/Audio/Game/S_Wave_Start_Codex",
    "/Game/AgentGameTest/Codex/Audio/Game/S_Wave_Clear_Codex",
    "/Game/AgentGameTest/Codex/Audio/Game/S_Victory_Codex",
    "/Game/AgentGameTest/Codex/Audio/Game/S_GameOver_Codex",
    "/Game/AgentGameTest/Codex/Audio/UI/S_UI_Confirm_Codex",
)

NIAGARA_ASSETS = (
    "/Game/AgentGameTest/Codex/VFX/Player/NS_Player_Attack_Codex",
    "/Game/AgentGameTest/Codex/VFX/Player/NS_Player_Dash_Codex",
    "/Game/AgentGameTest/Codex/VFX/Enemy/NS_Enemy_Hit_Codex",
    "/Game/AgentGameTest/Codex/VFX/Enemy/NS_Enemy_Death_Codex",
    "/Game/AgentGameTest/Codex/VFX/Environment/NS_World_Impact_Codex",
)

LEVEL_ASSETS = (
    "/Game/AgentGameTest/Codex/Levels/L_LastStand_Arena_Codex",
    "/Game/AgentGameTest/Codex/Levels/L_LastStand_EnemyTest_Codex",
    "/Game/AgentGameTest/Codex/Levels/L_LastStand_GameLoopTest_Codex",
    "/Game/AgentGameTest/Codex/Levels/L_LastStand_PlayerTest_Codex",
)


def load_required(asset_path: str):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if asset is None:
        raise RuntimeError(f"Missing asset: {asset_path}")
    return asset


def wav_stats(path: Path) -> dict:
    with wave.open(str(path), "rb") as wav_file:
        if wav_file.getsampwidth() != 2 or wav_file.getnchannels() != 1:
            raise RuntimeError(f"Unexpected WAV format: {path}")
        frame_count = wav_file.getnframes()
        raw = wav_file.readframes(frame_count)
        samples = struct.unpack(f"<{frame_count}h", raw)
        mean = sum(samples) / max(1, frame_count)
        return {
            "frames": frame_count,
            "sample_rate": wav_file.getframerate(),
            "first_sample": samples[0] if samples else 0,
            "last_sample": samples[-1] if samples else 0,
            "mean_sample": round(mean, 4),
            "peak_sample": max((abs(value) for value in samples), default=0),
        }


def main() -> None:
    report = {
        "step": 6,
        "agent": "Codex",
        "blueprints": [],
        "audio": [],
        "niagara": [],
        "levels": [],
        "errors": [],
        "success": False,
    }

    for asset_path in BLUEPRINT_ASSETS:
        try:
            blueprint = load_required(asset_path)
            unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
            status = str(blueprint.get_editor_property("status"))
            generated_class = blueprint.generated_class()
            if generated_class is None or "ERROR" in status.upper():
                raise RuntimeError(f"Compile status {status}; generated_class={generated_class}")
            unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
            report["blueprints"].append({
                "asset": asset_path,
                "status": status,
                "generated_class": generated_class.get_path_name(),
            })
        except Exception as exc:
            report["errors"].append(f"Blueprint {asset_path}: {type(exc).__name__}: {exc}")

    for asset_path in AUDIO_ASSETS:
        try:
            sound = load_required(asset_path)
            wav_path = WAV_ROOT / f"{asset_path.rsplit('/', 1)[-1]}.wav"
            entry = {
                "asset": asset_path,
                "class": sound.get_class().get_name(),
                "duration": round(float(sound.get_editor_property("duration")), 4),
                "looping": bool(sound.get_editor_property("looping")),
                "wav": str(wav_path),
                "wav_stats": wav_stats(wav_path),
            }
            if entry["looping"]:
                raise RuntimeError(f"One-shot sound unexpectedly loops: {asset_path}")
            report["audio"].append(entry)
        except Exception as exc:
            report["errors"].append(f"Audio {asset_path}: {type(exc).__name__}: {exc}")

    for asset_path in NIAGARA_ASSETS:
        try:
            system = load_required(asset_path)
            report["niagara"].append({
                "asset": asset_path,
                "class": system.get_class().get_name(),
            })
        except Exception as exc:
            report["errors"].append(f"Niagara {asset_path}: {type(exc).__name__}: {exc}")

    for asset_path in LEVEL_ASSETS:
        try:
            level = load_required(asset_path)
            report["levels"].append({
                "asset": asset_path,
                "class": level.get_class().get_name(),
            })
        except Exception as exc:
            report["errors"].append(f"Level {asset_path}: {type(exc).__name__}: {exc}")

    report["counts"] = {
        "blueprints": len(report["blueprints"]),
        "audio": len(report["audio"]),
        "niagara": len(report["niagara"]),
        "levels": len(report["levels"]),
    }
    report["success"] = not report["errors"] and report["counts"] == {
        "blueprints": len(BLUEPRINT_ASSETS),
        "audio": len(AUDIO_ASSETS),
        "niagara": len(NIAGARA_ASSETS),
        "levels": len(LEVEL_ASSETS),
    }

    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    if not report["success"]:
        raise RuntimeError("STEP 6 asset validation failed: " + " | ".join(report["errors"]))
    unreal.log(
        "CODEX_STEP6_ASSET_VALIDATION Success=true "
        f"Blueprints={len(BLUEPRINT_ASSETS)} Audio={len(AUDIO_ASSETS)} "
        f"Niagara={len(NIAGARA_ASSETS)} Levels={len(LEVEL_ASSETS)} Report={REPORT_PATH}"
    )


if __name__ == "__main__":
    main()
