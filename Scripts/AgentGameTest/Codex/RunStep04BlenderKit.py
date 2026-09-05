"""Run the kit in its newly created dedicated source, then retain editable source."""
from pathlib import Path
import bpy

root = Path(r"D:\Unreal Projects\CodexGame")
source = root / "ExternalAssets/LastStand/Codex/Models/LS_Codex_Environment.blend"
script = root / "Scripts/AgentGameTest/Codex/BuildStep04BlenderKit.py"
if Path(bpy.data.filepath).resolve() != source.resolve():
    raise RuntimeError("This runner only accepts the dedicated Codex STEP4 source")
scene = bpy.data.scenes.get("LS_Codex_Environment")
if scene and len(scene.objects) == 0:
    scene["last_stand_generator"] = "Codex_LastStand_Step04_BlenderKit_v1"
exec(compile(script.read_text(encoding="utf-8"), str(script), "exec"))
bpy.ops.wm.save_as_mainfile(filepath=str(source), check_existing=False)
print("CODEX_STEP4_BLENDER_SOURCE_SAVED")
