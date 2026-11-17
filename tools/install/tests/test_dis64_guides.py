#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
# EN: Exercise the actual SDK guide linker on a relocated copy of its mapped files.
# RU: Проверка штатного обработчика ссылок SDK на отдельно скопированных файлах.

from pathlib import Path
import re
import runpy
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]  # Source repository, never modified by this test.
LINKER = ROOT / "tools/link-installed-fir-docs.py"  # Actual production link rewriter.


class Dis64GuideTests(unittest.TestCase):
    """EN/RU: Check offline guide destinations, not compiler or VM execution.
    Проверять локальные ссылки документации, без запуска компилятора или VM.
    """

    # EN: Copy every mapped source into a private SDK, run the real linker, then
    # check local links outside fenced examples. Missing source/target fails explicitly.
    # RU: Копировать файлы в отдельный SDK, запустить штатную обработку и проверить
    # локальные ссылки вне примеров кода. Отсутствующие исходники или цели — ошибка.
    def test_relocated_documentation_closure(self):
        mapper = runpy.run_path(str(LINKER))["envelope_pairs"]
        with tempfile.TemporaryDirectory(prefix="Dis64 SDK guides ") as directory:
            package = Path(directory)
            pairs = mapper(ROOT, package)
            for source, destination in pairs.items():
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(source, destination)

            completed = subprocess.run(
                [sys.executable, str(LINKER), str(ROOT), str(package), "--envelope"],
                capture_output=True,
                text=True,
            )
            if completed.returncode:
                self.fail("SDK guide rewriting failed: " + completed.stderr)

            for guide in package.rglob("*.md"):
                fence = None
                for line in guide.read_text().splitlines():
                    marker = line.lstrip()[:3]
                    if marker in ("```", "~~~"):
                        if fence is None:
                            fence = marker
                        elif fence == marker:
                            fence = None
                        continue
                    if fence is not None:
                        continue
                    for target in re.findall(r"\]\(([^)\n]+)\)", line):
                        if "://" in target or target.startswith(("#", "mailto:")):
                            continue
                        resolved = (guide.parent / target.split("#")[0]).resolve()
                        try:
                            resolved.relative_to(package)
                        except ValueError:
                            self.fail("Link escaped the relocated SDK: " + target)
                        if not resolved.exists():
                            self.fail(str(guide) + " has a missing target: " + target)

            local_state = package / "docs/dis64/dis64-local-state.md"
            if "](native64-series0075.md)" not in local_state.read_text():
                self.fail("The native-series note must remain a local SDK link")


if __name__ == "__main__":
    unittest.main()
