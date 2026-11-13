# Copyright (c) 2026 bhgv
# EN: Validate real construction, shared owners and channel error recovery.
# RU: Проверки создания объектов, общих владельцев и восстановления каналов.
# Copyright (c) 2026 bhgv
"""Exercise guarded record, array and channel construction through native and guest routes."""

from pathlib import Path
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence

GUEST = r"""implement Records;
include "sys.m";
Records: module { init: fn(); };
Pair: adt { name: string; value: int; };
init() {
    sys := load Sys Sys->PATH;
    item := ref Pair("record", 73);
    items := array[3] of ref Pair;
    last := len items - 1;
    items[last] = item;
    if (items[0] != nil || items[last] != item) { raise "array initialization"; }
    name := item.name;
    if (name != "record" || item.value != 73) {
        raise "record initialization";
    }
    left := item.value;
    right := len items;
    (left, right) = (right, left);
    if (left != 3 || right != 73) { raise "word swap"; }
    plain := chan of int;
    if (plain == nil) { raise "unbuffered word channel"; }
    words := chan[2] of int;
    words <-= 29;
    if (<-words != 29) { raise "word channel"; }
    pairs := chan[2] of (string, int);
    pairs <-= ("mapped", 42);
    (label, number) := <-pairs;
    if (label != "mapped" || number != 42) { raise "mapped channel"; }
    raw := chan[2] of (int, int);
    raw <-= (10, 20);
    (first, second) := <-raw;
    if (first != 10 || second != 20) { raise "raw channel"; }
    sys->print("PASS record guest\n");
}
"""


# EN: Reuse checked record/array/channel operations in forty independent owners.
# RU: Повторить проверенные операции над записями, массивами и каналами на 40 потоках.
CONCURRENT_GUEST = (
    GUEST.replace("init() {", "worker(done: chan of int) {", 1).replace(
        r'sys->print("PASS record guest\n");', "done <-= 1;", 1
    )
    + r"""
# EN: Start forty residents and collect one validated result from each.
# RU: Запустить сорок потоков и собрать проверенные результаты.
init()
{
    sys := load Sys Sys->PATH;
    done := chan[40] of int;
    for (i := 0; i < 40; i++)
        spawn worker(done);
    for (i = 0; i < 40; i++)
        if (<-done != 1)
            raise "allocation worker failed";
    sys->print("PASS forty allocation workers\n");
}
"""
)


