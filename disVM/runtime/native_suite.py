# Copyright (c) 2026 bhgv
# EN: Repeatable CTest acceptance selection for registered native-spawn drivers.
# RU: Воспроизводимый выбор приёмочных CTest-драйверов с native-spawn.
import argparse
import json
from pathlib import Path
import re
import subprocess


# EN: Borrow ctest executable and configured build directory. Return registered
# test names whose Python driver contains --native-spawn. This is a source-based
# suite selector, not proof that every child uses native mode. Mixed reference
# profiles and explicit rejection controls are preserved. Missing/invalid CTest
# metadata or source read failures raise; no runtime or repository file changes.
# RU: Заимствовать ctest и настроенный build. Вернуть имена тестов, в Python-
# драйвере которых есть --native-spawn. Это выбор набора по исходникам, а не
# доказательство native-режима каждого ребёнка. Сохраняются смешанные эталонные
# профили и проверки явного отказа. Ошибки метаданных/чтения передаются наверх;
# runtime и файлы репозитория не меняются.
def native_tests(ctest, build):
    result = subprocess.run(
        [ctest, "--test-dir", str(build), "--show-only=json-v1"],
        check=True,
        capture_output=True,
        text=True,
    )
    selected = []
    for test in json.loads(result.stdout)["tests"]:
        drivers = [Path(arg) for arg in test.get("command", []) if arg.endswith(".py")]
        if any(path.is_file() and "--native-spawn" in path.read_text() for path in drivers):
            selected.append(test["name"])
    return selected


# EN: CLI supplies build, optional ctest/jobs and read-only --list. Own the CTest
# process until completion; preserve its actual exit status. Empty selections
# fail before execution. This gate does not certify unregistered paths, emitted
# JIT race instrumentation or another CPU architecture.
# RU: CLI задаёт build, необязательные ctest/jobs и --list без запуска. Владеть
# CTest до завершения, вернуть его настоящий статус. Пустой набор отвергается
# до запуска. Этот этап не сертифицирует незарегистрированные пути, race-
# инструментирование машинного кода JIT или другую архитектуру CPU.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=lambda value: Path(value).resolve())
    parser.add_argument("--ctest", default="ctest")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--list", action="store_true")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    names = native_tests(args.ctest, args.build)
    if not names:
        parser.error("configured build has no registered native-spawn drivers")
    if args.list:
        print("\n".join(names))
        return 0
    print(f"Selected {len(names)} registered native-spawn driver suites", flush=True)
    pattern = "^(" + "|".join(re.escape(name) for name in names) + ")$"
    return subprocess.run(
        [
            args.ctest,
            "--test-dir",
            str(args.build),
            "-j",
            str(args.jobs),
            "--output-on-failure",
            "-R",
            pattern,
        ]
    ).returncode


if __name__ == "__main__":
    raise SystemExit(main())
