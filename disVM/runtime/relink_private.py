# Copyright (c) 2026 bhgv
# EN: Relink private diagnostic objects without mutating production builds.
# RU: Линковка частных диагностов без изменения основной сборки.
"""Relink a private diagnostic wrapper against unchanged CMake runtime objects."""

from pathlib import Path
import shlex
import subprocess


# runtime identifies the matching native console/editor build, work owns temporary
# outputs, source is its C wrapper, variant copied define and symbols are exact
# linker interceptions. Return private binary. No production object is changed.
# Makefiles/GNU-style native linker only; compile/link failures never count as
# semantic detection. Optional executor/compiler_source replace only private xec.c/AMD64 compiler
# objects; originals and source headers remain unchanged.
# Optional exception/loader/collector/managed-owner replacements compile private views.
# Optional shared_arithmetic_source replaces only that guarded transaction body;
# benchmark comparisons keep computation and other production objects unchanged.
# Optional bytes_fast_source changes only the private C dispatcher for exact
# decline/false-completion/duplicate-execution controls.
# Optional exception_retirement_source replaces only the shared Fast emergency-string module.
# Optional module_graph_source/graph_capture_source replace only discovery indexing/capture.
# Optional snapshot_growth_source/graph_storage_source isolate allocation refusal controls.
# Optional graph_confirmation_source preserves real claim helpers in private refusal controls.
# Optional runtime_main_source replaces only the host composition for queue-locked channel observations.
# Optional packet_budget_source isolates alias-sort allocation refusal controls.
# packet_budget_source — частная проверка отказа памяти учёта повторных ссылок.
# Optional retirement_pass_source replaces only the shared Fast service coordinator.
# retirement_pass_source — частная проверка распределения квот служб Fast.
# Optional concurrent_runtime_source replaces only strict pre-close coordination.
# concurrent_runtime_source — частная проверка координации до закрытия.
# Optional native_admission_source replaces guest admission for retry controls.
# native_admission_source — частный контроль повторного допуска исполнителя.
# Optional duke_pass_source replaces only the native Duke dispatcher for queue-routing controls.
# Matching private headers/flags remain mandatory.
# remote_managed_source — private export ownership omission controls only.
# RU: runtime — исходная сборка, work — личные результаты, source — wrapper,
# variant — define, symbols — перехваты линкера; вернуть личный бинарник.
# runtime_main_source — необязательная частная композиция host с теми же CMake-флагами.
# duke_pass_source — необязательный частный диспетчер очереди Duke.
# Необязательные *_source заменяют только соответствующий объект личной сборки;
# executor выбирает её режим. Ошибки сборки не считаются семантической проверкой.
def relink(
    runtime,
    work,
    source,
    variant,
    symbols,
    executor=None,
    compiler_source=None,
    exception_source=None,
    loader_source=None,
    stack_source=None,
    runtime_rc_source=None,
    gc_source=None,
    managed_owner_source=None,
    shared_pointer_source=None,
    shared_bytes_source=None,
    shared_arithmetic_source=None,
    shared_real_arithmetic_source=None,
    bytes_fast_source=None,
    shared_real_conversion_source=None,
    heap_hold_source=None,
    list_hold_source=None,
    heap_commit_source=None,
    pointer_stage_source=None,
    pointer_prepare_source=None,
    pointer_transaction_source=None,
    pointer_builder_source=None,
    value_prepare_source=None,
    scalar_cons_source=None,
    raw_cons_source=None,
    mapped_cons_source=None,
    mapped_prepare_source=None,
    string_source=None,
    exception_retirement_source=None,
    managed_retirement_source=None,
    module_graph_source=None,
    graph_capture_source=None,
    snapshot_growth_source=None,
    graph_storage_source=None,
    graph_confirmation_source=None,
    runtime_main_source=None,
    duke_pass_source=None,
    native_admission_source=None,
    concurrent_runtime_source=None,
    retirement_pass_source=None,
    packet_budget_source=None,
    remote_managed_source=None,
):
    target = "disvm-editor" if runtime.name == "disvm-editor" else "disvm"
    metadata = runtime.parent / ("CMakeFiles/" + target + ".dir")
    flags = {}
    for line in (metadata / "flags.make").read_text().splitlines():
        if " = " in line:
            key, value = line.split(" = ", 1)
            flags[key] = shlex.split(value)
    link = [
        value
        for value in shlex.split((metadata / "link.txt").read_text())
        if not value.startswith("-Wl,--dependency-file=")
    ]
    obj = work / "wrapper.o"
    includes = "-I" + str(Path(__file__).resolve().parent)
    command = [
        link[0],
        *flags["C_DEFINES"],
        *flags["C_INCLUDES"],
        *flags["C_FLAGS"],
        includes,
        "-DCONTROL=" + str(variant),
        "-c",
        str(source),
        "-o",
        str(obj),
    ]
    built = subprocess.run(command, capture_output=True, text=True)
    if built.returncode:
        raise RuntimeError("private wrapper compile failed: " + built.stdout + built.stderr)
    for candidate, filename in (
        (executor, "xec.c"),
        (compiler_source, "comp-amd64.c"),
        (exception_source, "exception.c"),
        (loader_source, "loader.c"),
        (stack_source, "validstk.c"),
        (runtime_rc_source, "runtime_rc.c"),
        (gc_source, "gc.c"),
        (managed_owner_source, "managed_owner.c"),
        (string_source, "string.c"),
        (shared_pointer_source, "shared_pointer.c"),
        (shared_bytes_source, "shared_bytes.c"),
        (shared_arithmetic_source, "shared_arithmetic.c"),
        (shared_real_arithmetic_source, "shared_real_arithmetic.c"),
        (bytes_fast_source, "bytes_fast.c"),
        (shared_real_conversion_source, "shared_real_conversion.c"),
        (heap_hold_source, "heap_hold.c"),
        (list_hold_source, "list_hold.c"),
        (heap_commit_source, "heap_commit.c"),
        (pointer_stage_source, "pointer_stage.c"),
        (pointer_prepare_source, "pointer_prepare.c"),
        (pointer_transaction_source, "pointer_transaction.c"),
        (pointer_builder_source, "pointer_builder.c"),
        (value_prepare_source, "value_prepare.c"),
        (scalar_cons_source, "scalar_cons.c"),
        (raw_cons_source, "raw_cons.c"),
        (mapped_cons_source, "mapped_cons.c"),
        (mapped_prepare_source, "mapped_prepare.c"),
        (exception_retirement_source, "gc/fast/exception_retirement.c"),
        (managed_retirement_source, "gc/fast/managed_retirement.c"),
        (module_graph_source, "gc/common/module_graph.c"),
        (graph_capture_source, "gc/common/graph_capture.c"),
        (snapshot_growth_source, "gc/duke/snapshot_growth.c"),
        (graph_storage_source, "gc/common/graph_storage.c"),
        (graph_confirmation_source, "gc/fast/graph_confirmation.c"),
        (runtime_main_source, "main.c"),
        (duke_pass_source, "gc/duke/large_graph_pass.c"),
        (native_admission_source, "gc/common/native_admission.c"),
        (concurrent_runtime_source, "gc/common/concurrent_runtime.c"),
        (retirement_pass_source, "gc/fast/retirement_pass.c"),
        (packet_budget_source, "gc/fast/packet_budget.c"),
        (remote_managed_source, "remote_managed.c"),
    ):
        if candidate is None:
            continue
        replacement = work / (Path(filename).name + ".o")
        command = [
            link[0],
            *flags["C_DEFINES"],
            *flags["C_INCLUDES"],
            *flags["C_FLAGS"],
            "-c",
            str(candidate),
            "-o",
            str(replacement),
        ]
        if filename == "gc.c":
            command.insert(1, "-Drungc=dvm_retained_rungc")
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError("private replacement compile failed: " + built.stdout + built.stderr)
        suffix = (
            "/" + filename + ".o"
            if "/" in filename
            or filename
            in (
                "main.c",
                "runtime_rc.c",
                "managed_owner.c",
                "shared_pointer.c",
                "shared_bytes.c",
                "shared_arithmetic.c",
                "shared_real_arithmetic.c",
                "bytes_fast.c",
                "shared_real_conversion.c",
                "heap_hold.c",
                "list_hold.c",
                "heap_commit.c",
                "pointer_stage.c",
                "pointer_prepare.c",
                "pointer_transaction.c",
                "pointer_builder.c",
                "value_prepare.c",
                "scalar_cons.c",
                "raw_cons.c",
                "mapped_cons.c",
                "mapped_prepare.c",
                "remote_managed.c",
            )
            else "/native/" + filename + ".o"
        )
        targets = [index for index, value in enumerate(link) if value.endswith(suffix)]
        if len(targets) != 1:
            raise RuntimeError("private replacement object boundary drift: " + filename)
        link[targets[0]] = str(replacement)
    binary = work / ("runtime-" + str(variant))
    link[link.index("-o") + 1] = str(binary)
    link += [str(obj), *["-Wl,--wrap=" + symbol for symbol in symbols]]
    built = subprocess.run(link, cwd=runtime.parent, capture_output=True, text=True)
    if built.returncode:
        raise RuntimeError("private runtime relink failed: " + built.stdout + built.stderr)
    return binary
