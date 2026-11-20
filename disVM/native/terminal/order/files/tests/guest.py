# Copyright (c) 2026 bhgv
# EN: Source-built SDK plus actual runtime interpreter/JIT acceptance, private linking only.
# RU: Проверка source-built SDK с настоящим runtime interpreter/JIT, только private-linking.
import fcntl
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile


# EN/RU: Read native CMake compiler metadata without guessing system or library paths.
# Прочитать metadata native CMake compiler без угадывания system/library paths.
def flags(directory):
    result = {}
    for line in (directory / "flags.make").read_text().splitlines():
        if " = " in line:
            key, value = line.split(" = ", 1)
            result[key] = shlex.split(value)
    return result


# EN/RU: Run bounded compilation/execution, report the actual diagnostic; build errors never count as semantic detection.
# Выполнить bounded compilation/execution с настоящей диагностикой, build-errors не считаются semantic detection.
def run(command, cwd, env=None):
    result = subprocess.run(command, cwd=cwd, env=env, capture_output=True, text=True, timeout=90)
    if result.returncode:
        raise RuntimeError("command failed: " + " ".join(map(str, command)) + "\n" + result.stdout + result.stderr)
    return result


# EN/RU: Exercise the full local descriptor budget and reject an unsealed file.
# Проверить полный размер данных через FD и отклонение незапечатанного файла.
def startup_fd(runtime, work):
    fd = os.memfd_create("disvm-startup-test", os.MFD_CLOEXEC | os.MFD_ALLOW_SEALING)
    try:
        payload = bytes(range(256)) * 256
        with os.fdopen(os.dup(fd), "wb") as stream:
            stream.write(payload)
        command = [str(runtime), f"--startup-fd={fd}", "--no-jit", "owner.dis"]
        refused = subprocess.run(command, cwd=work, pass_fds=(fd,), capture_output=True, timeout=5)
        if refused.returncode == 0 or b"PASS owner Dis init" in refused.stdout:
            raise RuntimeError("DisVM accepted an unsealed startup file")
        # EN/RU: Linux UAPI values for Python builds without exported seal constants.
        # Значения Linux UAPI для сборок Python без констант seals.
        add_seals = getattr(fcntl, "F_ADD_SEALS", 1033)
        seals = (getattr(fcntl, "F_SEAL_SEAL", 1) | getattr(fcntl, "F_SEAL_SHRINK", 2) |
                 getattr(fcntl, "F_SEAL_GROW", 4) | getattr(fcntl, "F_SEAL_WRITE", 8))
        fcntl.fcntl(fd, add_seals, seals)
        for mode in ("--no-jit", "--jit"):
            command[2] = mode
            accepted = subprocess.run(command, cwd=work, pass_fds=(fd,), capture_output=True, timeout=5)
            if accepted.returncode or b"PASS owner Dis init" not in accepted.stdout:
                raise RuntimeError("DisVM lost sealed startup bytes: " + accepted.stderr.decode())
    finally:
        os.close(fd)


