#!/usr/bin/env python3
"""Read actual VM fault ZIPs independently, checking CRC, JSON and source/code sections."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile

with tempfile.TemporaryDirectory(prefix="flashvm-crash-") as directory:
    root = Path(directory)
    archive = root / "report.zip"
    subprocess.run(
        [str(Path(sys.argv[1]).resolve()), str(archive)], cwd=root, check=True, timeout=60
    )
    with zipfile.ZipFile(archive) as report:
        if not (report.testzip() is None):
            raise AssertionError("condition check failed: flashVM/scripts/test_crash_archive.py:17")
        if not (json.loads(report.read("manifest.json"))["kind"] == "FlashVM VM fault"):
            raise AssertionError("condition check failed: flashVM/scripts/test_crash_archive.py:18")
        trace = json.loads(report.read("backtrace.json"))
        if not (
            trace["first_chance"] and trace["status"] == -6 and (not trace["frames_truncated"])
        ):
            raise AssertionError("condition check failed: flashVM/scripts/test_crash_archive.py:20")
        if not (len(trace["frames"]) == 2):
            raise AssertionError("condition check failed: flashVM/scripts/test_crash_archive.py:21")
        if not (trace["frames"][1]["function"] == "outer"):
            raise AssertionError("condition check failed: flashVM/scripts/test_crash_archive.py:22")
        if not (trace["frames"][1]["source"] == "example.as" and trace["frames"][1]["line"] == 17):
            raise AssertionError("condition check failed: flashVM/scripts/test_crash_archive.py:23")
        for index, frame in enumerate(trace["frames"]):
            if not (frame["forced"] and frame["arguments"][0]["value"] == "42"):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/test_crash_archive.py:25"
                )
            if not (report.read(f"bytecode/frame{index}.avm1")):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/test_crash_archive.py:26"
                )
            if frame["engine"] == "jit":
                if not (len(report.read(f"jit/frame{index}.bin")) == frame["jit_size"]):
                    raise AssertionError(
                        "condition check failed: flashVM/scripts/test_crash_archive.py:28"
                    )
        heap = json.loads(report.read("heap.json"))
        if not (heap["global"] in {obj["id"] for obj in heap["objects"]}):
            raise AssertionError("condition check failed: flashVM/scripts/test_crash_archive.py:30")
        global_object = next(obj for obj in heap["objects"] if obj["id"] == heap["global"])
        if not (any((field["name"] == "Object" for field in global_object["properties"]))):
            raise AssertionError("condition check failed: flashVM/scripts/test_crash_archive.py:32")
        if not (all(("prototype" in obj and "properties" in obj for obj in heap["objects"]))):
            raise AssertionError("condition check failed: flashVM/scripts/test_crash_archive.py:33")
    if len(sys.argv) > 2:
        source_archive = root / "source-report.zip"
        subprocess.run(
            [str(Path(sys.argv[2]).resolve()), str(source_archive)],
            cwd=root,
            check=True,
            timeout=60,
        )
        with zipfile.ZipFile(source_archive) as report:
            if not (report.testzip() is None):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/test_crash_archive.py:43"
                )
            modules = json.loads(report.read("sources/index.json"))
            if not (modules == [{"module": 7, "name": "Main.as", "entry": "sources/module0.txt"}]):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/test_crash_archive.py:45"
                )
            if not (
                report.read(modules[0]["entry"]) == b"// source\r\nthrow 42;\nvar other = 0;\n"
            ):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/test_crash_archive.py:46"
                )
            frame = json.loads(report.read("backtrace.json"))["frames"][0]
            if not (
                frame["source"] == "Main.as"
                and frame["line"] == 2
                and (frame["source_line"] == "throw 42;")
            ):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/test_crash_archive.py:48"
                )
print("PASS fault ZIP CRC, JSON, arguments, source, globals/classes and active JIT bytes")
