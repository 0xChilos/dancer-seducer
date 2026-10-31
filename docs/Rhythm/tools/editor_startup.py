"""Create editor assets, then register the PIE demo observer."""
import os
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)

_directory = os.path.dirname(os.path.abspath(__file__))
for _name in ("create_editor_assets.py", "verify_editor_demo.py"):
    _path = os.path.join(_directory, _name)
    _namespace = {"__file__": _path, "__name__": "__main__"}
    with open(_path, encoding="utf-8-sig") as _source:
        exec(compile(_source.read(), _path, "exec"), _namespace)
