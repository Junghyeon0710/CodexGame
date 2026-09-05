"""Editor startup entrypoint for the independent STEP 4 Arena assets."""
from pathlib import Path
import unreal

script_root = Path(unreal.Paths.project_dir()).resolve() / "Scripts/AgentGameTest/Codex"
for script_name in ("SetupStep04Readability.py", "BuildStep04Arena.py"):
    script_path = script_root / script_name
    exec(compile(script_path.read_text(encoding="utf-8"), str(script_path), "exec"),
         {"__name__": "__main__", "__file__": str(script_path)})
unreal.log("CODEX_STEP4_INITIALIZE_SUCCESS")
