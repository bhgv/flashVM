# Copyright (c) 2026 bhgv
# EN: Exercise live native registry pages from actual concurrent guest threads.
# RU: Проверить live native registry pages из actual concurrent guest threads.
from pathlib import Path
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence


# EN/RU: Borrow matched runtime/compiler/include; own private wrapper and actual VM until all joins.
# Borrow matched runtime/compiler/include; владеть private wrapper и actual VM до всех joins.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="live registry ") as temporary:
        work = Path(temporary)
        image = work / "main.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(image),
                str(root / "fixtures/owner-monitor/main.b"),
            ],
            check=True,
            capture_output=True,
        )
        guest_image = work / "guest.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(guest_image),
                str(root / "fixtures/live-residents/main.b"),
            ],
            check=True,
            capture_output=True,
        )
        phase_image = work / "phases.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(phase_image),
                str(root / "fixtures/resident-phases/main.b"),
            ],
            check=True,
            capture_output=True,
        )
        activity_image = work / "activity.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(activity_image),
                str(root / "fixtures/resident-activity/main.b"),
            ],
            check=True,
            capture_output=True,
        )
        progress_image = work / "progress.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(progress_image),
                str(root / "fixtures/resident-progress/main.b"),
            ],
            check=True,
            capture_output=True,
        )
        join_binary = relink(
            runtime,
            work,
            root / "test_resident_summary_join.c",
            10,
            ["dvm_thread_registrar_join", "dvm_resident_summary"],
        )
        binary = relink(runtime, work, root / "test_live_registry_probe.c", 0, ["Sys_werrstr"])
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                result = subprocess.run(
                    [
                        str(binary),
                        "--stats",
                        mode,
                        *(["--gc-locked-poll"] if locked else []),
                        str(image),
                    ],
                    capture_output=True,
                    text=True,
                    timeout=90,
                )
                output = result.stdout + result.stderr
                if (
                    result.returncode
                    or "ThreadSanitizer:" in output
                    or "runtime error:" in output
                    or "NATIVELIVE_REGISTRY observed=1 bounded=1" not in output
                ):
                    raise RuntimeError("live registry failed\n" + output[-10000:])
                evidence(output, children=8)
                guest_result = subprocess.run(
                    [
                        str(runtime),
                        "--stats",
                        mode,
                        *(["--gc-locked-poll"] if locked else []),
                        str(guest_image),
                    ],
                    capture_output=True,
                    text=True,
                    timeout=90,
                )
                guest_output = guest_result.stdout + guest_result.stderr
                if (
                    guest_result.returncode
                    or "ThreadSanitizer:" in guest_output
                    or "runtime error:" in guest_output
                    or "PASS guest live residents:" not in guest_output
                ):
                    raise RuntimeError("guest live resident API failed\n" + guest_output[-10000:])
                evidence(guest_output, children=8)
                phase_result = subprocess.run(
                    [
                        str(runtime),
                        "--stats",
                        mode,
                        *(["--gc-locked-poll"] if locked else []),
                        str(phase_image),
                    ],
                    capture_output=True,
                    text=True,
                    timeout=90,
                )
                phase_output = phase_result.stdout + phase_result.stderr
                if (
                    phase_result.returncode
                    or "ThreadSanitizer:" in phase_output
                    or "runtime error:" in phase_output
                    or "PASS resident phases: eight waits dispatch before after wake joined"
                    not in phase_output
                ):
                    raise RuntimeError("resident phase API failed\n" + phase_output[-10000:])
                evidence(phase_output, children=8)
                activity_result = subprocess.run(
                    [
                        str(join_binary),
                        "--stats",
                        mode,
                        *(["--gc-locked-poll"] if locked else []),
                        str(activity_image),
                    ],
                    capture_output=True,
                    text=True,
                    timeout=90,
                )
                activity_output = activity_result.stdout + activity_result.stderr
                if (
                    activity_result.returncode
                    or "ThreadSanitizer:" in activity_output
                    or "runtime error:" in activity_output
                    or "DETECTED resident summary" in activity_output
                    or "NATIVERESIDENT_JOIN observed=1 partition=1" not in activity_output
                    or "PASS resident activity: send receive alt table native ready wake exact IDs reaped joined"
                    not in activity_output
                ):
                    raise RuntimeError("resident activity API failed\n" + activity_output[-10000:])
                evidence(activity_output, children=5)
                progress_result = subprocess.run(
                    [
                        str(runtime),
                        "--stats",
                        mode,
                        *(["--gc-locked-poll"] if locked else []),
                        str(progress_image),
                    ],
                    capture_output=True,
                    text=True,
                    timeout=90,
                )
                progress_output = progress_result.stdout + progress_result.stderr
                if (
                    progress_result.returncode
                    or "ThreadSanitizer:" in progress_output
                    or "runtime error:" in progress_output
                    or "PASS resident progress: eight owners four wakes monotonic counters shared canaries summary joined"
                    not in progress_output
                ):
                    raise RuntimeError("resident progress API failed\n" + progress_output[-10000:])
                evidence(progress_output, children=9)
                print("PASS live registry " + mode + " locked=" + str(locked), flush=True)
        for variant, target_image, children, diagnostic in (
            (1, phase_image, 8, "resident phases never published eight readiness waits"),
            (2, phase_image, 8, "resident phases never published owner dispatch"),
            (
                3,
                activity_image,
                5,
                "resident activity missing send receive alt table native publication",
            ),
            (4, progress_image, 9, "resident progress stopped publishing completed turns"),
        ):
            omitted = relink(
                runtime,
                work,
                root / "test_resident_phase_omission.c",
                variant,
                ["dvm_resident_observe"],
            )
            result = subprocess.run(
                [str(omitted), "--stats", "--no-jit", str(target_image)],
                capture_output=True,
                text=True,
                timeout=90,
            )
            output = result.stdout + result.stderr
            if (
                "DETECTED " + diagnostic not in output
                or "PASS resident phases:" in output
                or "PASS resident activity:" in output
                or "PASS resident progress:" in output
                or "ThreadSanitizer:" in output
                or "runtime error:" in output
                or result.returncode != 0
            ):
                raise RuntimeError("phase omission not detected semantically\n" + output[-10000:])
            evidence(output, children=children)
            print("PASS resident observation omission " + str(variant), flush=True)
        broken_join = relink(
            runtime,
            work,
            root / "test_resident_summary_join.c",
            1,
            ["dvm_thread_registrar_join", "dvm_resident_summary"],
        )
        result = subprocess.run(
            [str(broken_join), "--stats", "--no-jit", str(activity_image)],
            capture_output=True,
            text=True,
            timeout=90,
        )
        output = result.stdout + result.stderr
        if (
            result.returncode
            or "ThreadSanitizer:" in output
            or "runtime error:" in output
            or "DETECTED resident summary lost in-flight partition" not in output
            or "PASS resident activity:" not in output
        ):
            raise RuntimeError("resident summary omission not detected cleanly\n" + output[-10000:])
        evidence(output, children=5)
        print("PASS resident summary in-flight omission", flush=True)


if __name__ == "__main__":
    main()