# EN/RU: Compile a portable guest and link actual production objects with native launch/wire instrumentation.
# Собрать переносимый guest и связать production objects с native launch/wire instrumentation.
def main():
    runtime, compiler, include, sdk = [Path(value).resolve() for value in sys.argv[1:5]]
    root = Path(__file__).resolve().parents[6]
    runtime_root = root / "disVM/runtime"
    sdk_target = sdk / "terminal/order/files"
    metadata = runtime.parent / "CMakeFiles/disvm.dir"
    native_metadata = sdk_target / "CMakeFiles/terminal-ordered-files.dir"
    rt = flags(metadata)
    runtime_link = shlex.split((metadata / "link.txt").read_text())
    sdk_link = shlex.split((native_metadata / "link.txt").read_text())
    native = flags(native_metadata)
    cli = os.environ.get("DVM_TEST_TERMINAL_CLI")
    if cli:
        refused = subprocess.run([cli, "--terminal-launch-fd", "0", "fixture",
                                  "app.dis", "-", "--", "main.dis"],
                                 capture_output=True, text=True, timeout=5)
        if refused.returncode != 2 or "delegated connected stream FD" not in refused.stderr:
            raise RuntimeError("CLI did not reject stdin as launch carrier")
        for options in [
            ["--request", "launch", "0"],
            ["--records", "launch", "records"],
            ["--grant", "/zero", "7", "r", "--grant", "/seven", "7", "rw"],
            ["--grant", "/app.dis/sub", "7", "r"],
            ["--fd-grant", "/nine/child", "held-input", "r"],
            ["--fd-grant", "/input", ".unibus", "r"],
            ["--fd-grant", "/input", "app.dis", "r"],
            ["--fd-grant", "/input", "../held-input", "r"],
            ["--fd-grant", "/input", "held-input", "r", "--fd-grant", "/again", "held-input", "r"],
            ["--grant", "/reverse-exit", "7", "r"],
            ["--inherit-fd", "32", "r"],
            ["--inherit-fd", "03", "r"],
            ["--inherit-fd", "1", "r", "--inherit-fd", "1", "w"],
            ["--inherit-fd", "0", "r", "--request", ".fd/0", "17"],
            ["--fd-grant", "/input", ".fd/0", "r", "--inherit-fd", "0", "r"],
        ]:
            refused = subprocess.run([cli, "--terminal-launch-fd", "0", "fixture", "app.dis", "-",
                                      *options, "--", "main.dis"], capture_output=True, text=True, timeout=5)
            if refused.returncode != 2 or not refused.stderr.startswith("usage:"):
                raise RuntimeError("CLI accepted invalid launch configuration: " + " ".join(options))
    with tempfile.TemporaryDirectory(prefix="unibus-guest-") as name:
        work = Path(name)
        examples = root / "disVM/examples/vfs-run"
        modules = root / "disVM/module"
        for source in [examples / "runexample.b", root / "disVM/limbo/vfsctl.b"]:
            run([str(compiler), "-I", str(include), "-I", str(modules), "-I", str(examples),
                 "-o", str(work / (source.stem + ".dis")), str(source)], work)
        run([str(compiler), "-I", str(include), "-I", str(examples), "-o", str(work / "main.dis"),
             "-I", str(runtime_root / "fixtures/provider-modules"), "-I", str(root / "disVM/module"),
             str(runtime_root / "fixtures/unibus-files/main.b")], work)
        value_source = work / "value.b"
        value_source.write_text((runtime_root / "fixtures/provider-modules/constant.b").read_text().replace("EXPECTED_VALUE", "111"))
        for source, output in [(value_source, "dep.dis"), (runtime_root / "fixtures/provider-modules/app.b", "app.dis")]:
            run([str(compiler), "-I", str(include), "-I", str(runtime_root / "fixtures/provider-modules"),
                 "-o", str(work / output), str(source)], work)
        owner = work / "owner"
        owner.mkdir()
        (owner / "owner-only.txt").write_bytes(b"owner")
        run([str(compiler), "-I", str(include), "-I", str(modules), "-o", str(owner / "owner.dis"),
             str(runtime_root / "fixtures/unibus-files/owner.b")], work)
        startup_fd(runtime, owner)
        wrapper = work / "wrapper.o"
        run([runtime_link[0], *rt["C_DEFINES"], *rt["C_INCLUDES"], *native["C_INCLUDES"], *rt["C_FLAGS"],
             "-c", str(runtime_root / "test_unibus_files.c"), "-o", str(wrapper)], work)
        objects = [wrapper]
        for source in [root / "disVM/native/terminal/tests/link/fixture.c",
                       root / "disVM/native/terminal/order/broker/tests/wire.c",
                       root / "disVM/native/terminal/tests/link/images.c",
                       root / "disVM/native/terminal/tests/link/reverse.c"]:
            obj = work / (source.stem + ".o")
            run([sdk_link[0], *native["C_DEFINES"], *native["C_INCLUDES"], *native["C_FLAGS"],
                 "-DORDERED_GUEST_FILE_TEST=1", "-c", str(source), "-o", str(obj)], work)
            objects.append(obj)
        link = [value for value in shlex.split((metadata / "link.txt").read_text())
                if not value.startswith("-Wl,--dependency-file=")]
        link[1:1] = [value for value in native["C_FLAGS"] if value.startswith("-fsanitize=")]
        link[link.index("-o") + 1] = str(work / "disvm-files")
        libraries = [str((sdk_target / value).resolve()) for value in sdk_link if value.endswith(".a")]
        link += [str(value) for value in objects]
        link += ["-Wl,--wrap=main",
                 "-Wl,--start-group", *libraries, "-Wl,--end-group",
                 *[value for value in sdk_link if value.startswith("-l") or value == "-pthread"]]
        run(link, runtime.parent)
        profiles = ["default", "custom", "numbered"] if cli else ["default", "numbered"]
        for profile in profiles:
            env = os.environ.copy()
            if profile == "custom":
                env["DVM_TEST_TERMINAL_CUSTOM"] = "1"
            if profile == "numbered":
                env["DVM_TEST_TERMINAL_NUMBERED"] = "1"
            for mode in ["--no-jit", "--jit"]:
                env["DVM_TEST_OWNER_RUNTIME"] = str(runtime)
                env["DVM_TEST_OWNER_MODE"] = mode
                (work / "held-source.bin").write_bytes(b"skip:from-held-cursor")
                (work / "held-target.bin").write_bytes(b"keep:")
                if profile == "numbered":
                    (work / "numbered-in.bin").write_bytes(b"skip:xyz")
                    (work / "numbered-out.bin").write_bytes(b"keep:")
                    (work / "numbered-err.bin").write_bytes(b"keep:")
                    (work / "numbered-rw.bin").write_bytes(b"keep:abcdef")
                result = run([str(work / "disvm-files"), "--native-spawn", mode, str(work / "main.dis")],
                             work, env)
                guest_output = result.stdout
                if profile == "numbered":
                    guest_output = (work / "numbered-out.bin").read_text()
                    if not guest_output.startswith("keep:") or "remote stdout\n" not in guest_output or \
                            "remote stdout\n" in result.stdout or \
                            (work / "numbered-err.bin").read_bytes() != b"keep:remote stderr\n" or \
                            (work / "numbered-rw.bin").read_bytes() != b"keep:abcdef!":
                        raise RuntimeError("numbered stdio or shared cursor mismatch")
                if not cli and result.stdout.count("PASS owner Dis init") != 1:
                    raise RuntimeError("owner VM did not initialize the published command exactly once")
                if not cli and "PASS path-only Styx command submission" not in guest_output:
                    raise RuntimeError("guest did not submit by its VFS alias")
                if "PASS real Dis guest file Unibus bidirectional records" not in guest_output or \
                        "PASS transferred Sys FD retained source position" not in guest_output or \
                        "PASS real Dis Unibus file native ownership drain" not in result.stdout:
                    raise RuntimeError("missing guest/lifetime evidence: " + result.stdout + result.stderr)
                if (work / "held-target.bin").read_bytes() != b"keep:written-with-held-fd":
                    raise RuntimeError("write to transferred FD reset or truncated its original cursor")
                print("PASS actual portable Dis guest record files " + profile + " " + mode, flush=True)


if __name__ == "__main__":
    main()
