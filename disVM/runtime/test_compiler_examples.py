# Copyright (c) 2026 bhgv
# EN: Run deployable compiler tutorials and execute the artifact they export.
# RU: Запуск поставляемых примеров компиляции и исполнение экспортированного кода.

from pathlib import Path
import json
import os
import subprocess
import sys
import tempfile


# EN: Execute borrowed runtime/module in mode with private cwd/temp files.
# Return captured output; timeout, nonzero exit or missing marker raises.
# RU: Исполнить заимствованные runtime/module в режиме mode с отдельным cwd/temp.
# Вернуть вывод; зависание, ненулевой код или отсутствие marker дают ошибку.
def execute(runtime, module, mode, work, marker, extra=()):
    result = subprocess.run(
        [str(runtime), mode, *extra, str(module)], cwd=work,
        env=dict(os.environ, TMPDIR=str(work)),
        text=True, capture_output=True, timeout=45,
    )
    if result.returncode or marker not in result.stdout:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout


# EN: argv gives runtime, module directory and --jit/--no-jit. Tutorials get
# independent temporary directories so exclusive exports can be tested repeatedly.
# Validate exported debug/source and actual execution, then remove owned files.
# RU: argv задаёт runtime, каталог модулей и --jit/--no-jit. У примеров отдельные
# временные каталоги, поэтому эксклюзивное создание повторяемо. Проверить debug,
# исходник и реальное исполнение, затем удалить принадлежащие тесту файлы.
def main():
    runtime, modules = (Path(value).resolve() for value in sys.argv[1:3])
    mode = sys.argv[3]
    if mode not in ("--jit", "--no-jit"):
        raise ValueError("expected --jit or --no-jit")
    tutorials = (
        ("compiler-service", "PASS compiler tutorial:"),
        ("build-panel", "PASS build panel tutorial:"),
        ("artifact-export", "PASS artifact tutorial:"),
    )
    for name, marker in tutorials:
        with tempfile.TemporaryDirectory(prefix="compiler example ") as directory:
            work = Path(directory)
            execute(runtime, modules / (name + ".dis"), mode, work, marker)
            if name == "artifact-export":
                debug = json.loads((work / "example.firdbg").read_text(encoding="utf-8"))
                if "return 6*7;" not in debug["source"]:
                    raise RuntimeError("exported debug lost the original source")
                execute(runtime, work / "example.dis", mode, work,
                        "DATA 0 42\nDATA 8 1\n", ("--inspect-words", "0", "2"))
        print("PASS compiler example and cleanup: " + name + " " + mode)


if __name__ == "__main__":
    main()
