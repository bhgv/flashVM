# Copyright (c) 2026 bhgv
# EN: Verify MAUTORWX against independent real host permissions and module code.
# RU: Проверять MAUTORWX по независимым host permissions и module code.
from pathlib import Path
import os
import selectors
import subprocess
import sys
import tempfile
from test_owner_location import compile_image
from test_native_kill import wait_marker


# EN/RU: Reset host data/modes for one VM run, independent from guest routing.
# Return no handles; work owns files until temporary-directory cleanup.
# Сбросить host data/modes перед VM run; work владеет файлами до cleanup.
def prepare(work):
    for name in ("a", "b", "c"):
        directory = work / name
        directory.chmod(0o700)
        (directory / "create").chmod(0o700)
        for leaf, payload in (("common", name.upper()), ("masked", name.upper()), ("denied", "D")):
            path = directory / leaf
            if path.exists():
                path.chmod(0o600)
            path.write_text(payload)
        (directory / "common").chmod({"a": 0o400, "b": 0o200, "c": 0o600}[name])
        (directory / "masked").chmod(0o004 if name == "a" else 0o400)
        (directory / "denied").chmod(0)
        for leaf in ("new", "upper", "lower-existing", "exclusive", "collision"):
            (directory / "create" / leaf).unlink(missing_ok=True)
        (directory / "tool.dis").chmod(0o644 if name == "a" else 0o744)
    for leaf in ("exclusive", "lower-existing"):
        (work / "c/create" / leaf).write_text("C")
    (work / "a/create/collision").write_text("A")
    (work / "a/create").chmod(0o503)  # Owner cannot write despite other w+x.
    (work / "b/create").chmod(0o600)  # Write without directory search is insufficient.


# EN/RU: Run one owner VM, change chmod only at its explicit barrier, verify host
# bytes and absence of lower fallback effects; restore modes even on failure.
# Запустить VM, chmod только на barrier, проверить bytes/отсутствие lower effects;
# восстановить modes даже при отказе. Inputs borrowed, child owned to exit.
def execute(runtime, image, work, mode, locked):
    prepare(work)
    output, errors = bytearray(), bytearray()
    process = subprocess.Popen(
        [str(runtime), mode, "--stats", *(["--gc-locked-poll"] if locked else []), str(image)],
        cwd=work, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        with selectors.DefaultSelector() as poll:
            poll.register(process.stdout, selectors.EVENT_READ, output)
            poll.register(process.stderr, selectors.EVENT_READ, errors)
            wait_marker(poll, output, errors, b"AUTO_CHMOD_READY\n", in_output=True)
            (work / "a/common").chmod(0o200)
            (work / "b/common").chmod(0o400)
            (work / "a/create").chmod(0o700)
            (work / "a/tool.dis").chmod(0o744)
            process.stdin.write(b"1")
            process.stdin.flush()
        tail, diagnostics = process.communicate(timeout=45)
        text = (output + errors + tail + diagnostics).decode(errors="replace")
        if process.returncode or "PASS MAUTORWX " not in text or "ThreadSanitizer:" in text:
            raise RuntimeError(text[-18000:])
        for name in ("a", "b", "c"):
            (work / name / "common").chmod(0o600)
            (work / name / "create").chmod(0o700)
        if [(work / name / "common").read_text() for name in ("a", "b", "c")] != ["Z", "W", "C"]:
            raise RuntimeError("automatic read/write selected incorrect host files")
        for relative, expected in (("c/create/new", "N"), ("a/create/upper", "U"),
                                   ("a/create/lower-existing", "L"), ("c/create/lower-existing", "C"),
                                   ("c/create/exclusive", "C"), ("a/create/collision", "A")):
            if (work / relative).read_text() != expected:
                raise RuntimeError("incorrect create bytes: " + relative)
        for relative in ("a/create/new", "b/create/new", "b/create/upper", "c/create/upper",
                         "a/create/exclusive", "b/create/exclusive", "b/create/collision", "c/create/collision"):
            if (work / relative).exists():
                raise RuntimeError("unexpected create/fallback: " + relative)
        print(f"PASS MAUTORWX {mode} locked={locked}")
    finally:
        if process.poll() is None:
            process.kill()
            process.communicate()
        for name in ("a", "b", "c"):
            (work / name).chmod(0o700)
            (work / name / "create").chmod(0o700)


# EN/RU: Compile distinct module identities and real fixture, own work until all
# interpreter/JIT/poll runs end. Require non-root permission enforcement.
# Собрать разные module identities и fixture; владеть work до конца всех modes.
def main():
    if os.geteuid() == 0:
        raise RuntimeError("MAUTORWX permission acceptance requires a non-root host")
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    example = root.parent / "examples/namespace-load"
    with tempfile.TemporaryDirectory(prefix="union autorwx ") as temporary:
        work = Path(temporary)
        for name in ("a", "b", "c", "view", "mirror", "plain"):
            (work / name).mkdir()
            (work / name / "create").mkdir()
        for name in ("a", "b", "c"):
            compile_image(compiler, include, root, example / f"provider-{'a' if name == 'a' else 'b'}.b",
                          work / name / "tool.dis", (example,))
        image = work / "main.dis"
        compile_image(compiler, include, root, root / "fixtures/union-auto/main.b", image, (example,))
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                execute(runtime, image, work, mode, locked)


if __name__ == "__main__":
    main()
