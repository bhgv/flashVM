# Copyright (c) 2026 bhgv
# EN: Repeatable CTest acceptance of explicitly registered native-only drivers.
# RU: Воспроизводимая приёмка явно зарегистрированных native-only CTest-драйверов.
import argparse
import json
from pathlib import Path
import re
import subprocess


# EN: Borrow ctest/build; select enabled tests explicitly labelled native.
# Historical mixed/reference probes are not active acceptance. Return names,
# raise on invalid metadata; execute no runtime here.
# RU: Заимствовать ctest/build; выбрать включённые тесты с явной меткой native.
# Исторические mixed/reference-пробы не входят в приёмку. Вернуть имена,
# при неверных метаданных вызвать исключение; runtime здесь не запускается.
def native_tests(ctest, build):
    result = subprocess.run(
        [ctest, "--test-dir", str(build), "--show-only=json-v1"],
        check=True,
        capture_output=True,
        text=True,
    )
    selected = []
    for test in json.loads(result.stdout)["tests"]:
        properties = {item["name"]: item["value"] for item in test.get("properties", [])}
        if not properties.get("DISABLED") and "native" in properties.get("LABELS", []):
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
