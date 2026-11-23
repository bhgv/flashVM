# Copyright (c) 2026 bhgv
# EN: Relink a source-matched runtime with the explicit Styx publication fixture.
# RU: Связать runtime с явным стендом публикации Styx.
import os
from pathlib import Path
from runpy import run_path
import shlex
import sys
import tempfile

helpers = run_path(str(Path(__file__).resolve().parents[2] / "order/files/tests/guest.py"))
flags = helpers["flags"]
run = helpers["run"]


# EN/RU: Build the example and run the complete exchange in both runtime modes.
# Собрать пример и выполнить полный обмен в обоих режимах runtime.
def main():
    runtime, compiler, sdk = map(lambda arg: Path(arg).resolve(), sys.argv[1:4])
    flash = Path(sys.argv[4]).resolve() if len(sys.argv) == 5 else None
    here = Path(__file__).resolve().parent
    disvm = here.parents[3]
    native = sdk / "terminal/CMakeFiles/terminal-scalar-route.dir"
    runtime_target = runtime.parent / "CMakeFiles/disvm.dir"
    native_link = shlex.split((native / "link.txt").read_text())
    rt_link = shlex.split((runtime_target / "link.txt").read_text())
    options = flags(native)
    with tempfile.TemporaryDirectory(prefix="styx-variable-") as directory:
        work = Path(directory)
        program = work / "variable.dis"
        run([str(compiler), "-I", str(disvm / "upstream/module"), "-I", str(disvm / "module"),
             "-o", str(program), str(disvm / "examples/remote-scalar/styx-variable.b")], work)
        obj = work / "peer.o"
        run([native_link[0], *options["C_DEFINES"], *options["C_INCLUDES"], *options["C_FLAGS"],
             "-c", str(here / "peer.c"), "-o", str(obj)], work)
        executable = work / "disvm"
        link = [arg for arg in rt_link if not arg.startswith("-Wl,--dependency-file=")]
        link[link.index("-o") + 1] = str(executable)
        libraries = [str((sdk / "terminal" / arg).resolve()) for arg in native_link if arg.endswith(".a")]
        link += [str(obj), "-Wl,--wrap=main", "-Wl,--wrap=dvm_stack_bind",
                 "-Wl,--start-group", *libraries, "-Wl,--end-group",
                 *[arg for arg in native_link if arg.startswith("-l") or arg == "-pthread"]]
        run(link, runtime.parent)
        for mode in ("--no-jit", "--jit"):
            result = run([str(executable), mode, str(program)], work)
            run([str(executable), mode, str(program)], work, dict(os.environ, DVM_TEST_VARIABLE_LEGACY="1"))
            marker = "PASS Limbo variable round trip over Styx"
            if marker not in result.stdout:
                raise RuntimeError(marker + " missing\n" + result.stdout + result.stderr)
            print(marker, mode, flush=True)
            if flash:
                result = run([str(executable), mode, str(program)], work,
                             dict(os.environ, DVM_TEST_VARIABLE_FLASH_HOST=str(flash)))
                if marker not in result.stdout or "PASS FlashVM Styx RPC host" not in result.stdout:
                    raise RuntimeError("FlashVM variable bridge did not complete\n" + result.stdout + result.stderr)
                print("PASS FlashVM and Limbo shared variable", mode, flush=True)


if __name__ == "__main__":
    main()
