"""Create an isolated, empty Codex source file for background Blender MCP."""
from pathlib import Path
import bpy

target = Path(r"D:\Unreal Projects\CodexGame\ExternalAssets\LastStand\Codex\Models\LS_Codex_Environment.blend")
if target.exists():
    raise RuntimeError(f"Refusing to replace an existing Blender source: {target}")
target.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.name = "LS_Codex_Environment"
bpy.ops.wm.save_as_mainfile(filepath=str(target), check_existing=False)
print(f"CODEX_STEP4_BLENDER_BOOTSTRAP_SUCCESS {target}")
