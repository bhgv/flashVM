# Copyright (c) 2026 bhgv
# EN: Compose real array-length, capture replay and channel commit regressions.
# RU: Объединить проверки длины массивов, повторов захвата и commit каналов.
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

FIXTURES = Path(__file__).parent / "fixtures/shared-array-length"  # Immutable private inputs.
GUEST = (FIXTURES / "main.b").read_text()  # Real reference-mode capture workload.
RECEIVE_ERRORS = (FIXTURES / "receive-errors.b").read_text()  # Native commit-boundary workload.
WRAPPER = "".join(
    (FIXTURES / name).read_text()
    for name in (
        "capture-setup.inc",
        "capture-ranges.inc",
        "capture-addresses.inc",
        "array-guards.inc",
        "array-probe.inc",
    )
)  # Same translation unit: private helpers stay local to this test binary.


# EN: CLI runtime/compiler/include identify matching native products. Own temporary
# files/binaries until all C/JIT/poll/control profiles finish; raise on refusal,
# timeout or wrong oracle evidence. Never mutate production build objects.
# RU: CLI runtime/compiler/include задают согласованные продукты сборки. Владеть
# временными файлами до завершения всех C/JIT/poll/control; ошибка, таймаут либо
# неверный oracle дают исключение. Production-объекты не изменяются.
def main():
    """Use matching native objects, exact omission diagnostics and both polls."""
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix="guarded array length ") as directory:
        work = Path(directory)
        source, image = work / "main.b", work / "main.dis"
        source.write_text(GUEST)
        subprocess.run(
            [str(compiler), "-F", "-I", str(include), "-o", str(image), str(source)], check=True
        )
        error_source, error_image = work / "receive.b", work / "receive.dis"
        error_source.write_text(RECEIVE_ERRORS)
        subprocess.run(
            [str(compiler), "-F", "-I", str(include), "-o", str(error_image), str(error_source)],
            check=True,
        )
        wrapper = work / "wrapper.c"
        wrapper.write_text(WRAPPER)
        for variant in ((0, 4) if "--positive-only" in sys.argv else (0, 1, 2, 3, 4)):
            binary = relink(
                runtime,
                work,
                wrapper,
                variant,
                [
                    "dvm_runtime_root_registry_probe",
                    "dvm_managed_small_three_begin",
                    "dvm_small_range_access_extend",
                    "dvm_runtime_array_length_source",
                    "dvm_runtime_array_length_value",
                    "dvm_runtime_array_index_operands",
                    "dvm_runtime_bytes_publish_opcode",
                    "dvm_managed_ranges_begin",
                    "dvm_runtime_string_view",
                    "dvm_managed_three_begin",
                    "dvm_managed_spans_begin",
                    "dvm_runtime_channel_waiter_published",
                    "dvm_runtime_guest_sleep",
                    "dvm_runtime_mapped_buffer_transfer",
                    "destroy",
                    "error",
                    "malloc",
                    "calloc",
                    "free",
                ],
            )
            for mode in ("--no-jit", "--jit"):
                if variant == 0:
                    run = subprocess.run(
                        [
                            str(binary),
                            mode,
                            "--gc-worker",
                            "--guarded-copies",
                            "--native-spawn",
                            "--parallel-channel-buffer",
                            "--stats",
                            str(error_image),
                        ],
                        capture_output=True,
                        text=True,
                        timeout=30,
                        env=dict(os.environ, DVM_TEST_RECEIVE_ERRORS="1"),
                    )
                    if (
                        run.returncode
                        or "PASS buffered receive commit boundary" not in run.stdout
                        or run.stdout.count("PASS injected receive array release failure") != 2
                        or "parallel_channel_refills=1" not in run.stdout
                    ):
                        raise RuntimeError(run.stdout + run.stderr)
                for locked in (False, True):
                    args = [
                        str(binary),
                        mode,
                        "--gc-worker",
                        "--guarded-copies",
                        "--managed-owner-probe",
                    ]
                    if locked:
                        args += ["--gc-locked-poll"]
                    run = subprocess.run(
                        args + [str(image)],
                        capture_output=True,
                        text=True,
                        timeout=90,
                        env=dict(os.environ, TMPDIR=str(work)),
                    )
                    output = run.stdout + run.stderr
                    if "WARNING: ThreadSanitizer:" in output or "runtime error:" in output:
                        raise RuntimeError(output)
                    if variant in (1, 2, 3):
                        marker = {
                            1: "source read guarded",
                            2: "field read guarded",
                            3: "exact length and canaries",
                        }[variant]
                        if run.returncode != 10 or "FAIL array length: " + marker not in output:
                            raise RuntimeError("array-length control missed: " + output)
                    elif (
                        run.returncode
                        or "PASS array length probe:" not in output
                        or "PASS array length32" not in output
                        or "PASS guarded array index route" not in output
                    ):
                        raise RuntimeError("array-length positive failed: " + output)
                    if variant == 0 and (
                        "PASS scheduled length extension retry" not in output
                        or "PASS scheduled index retry" not in output
                        or "PASS scheduled address publication retry" not in output
                        or "PASS four interior addresses survive Cycles before use" not in output
                    ):
                        raise RuntimeError("length extension retry missed: " + output)
                    if variant == 0 and "PASS ALT selected guard capture refusal" not in output:
                        raise RuntimeError("ALT selection refusal coverage missed: " + output)
                    if variant == 0 and "PASS injected string view refusal" not in output:
                        raise RuntimeError("string view refusal coverage missed: " + output)
                    if variant == 0 and any(
                        f"PASS guarded capture replay {stage}" not in output for stage in range(50)
                    ):
                        raise RuntimeError("slice/conversion replay coverage missed: " + output)
    print(
        "PASS actual LENA:15360 aliases,nil/geometry,caller-rooted Cycles/Fast,no allocations,both polls"
    )


if __name__ == "__main__":
    main()
