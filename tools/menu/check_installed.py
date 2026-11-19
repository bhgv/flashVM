# Copyright (c) 2026 bhgv
# EN: Exercise discovered installed console launchers from an unrelated directory.
# RU: Проверить обнаруженные установленные консольные команды из постороннего каталога.

from pathlib import Path
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from menu.products import product_entries


# EN: argv gives one installation prefix. Run each non-graphical discovered entry
# with bounded input/time, both Dis dispatches where applicable. Graphical entries
# require the separate SDL/replay checks; no missing optional products are fabricated.
# RU: argv задаёт prefix установки. Запустить каждую найденную консольную команду
# с ограниченными вводом/временем, в обоих режимах Dis. Графические команды требуют
# отдельных SDL/replay проверок; отсутствующие необязательные продукты не выдумываются.
def main():
    prefix = Path(sys.argv[1]).resolve()
    entries = product_entries(prefix)
    required = {"console", "terminal", "services", "modules", "desktop", "editor", "lifecycle",
                "pointer", "clipboard", "envelope", "pilot", "storage", "syntax", "tokens", "resident-owners"}
    actual = {entry[0] for entry in entries.values()}
    if not required.issubset(actual):
        raise RuntimeError("missing installed menu entries: " + str(sorted(required - actual)))
    if "managed-workers" in actual:
        raise RuntimeError("removed execution pool remains advertised")
    with tempfile.TemporaryDirectory(prefix="installed menu ") as directory:
        for key, (name, label, script, graphical) in entries.items():
            if graphical:
                print("SDL/replay entry " + key + ": " + name)
                continue
            modes = ((),) if name in ("envelope", "pilot", "storage", "flashvm-native") else (("--jit",), ("--no-jit",))
            for flags in modes:
                result = subprocess.run(
                    ["/bin/sh", str(script), *flags], cwd=directory,
                    input="quit\n" if name == "terminal" else "",
                    text=True, capture_output=True, timeout=60,
                )
                output = result.stdout + result.stderr
                if result.returncode or "FAIL" in output or "fail:" in output:
                    raise RuntimeError(name + " " + str(flags) + "\n" + output[-5000:])
                if not output.strip():
                    raise RuntimeError("silent installed command: " + name)
                print("PASS installed menu " + key + ": " + name + " " + str(flags), flush=True)


if __name__ == "__main__":
    main()
