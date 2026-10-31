"""Execute real task-owner paths near identity exhaustion with a private counter fixture."""

from pathlib import Path
import subprocess
import sys
import tempfile


# CLI supplies runtime, matched compiler, application interfaces, Sys interfaces,
# production owner source, fixture program and interpreter/JIT mode. Return zero
# only after both bytecode compilations and the real runtime predicate gate pass.
# Change one counted initialization expression in a temporary source; never edit
# the production owner or expose a caller-controlled seed in its public API.
def main():
    runtime, compiler, interfaces, system, source, fixture = (
        Path(value).resolve() for value in sys.argv[1:7]
    )
    mode = sys.argv[7]
    if mode not in ("jit", "interpreter"):
        raise RuntimeError("unknown task identity-limit mode")
    anchor = "ref State(array[Capacity] of Task, 0, 0)"
    text = source.read_text()
    if text.count(anchor) != 1:
        raise RuntimeError("private task initial-counter fixture anchor changed")
    with tempfile.TemporaryDirectory(prefix="asinc identity limit ") as temporary:
        work = Path(temporary)
        owner = work / "asinctasks.b"
        owner.write_text(text.replace(anchor, "ref State(array[Capacity] of Task, 0, 2147483646)"))
        for input_path, output in ((owner, "asinctasks.dis"), (fixture, "limit-test.dis")):
            subprocess.run(
                [str(compiler), "-g", "-I", str(interfaces), "-I", str(system),
                 "-o", str(work / output), str(input_path)],
                cwd=work, check=True, timeout=30,
            )
        subprocess.run(
            [sys.executable, str(Path(__file__).with_name("test_application.py")),
             str(runtime), str(work / "limit-test.dis"), mode],
            check=True, timeout=60,
        )


if __name__ == "__main__":
    main()
