#!/usr/bin/env python3
"""Reproduce real FlashVM measurements; never present the Inferno test pool as a speed benchmark."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import statistics
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


# Resolve only library flags/artifacts from an explicitly supplied trusted build.
def libraries(build):
    line = build / "CMakeFiles/flashvm_reference_swf.dir/link.txt"
    if not line.is_file():
        raise ValueError("Build flashvm_reference_swf first; expected " + str(line))
    result = []
    for word in shlex.split(line.read_text()):
        if word.endswith(".a"):
            path = Path(word)
            path = path if path.is_absolute() else build / path
            if not path.is_file():
                raise ValueError("Missing library: " + str(path))
            result.append(str(path))
        elif word.startswith("-l") or word == "-pthread":
            result.append(word)
    if not result:
        raise ValueError("No supported static-library link list found")
    return result


# Hash the audited first-party sources; a library build must match them separately.
def source_hash(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


# Every sample uses a new context/process and verifies graph reclamation, not just timing.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--flashvm-build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--counts", default="10000,50000")
    parser.add_argument("--repeats", type=int, default=5)
    args = parser.parse_args()
    counts = [int(value) for value in args.counts.split(",")]
    if not counts or any(value < 1 or value > 1000000 for value in counts) or args.repeats < 1:
        parser.error("Invalid counts/repeats")
    args.output.mkdir(parents=True, exist_ok=True)
    build = args.flashvm_build.resolve()
    binary = args.output.resolve() / "flashvm_gc_bench"
    obj = binary.with_suffix(".o")
    subprocess.run(
        [
            os.environ.get("CC", "cc"),
            "-O2",
            "-std=c89",
            "-I" + str(ROOT / "flashVM/include"),
            "-c",
            str(Path(__file__).with_name("flashvm.c")),
            "-o",
            str(obj),
        ],
        check=True,
    )
    subprocess.run(
        [os.environ.get("CXX", "c++"), str(obj), *libraries(build), "-pthread", "-o", str(binary)],
        check=True,
    )
    samples = []
    summary = []
    cases = ["objects_dead", "cycle_dead", "cycle_rooted", "strings_arena", "strings_traced"]
    for count in counts:
        for case in cases:
            group = []
            for repeat in range(args.repeats):
                result = subprocess.run(
                    [
                        sys.executable,
                        str(Path(__file__).with_name("measure.py")),
                        str(binary),
                        case,
                        str(count),
                    ],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    universal_newlines=True,
                    check=True,
                )
                data = json.loads(result.stdout)
                data["repeat"] = repeat
                samples.append(data)
                group.append(data)
            peaks = [item["peak_rss_bytes"] for item in group if item["peak_rss_bytes"] is not None]
            summary.append(
                {
                    "case": case,
                    "count": count,
                    "median_collect_cpu_ms": statistics.median(
                        item["drop_collect_cpu_ms"] for item in group
                    ),
                    "median_keep_cpu_ms": statistics.median(
                        item["keep_collect_cpu_ms"] for item in group
                    ),
                    "median_peak_rss_bytes": statistics.median(peaks) if peaks else None,
                    "strings_after_bytes": group[-1]["strings_after_bytes"],
                }
            )
    report = {
        "platform": platform.platform(),
        "compiler": subprocess.check_output(
            [os.environ.get("CC", "cc"), "--version"], universal_newlines=True
        ).splitlines()[0],
        "flashvm_gc_sha256": source_hash(ROOT / "flashVM/src/core/gc.cpp"),
        "flashvm_string_gc_sha256": source_hash(ROOT / "flashVM/src/core/string_gc.cpp"),
        "inferno_performance_comparison": "NOT MEASURED: allocator/runtime ABI gate must pass; test-pool timings are invalid",
        "repeats": args.repeats,
        "summary": summary,
        "samples": samples,
    }
    target = args.output / "measurements.json"
    target.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
    print("Saved " + str(target))


if __name__ == "__main__":
    main()