def verify_existing_shared_dispatch(runtime, work, image):
    """Run real record and pointer handlers after entering SHARED."""
    executor = work / "shared-record-executor.c"
    source = (runtime.parent / "native/xec.c").read_text()
    boundary = "\t\t/* Successful translated steps own their register/PC publication. */"
    if source.count(boundary) != 1 or source.count("executed:\n") != 1:
        raise RuntimeError("record handler dispatch boundary drift")
    batch = "if(!dvm_runtime_instruction_limit && dvm_runtime_shared_integer)"
    source = source.replace(
        batch,
        "if(op != IADDW && op != ISUBW && op != IBNEW && op != IBGEW && !dvm_runtime_instruction_limit && dvm_runtime_shared_integer)",
        1,
    )
    source = source.replace(
        boundary,
        """
        int shared_record = (op == INEW || op == INEWZ || op == INEWA || op == INEWAZ || op == IMOVP || op == IMOVW || op == IINDL || op == ILENA || op == IADDW || op == ISUBW || op == IBNEW || op == IBGEW || op == ILEA || op == IBNEC || op == INEWCW) && dvm_runtime_gc_dispatch_share();
""" + boundary,
    )
    executor.write_text(
        source.replace(
            "executed:\n",
            """
executed:
        if (shared_record)
            dvm_runtime_gc_dispatch_finish();
""",
        )
    )
    wrapper = work / "shared-record-check.c"
    wrapper.write_text(r"""
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include "isa.h"
#include "disvm/native_owner.h"
#include "runtime_gc.h"
#include <stdio.h>
int __real_dvm_runtime_record_try_shared(void *, Type *, int);
int __wrap_dvm_runtime_record_try_shared(void *cell, Type *type, int clear)
{
    int result = __real_dvm_runtime_record_try_shared(cell, type, clear);
    if (result)
        puts("PASS record published directly in SHARED");
    return result;
}
Channel *__real_dvm_runtime_channel_try_shared(void *, Type *, WORD, void (*)(void));
Channel *__wrap_dvm_runtime_channel_try_shared(void *cell, Type *type, WORD count, void (*mover)(void))
{
    Channel *result = __real_dvm_runtime_channel_try_shared(cell, type, count, mover);
    if (result)
        puts(count ? "PASS buffered channel in SHARED" : "PASS unbuffered channel in SHARED");
    return result;
}

const char *__real_dvm_runtime_movp_opcode(int, const void *, void *, int *);
const char *__wrap_dvm_runtime_movp_opcode(int op, const void *source, void *destination, int *replay)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats before, after;
    int shared = op == IMOVP && dvm_runtime_gc_dispatch_is_shared() &&
        dvm_native_owner_statistics(owner, &before);
    const char *result = __real_dvm_runtime_movp_opcode(op, source, destination, replay);
    if (shared && !result && !*replay &&
        dvm_native_owner_statistics(owner, &after) && before.reentries == after.reentries)
        puts("PASS pointer published directly in SHARED");
    return result;
}

const char *__real_dvm_runtime_bytes_copy_opcode(int, const void *, void *, size_t);
const char *__wrap_dvm_runtime_bytes_copy_opcode(int op, const void *source, void *destination, size_t size)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats before, after;
    int shared = op == IMOVW && dvm_runtime_gc_dispatch_is_shared() &&
        dvm_native_owner_statistics(owner, &before);
    const char *result = __real_dvm_runtime_bytes_copy_opcode(op, source, destination, size);
    if (shared && !result &&
        dvm_native_owner_statistics(owner, &after) && before.reentries == after.reentries)
        puts("PASS word copied directly in SHARED");
    return result;
}

int __real_dvm_runtime_array_try_shared(void *, Type *, WORD, int);
int __wrap_dvm_runtime_array_try_shared(void *cell, Type *type, WORD count, int clear)
{
    int result = __real_dvm_runtime_array_try_shared(cell, type, count, clear);
    if (result)
        puts("PASS array published directly in SHARED");
    return result;
}

int __real_dvm_runtime_array_index_publish_shared(void *, WORD);
int __wrap_dvm_runtime_array_index_publish_shared(void *cell, WORD address)
{
    int result = __real_dvm_runtime_array_index_publish_shared(cell, address);
    if (result)
        puts("PASS array address published directly in SHARED");
    return result;
}

const char *__real_dvm_runtime_array_length(const void *, void *);
const char *__wrap_dvm_runtime_array_length(const void *source, void *destination)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats before, after;
    int shared = dvm_runtime_gc_dispatch_is_shared() && dvm_native_owner_statistics(owner, &before);
    const char *result = __real_dvm_runtime_array_length(source, destination);
    if (shared && !result &&
        dvm_native_owner_statistics(owner, &after) && before.reentries == after.reentries)
        puts("PASS array length read directly in SHARED");
    return result;
}

const char *__real_dvm_runtime_arithmetic(int, const void *, const void *, void *);
const char *__wrap_dvm_runtime_arithmetic(int op, const void *first, const void *second, void *output)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats before, after;
    int shared = dvm_runtime_gc_dispatch_is_shared() && dvm_native_owner_statistics(owner, &before);
    const char *result = __real_dvm_runtime_arithmetic(op, first, second, output);
    if (shared && !result &&
        dvm_native_owner_statistics(owner, &after) && before.reentries == after.reentries)
        puts("PASS integer arithmetic directly in SHARED");
    return result;
}

const char *__real_dvm_runtime_integer_branch(int, const void *, const void *, const void *, void *);
const char *__wrap_dvm_runtime_integer_branch(int op, const void *source, const void *other,
    const void *target, void *pc)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats before, after;
    int shared = dvm_runtime_gc_dispatch_is_shared() && dvm_native_owner_statistics(owner, &before);
    const char *result = __real_dvm_runtime_integer_branch(op, source, other, target, pc);
    if (shared && !result &&
        dvm_native_owner_statistics(owner, &after) && before.reentries == after.reentries)
        puts("PASS integer branch directly in SHARED");
    return result;
}

const char *__real_dvm_runtime_bytes_publish_opcode(int, const void *, void *, size_t);
const char *__wrap_dvm_runtime_bytes_publish_opcode(int op, const void *snapshot, void *destination, size_t size)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats before, after;
    int shared = op == ILEA && dvm_runtime_gc_dispatch_is_shared() &&
        dvm_native_owner_statistics(owner, &before);
    const char *result = __real_dvm_runtime_bytes_publish_opcode(op, snapshot, destination, size);
    if (shared && !result &&
        dvm_native_owner_statistics(owner, &after) && before.reentries == after.reentries)
        puts("PASS address copied directly in SHARED");
    return result;
}

const char *__real_dvm_runtime_string_branch(int, const void *, const void *, const void *, void *);
const char *__wrap_dvm_runtime_string_branch(int condition, const void *source, const void *other,
    const void *target, void *pc)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats before, after;
    int shared = dvm_runtime_gc_dispatch_is_shared() && dvm_native_owner_statistics(owner, &before);
    const char *result = __real_dvm_runtime_string_branch(condition, source, other, target, pc);
    if (shared && !result &&
        dvm_native_owner_statistics(owner, &after) && before.reentries == after.reentries)
        puts("PASS string branch directly in SHARED");
    return result;
}

""")
    binary = relink(
        runtime,
        work,
        wrapper,
        1,
        [
            "dvm_runtime_record_try_shared",
            "dvm_runtime_movp_opcode",
            "dvm_runtime_bytes_copy_opcode",
            "dvm_runtime_array_try_shared",
            "dvm_runtime_array_index_publish_shared",
            "dvm_runtime_array_length",
            "dvm_runtime_arithmetic",
            "dvm_runtime_integer_branch",
            "dvm_runtime_bytes_publish_opcode",
            "dvm_runtime_string_branch",
            "dvm_runtime_channel_try_shared",
        ],
        executor=executor,
    )
    for mode in ("--no-jit", "--jit"):
        result = subprocess.run(
            [binary, mode, "--native-spawn", image],
            cwd=work,
            capture_output=True,
            text=True,
            timeout=30,
        )
        output = result.stdout + result.stderr
        if (
            result.returncode
            or "PASS record published directly in SHARED" not in output
            or "PASS pointer published directly in SHARED" not in output
            or "PASS word copied directly in SHARED" not in output
            or "PASS array published directly in SHARED" not in output
            or "PASS array address published directly in SHARED" not in output
            or "PASS array length read directly in SHARED" not in output
            or "PASS integer arithmetic directly in SHARED" not in output
            or "PASS integer branch directly in SHARED" not in output
            or "PASS address copied directly in SHARED" not in output
            or "PASS string branch directly in SHARED" not in output
            or "PASS buffered channel in SHARED" not in output
            or "PASS unbuffered channel in SHARED" not in output
            or "PASS record guest" not in output
        ):
            raise RuntimeError(output)


