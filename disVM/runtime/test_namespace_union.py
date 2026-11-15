# Copyright (c) 2026 bhgv
# EN: Real native union fixture with independent host data and chmod handshake.
# RU: Native union fixture с независимыми host data и chmod handshake.
from pathlib import Path
import os
import selectors
import subprocess
import sys
import tempfile
from test_owner_location import compile_image
from test_native_kill import wait_marker


# EN/RU: Own temporary trees and VM children; chmod only after explicit guest ready.
# Require actual host bytes/creation targets and zero resource leaks.
# Владеть деревьями/VM children; chmod только после guest ready, проверить bytes/targets.
def main():
    if os.geteuid() == 0:
        raise RuntimeError("union permission acceptance requires non-root host user")
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="namespace union ") as temporary:
        work = Path(temporary)
        image = work / "main.dis"
        compile_image(compiler, include, root, root / "fixtures/namespace-union/main.b", image)
        for name in ("a", "b", "c", "view", "mirror", "fresh", "after", "replace"):
            (work / name).mkdir()
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                for name, token in (("a", "A"), ("b", "B"), ("c", "C"), ("view", "V"), ("mirror", "M"), ("fresh", "F"), ("after", "H"), ("replace", "R")):
                    path = work / name / "token"
                    if path.exists():
                        path.chmod(0o600)
                    path.write_text(token)
                    path.chmod(0o444 if name in ("a", "b") else 0o666)
                for name in ("a", "b", "c"):
                    (work / name / (name + "-only")).write_text(name.upper())
                    (work / name / "new").unlink(missing_ok=True)
                    (work / name / "no-create").unlink(missing_ok=True)
                (work / "fresh/f-only").write_text("F")
                output, errors = bytearray(), bytearray()
                process = subprocess.Popen([str(runtime), mode, "--stats",
                                            *(["--gc-locked-poll"] if locked else []), str(image)],
                                           cwd=work, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                try:
                    with selectors.DefaultSelector() as poll:
                        poll.register(process.stdout, selectors.EVENT_READ, output)
                        poll.register(process.stderr, selectors.EVENT_READ, errors)
                        wait_marker(poll, output, errors, b"UNION_CHMOD_READY\n", in_output=True)
                        (work / "a/token").chmod(0o666)
                        process.stdin.write(b"1")
                        process.stdin.flush()
                    after, diagnostics = process.communicate(timeout=30)
                    text = (output + errors + after + diagnostics).decode(errors="replace")
                    if process.returncode or "PASS union order/" not in text or "ThreadSanitizer:" in text:
                        raise RuntimeError(text[-18000:] + "\nHOST=" + repr([(work / name / "token").read_text() for name in ("a", "b", "c")]))
                    if (work / "c/new").read_text() != "N" or any((work / name / "new").exists() for name in ("a", "b")):
                        raise RuntimeError("MCREATE wrote to wrong branch")
                    if any((work / name / "no-create").exists() for name in ("a", "b", "c")):
                        raise RuntimeError("create without MCREATE changed source")
                    if [(work / name / "token").read_text() for name in ("a", "b", "c", "view")] != ["T", "B", "C", "V"]:
                        raise RuntimeError("union write fallback or truncation modified wrong layer")
                    print(f"PASS namespace union {mode} locked={locked}")
                finally:
                    if process.poll() is None:
                        process.kill()
                        process.communicate()
                    (work / "a/token").chmod(0o600)
                    (work / "b/token").chmod(0o600)


if __name__ == "__main__":
    main()
