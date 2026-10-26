"""Exercise a real interactive Limbo editor, actual files and balanced native shutdown."""

from pathlib import Path
import subprocess
import re
import sys
import tempfile


# Run typed commands against the compiled editor; inspect status/output and actual file bytes.
# argv supplies runtime, editor module and native/interpreter mode. Every session has a deadline.
def main():
    runtime, program, mode = sys.argv[1:]
    runtime = str(Path(runtime).resolve())
    program = Path(program).resolve()
    option = "--jit" if mode == "jit" else "--no-jit"

    def session(commands, expected=0):
        if isinstance(commands, str):
            commands = commands.encode("utf-8")
        result = subprocess.run(
            [runtime, option, "--stats", str(program)],
            cwd=program.parent,
            input=commands,
            capture_output=True,
            timeout=30,
        )
        output = result.stdout.decode("utf-8")
        error = result.stderr.decode("utf-8")
        counters = re.search(r"instructions=(\d+) native=(\d+)", error)
        if expected == 0 and (
            not counters
            or (mode == "jit" and int(counters[2]) == 0)
            or (mode != "jit" and int(counters[2]) != 0)
        ):
            raise RuntimeError("editor dispatch mode mismatch: " + error)
        if result.returncode != expected:
            raise RuntimeError("editor exit mismatch: " + output + error)
        if expected == 0 and (
            "BYE document owner stopped" not in output
            or "PASS Dis application completed" not in output
        ):
            raise RuntimeError("editor failed to stop its document owner: " + output + error)
        return output, error

    output, _ = session(
        "set Я🙂\\n\nappend beta\ninsert 1 X\ndelete 1 2\nundo\nredo\n"
        "replaceall beta|γ\nshow\nfind γ\nquit\nquit!\n"
    )
    if "TEXT 5\nЯ🙂\nγ\n\nEND" not in output or "FOUND offset=3 line=2 column=0" not in output:
        raise RuntimeError("real Unicode edit/search/history failed: " + output)
    if "DIRTY: save or use quit!" not in output:
        raise RuntimeError("dirty quit was silently accepted")
    output, _ = session(
        "complete as MovieClip.goto\nhint as MovieClip.gotoAndPlay 0\n"
        "complete limbo Sys.pr\nhint limbo Sys.open 1\n"
        "complete as Projection.to\nhint as Projection.tofloat 1\nquit\n"
    )
    for required in (
        "CANDIDATE MovieClip.gotoAndPlay",
        "CANDIDATE MovieClip.gotoAndStop",
        "PARAMETER 0 frame:Number|String",
        "CANDIDATE Sys.print",
        "PARAMETER 1 mode:int",
        "CANDIDATE Projection.tofloat",
        "CANDIDATE Projection.tobig",
        "PARAMETER 1 completion:function",
    ):
        if required not in output:
            raise RuntimeError("AS/Limbo declaration help failed: " + output)
    output, _ = session("set alpha\nwatch length big\nwatch text string\nwatchtable\n"
                        "watch undo method\ncallref undo 1\ncallref undo 2\nshow\nsetref 3 beta\nshow\nquit!\n")
    for required in ("WATCH integer 5", "WATCH string", "WATCH table 5", "WATCH ref console-document 1 undo",
                     "PROJECT status=2 revision=2", "TEXT 0\n\nEND", "TEXT 4\nbeta\nEND"):
        if required not in output:
            raise RuntimeError("live document inspector failed: " + required + "\n" + output)
    with tempfile.TemporaryDirectory(dir=program.parent) as temporary:
        root = Path(temporary)
        target = root / "saved source.b"
        target.write_bytes(b"old")
        output, _ = session(
            "set α\\nβ\nsave "
            + str(target)
            + "\nset changed\nload "
            + str(target)
            + "\nload! "
            + str(target)
            + "\nshow\nquit\n"
        )
        if target.read_bytes() != "α\nβ".encode("utf-8") or "TEXT 3\nα\nβ\nEND" not in output:
            raise RuntimeError("native save/load did not preserve exact UTF-8 bytes: " + output)
        if "DIRTY: save first or use load!" not in output:
            raise RuntimeError("load discarded unsaved edits without explicit force")
        relative = subprocess.run(
            [runtime, option, str(program)],
            cwd=root,
            input=b"set relative\nsave relative.b\nquit\n",
            capture_output=True,
            timeout=30,
        )
        if relative.returncode or (root / "relative.b").read_bytes() != b"relative":
            raise RuntimeError("module lookup changed the user file working directory")
        output, _ = session(
            "set stable\n"
            + ("save " + str(target) + "\n") * 40
            + ("load " + str(target) + "\n") * 40
            + "quit\n"
        )
        if "ERROR" in output or target.read_bytes() != b"stable":
            raise RuntimeError("repeated I/O leaked native descriptor owners")
        invalid = root / "invalid.b"
        invalid.write_bytes(b"\xff")
        output, _ = session("set keep\nload! " + str(invalid) + "\nshow\nquit!\n")
        if "ERROR invalid UTF-8" not in output or "TEXT 4\nkeep\nEND" not in output:
            raise RuntimeError("invalid file changed the document: " + output)
        output, _ = session("set keep\nsave " + str(root) + "\nquit\nquit!\n")
        if "ERROR save failed" not in output or "DIRTY:" not in output:
            raise RuntimeError("failed write marked the document saved: " + output)
        prefix = root / "prefix.b"
        prefix.write_bytes(b"untouched")
        output, _ = session(b"set changed\nsave " + str(prefix).encode() + b"\x00suffix\nquit!\n")
        if prefix.read_bytes() != b"untouched" or "ERROR save failed" not in output:
            raise RuntimeError("embedded NUL truncated the native path")
        binary = root / "controls.b"
        output, _ = session(b"set a\x00b\x1bc\nshow\nsave " + str(binary).encode() + b"\nquit\n")
        if "a␀b␛c" not in output or binary.read_bytes() != b"a\x00b\x1bc":
            raise RuntimeError("visible controls changed the saved source bytes")
        resource = root / "resource with spaces.dis"
        resource.write_bytes(bytes(range(32)))
        output, _ = session(
            "set keep\ninspect "
            + str(resource)
            + "\ninspect "
            + str(resource)
            + ".missing\nshow\nquit\nquit!\n"
        )
        for expected in (
            "RESOURCE size=32 offset=0 bytes=16",
            "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f",
            "ERROR inspect: cannot open file",
            "TEXT 4\nkeep\nEND",
            "DIRTY: save or use quit!",
        ):
            if expected not in output:
                raise RuntimeError(
                    "binary inspection changed text/history or preview failed: " + output
                )
        if resource.read_bytes() != bytes(range(32)):
            raise RuntimeError("read-only resource inspection modified its input")
        empty = root / "empty.b"
        output, _ = session("save " + str(empty) + "\nquit\n")
        if empty.read_bytes() != b"":
            raise RuntimeError("empty snapshot was not saved as an empty file")
        huge = root / "huge.b"
        huge.write_bytes(b"x" * (4 * 1048576 + 1))
        output, _ = session("set keep\nload! " + str(huge) + "\nshow\nquit!\n")
        if "ERROR source exceeds" not in output or "TEXT 4\nkeep\nEND" not in output:
            raise RuntimeError("oversized load changed the document")
    output, _ = session(b"set keep\n" + b"set " + b"x" * 4097 + b"\nset \xff\nshow\nquit!\n")
    if (
        output.count("ERROR command exceeds limit or contains invalid UTF-8") != 2
        or "TEXT 4\nkeep\nEND" not in output
    ):
        raise RuntimeError("invalid/oversized terminal input was partially applied")
    output, error = session("set unsaved\n", 1)
    if "EOF with unsaved document" not in error:
        raise RuntimeError("dirty EOF was not reported")
    session("")
    print(
        "PASS interactive Dis editor: Unicode, history, search, save/load, errors, bounds and shutdown "
        + mode
    )


if __name__ == "__main__":
    main()
