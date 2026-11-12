# Copyright (c) 2026 bhgv
# [GC:] EN: Build real world lifecycle probes and exact guard omission controls.
# RU: Сборка настоящих проверок жизненного цикла мира и удаления его защит.
import argparse
import itertools
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from world_source import read_world_source


# [GC:] EN: Compile borrowed command arguments; return after success, otherwise
# raise with the actual compiler output. No runtime failure counts as a build.
# RU: Собрать заимствованную команду; вернуться при успехе, иначе вызвать
# исключение с настоящим выводом компилятора. Сбой runtime не заменяет сборку.
def build(command):
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError("sealed borrow compilation failed\n" + result.stdout + result.stderr)


# [GC:] EN: Borrow explicit source roots and own private binaries until exit.
# Run both world policies, poll modes and controller phases; negative controls
# must return exact status 12 and their invariant, never crash/timeout/sanitizer.
# --positive-only omits intentional violations for sanitizer builds.
# RU: Заимствовать заданные корни исходников; владеть бинарниками до выхода.
# Проверить обе политики мира, опроса и фазы контроллера. Контроли обязаны дать
# ровно код 12 и свой инвариант, а не сбой/таймаут/санитайзер. --positive-only
# исключает намеренные нарушения при проверке санитайзером.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--flashvm-root", type=Path, required=True)
    parser.add_argument("--positive-only", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    fvm = args.flashvm_root.resolve()
    original = read_world_source(root / "host")
    cases = [("production", original, None)]
    if not args.positive_only:
        gates = [
            (
                "late",
                "!world->sealed && participant->read_depth != ULONG_MAX",
                "participant->read_depth != ULONG_MAX",
                "controller borrow reopened sealed world",
            ),
            (
                "service",
                "!world->sealed && participant->read_depth != ULONG_MAX",
                "(!world->sealed || participant->service) && participant->read_depth != ULONG_MAX",
                "service borrow reopened sealed world",
            ),
            (
                "self",
                "participant->shared || participant->read_depth ||",
                "participant->shared ||",
                "controller sealed with its own borrow",
            ),
        ]
        for name, anchor, changed, marker in gates:
            if original.count(anchor) != 1:
                raise RuntimeError("sealing omission anchor changed: " + name)
            cases.append((name, original.replace(anchor, changed), marker))
    cpp_source = """// Copyright (c) 2026 bhgv
// [GC:] EN: C++98 public epoch ABI with a real idle participant.
// RU: Публичный ABI эпох C++98 с настоящим idle-участником.
#include "disvm/native_world.h"


// [GC:] EN: Borrow world/participant; return one after matched begin/end and
// completed epoch. No allocation or owner transfer; call before sealing.
// RU: Заимствовать world/participant; один после парного begin/end и
// завершения эпохи. Без выделения/передачи владения; вызывать до sealing.
extern "C" int dvm_test_cpp_epochs(DVM_NativeWorld *world, DVM_WorldParticipant *participant)
{
    if (!dvm_world_read_begin(participant))
    {
        return 0;
    }
    int borrowed = dvm_world_has_borrowers(world);
    unsigned long epoch = dvm_world_retire_epoch(world);
    int ended = dvm_world_read_end(participant);
    return borrowed && epoch && ended && dvm_world_epoch_elapsed(world, epoch);
}
"""
    with tempfile.TemporaryDirectory(prefix="native sealed borrow ") as folder:
        work = Path(folder)
        cpp = work / "epoch_client.cpp"
        cpp.write_text(cpp_source)
        cpp_object = work / "epoch_client.o"
        cxx = shlex.split(os.environ.get("CXX", "c++"))
        cxx += shlex.split(os.environ.get("CFLAGS", "-O1 -g"))
        cxx += [
            "-std=c++98",
            "-fno-exceptions",
            "-fno-rtti",
            "-I" + str(root / "include"),
            "-c",
            str(cpp),
            "-o",
            str(cpp_object),
        ]
        build(cxx)
        for name, code, marker in cases:
            world = work / (name + ".c")
            world.write_text(code)
            binary = work / name
            command = shlex.split(os.environ.get("CC", "cc"))
            command += shlex.split(os.environ.get("CFLAGS", "-O1 -g"))
            command += ["-DDVM_NATIVE_REGISTERS=1", "-pthread"]
            command += [
                "-I" + str(path)
                for path in (
                    root / "host",
                    root / "host/include",
                    root / "upstream/include",
                    root / "upstream/libinterp",
                    root / "upstream/emu/port",
                    root / "include",
                    fvm / "include",
                    fvm / "ports",
                )
            ]
            command += [str(root / "tests/native_seal.c"), str(world), str(cpp_object)]
            command += [
                str(root / path)
                for path in (
                    "host/native_registers.c",
                    "host/native_context.c",
                )
            ]
            command += [
                str(fvm / "ports/linux/thread.c"),
                str(fvm / "ports/linux/channel_sync.c"),
                "-o",
                str(binary),
            ]
            build(command)
            if name == "production" and not args.positive_only:
                header = (root / "include/disvm/native_world.h").read_text()
                closing = "#ifdef __cplusplus\n}\n#endif"
                if header.count(closing) != 1:
                    raise RuntimeError("public epoch linkage boundary changed")
                header = header.replace(closing, "").replace(
                    "int dvm_world_read_begin(", closing + "\n\nint dvm_world_read_begin(", 1
                )
                header = header.replace(
                    '#include "native_context.h"',
                    '#include "' + str(root / "include/disvm/native_context.h") + '"',
                )
                legacy_header = work / "legacy_epoch.h"
                legacy_header.write_text(header)
                legacy_cpp = work / "legacy_epoch.cpp"
                legacy_cpp.write_text(
                    cpp_source.replace('"disvm/native_world.h"', '"legacy_epoch.h"')
                )
                legacy_object = work / "legacy_epoch.o"
                legacy_compile = [
                    (
                        str(legacy_cpp)
                        if item == str(cpp)
                        else str(legacy_object) if item == str(cpp_object) else item
                    )
                    for item in cxx
                ]
                build(legacy_compile)
                legacy_link = [
                    (
                        str(legacy_object)
                        if item == str(cpp_object)
                        else str(work / "legacy_link") if item == str(binary) else item
                    )
                    for item in command
                ]
                result = subprocess.run(legacy_link, capture_output=True, text=True)
                symbols = (
                    "read_begin",
                    "read_end",
                    "has_borrowers",
                    "retire_epoch",
                    "epoch_elapsed",
                )
                if (
                    result.returncode == 0
                    or "undefined reference" not in result.stderr
                    or not all("dvm_world_" + symbol in result.stderr for symbol in symbols)
                ):
                    raise RuntimeError(
                        "C++ epoch linkage omission was not detected\n" + result.stderr
                    )
            for profile in itertools.product(
                ("atomic", "locked"), ("reference", "concurrent"), ("active", "idle")
            ):
                result = subprocess.run(
                    [str(binary), *profile], capture_output=True, text=True, timeout=15
                )
                output = result.stdout + result.stderr
                if "Sanitizer" in output or "runtime error:" in output:
                    raise RuntimeError("sealed borrow sanitizer failure\n" + output[-8000:])
                expected = 12 if marker else 0
                wanted = marker or "PASS sealed borrows: nested=2 joined=1 late=0 registers=stable"
                if result.returncode != expected or wanted not in output:
                    raise RuntimeError(
                        f"{name} {profile}: exit={result.returncode}\n{output[-8000:]}"
                    )
    print(
        "PASS sealed borrows: eight lifecycle profiles"
        + (
            ""
            if args.positive_only
            else "; 24 exact omission controls; one C++ ABI omission control"
        )
    )


if __name__ == "__main__":
    main()