# EN: Expand fixture's five ordered probe modules into an owned source string.
# fixture is the borrowed coordinator path; validate every include exactly once.
# Read errors/changed layout raise; no source files or runtime state are modified.
# RU: Развернуть пять модулей fixture по порядку в личную строку исходника.
# fixture — путь координатора; каждый include обязан встретиться один раз.
# Ошибки чтения/структуры дают отказ; исходники и runtime не изменяются.
def record_probe_source(fixture):
    source = fixture.read_text()
    for name in (
        "construction.inc",
        "channel-lifecycle.inc",
        "channel-transfer.inc",
        "alt-lifetime.inc",
        "mapped-buffer.inc",
    ):
        relative = "fixtures/record-new/" + name
        include = '#include "' + relative + '"'
        if source.count(include) != 1:
            raise RuntimeError("record probe module include changed: " + relative)
        source = source.replace(include, (fixture.parent / relative).read_text(), 1)
    return source


def main():
    """Run real dispatch and rooted interleaving in both runtime profiles."""
    runtime, compiler, include = (Path(arg).resolve() for arg in sys.argv[1:4])
    fixture = Path(__file__).with_suffix(".c").resolve()
    with tempfile.TemporaryDirectory(prefix="record-new-") as directory:
        work = Path(directory)
        guest, image = work / "main.b", work / "main.dis"
        guest.write_text(GUEST)
        subprocess.run(
            [str(compiler), "-I", str(include), "-o", str(image), str(guest)], check=True
        )
        delayed_fixture = work / "record-new.c"
        fixture_source = record_probe_source(fixture)
        if fixture_source.count("busy_publication = 1;") != 1:
            raise RuntimeError("allocation contention fixture anchor drift")
        delayed_fixture.write_text(
            fixture_source.replace("busy_publication = 1;", "busy_publication = 130;", 1)
        )
        binary = relink(
            runtime,
            work,
            delayed_fixture,
            0,
            [
                "dvm_runtime_root_registry_probe",
                "dvm_runtime_record_new",
                "dvm_runtime_heap_commit_attempt",
                "dvm_runtime_array_new",
                "dvm_runtime_root_batch_publish",
                "destroy",
                "fvm_port_sync_wait",
                "dvm_managed_workers_wake_guest",
                "dvm_managed_ranges_begin",
            ],
        )
        for mode in ("--no-jit", "--jit"):
            result = subprocess.run(
                [
                    str(binary),
                    mode,
                    "--guarded-copies",
                    "--gc-worker",
                    "--managed-owner-probe",
                    str(image),
                ],
                capture_output=True,
                text=True,
                timeout=90,
            )
            output = result.stdout + result.stderr
            if mode == "--jit" and "PASS guarded record JIT cache" not in output:
                raise RuntimeError(output)
            if (
                result.returncode
                or any(
                    marker not in output
                    for marker in (
                        "PASS buffered channel recovery on same channel",
                        "PASS detached ALT publication failure",
                        "PASS native channel buffer wait",
                        "PASS rooted channel buffer lifetime",
                        "PASS buffered channel completion",
                        "PASS direct channel mover failure",
                        "PASS two-sided channel finalization",
                        "PASS native Prog hold race",
                        "PASS guarded channel drain",
                        "PASS mapped channel receive ownership",
                        "PASS mapped channel send publication",
                        "PASS guarded mapped channel mover",
                        "PASS guarded channels",
                        "PASS guarded arrays",
                        "PASS array opcode route",
                        "PASS rooted record construction",
                        "PASS imported record lease",
                        "PASS record opcode route",
                        "PASS record guest",
                    )
                )
                or "ThreadSanitizer:" in output
                or "runtime error:" in output
            ):
                raise RuntimeError(output)
        verify_existing_shared_dispatch(runtime, work, image)
        concurrent_source, concurrent_image = work / "concurrent.b", work / "concurrent.dis"
        concurrent_source.write_text(CONCURRENT_GUEST)
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-o",
                str(concurrent_image),
                str(concurrent_source),
            ],
            check=True,
        )
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                command = [str(runtime), mode, "--native-spawn", "--stats"]
                if locked:
                    command.append("--gc-locked-poll")
                run = subprocess.run(
                    command + [str(concurrent_image)], capture_output=True, text=True, timeout=60
                )
                output = run.stdout + run.stderr
                if run.returncode or "PASS forty allocation workers" not in output:
                    raise RuntimeError(output)
                evidence(output, children=40)

    print(
        "PASS guarded records, arrays and channels, stale writer, refusal, monitor error and C/JIT guest"
    )


if __name__ == "__main__":
    main()
