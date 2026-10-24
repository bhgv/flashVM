#!/usr/bin/env python3
"""Independent host-side source-to-SWF build driver for editors (Python 3.9+)."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess
import tempfile

SOURCE_LIMIT = 65536
LOG_LIMIT = 65536
LANGUAGES = ("as1", "as2", "as3", "limbo")


class BuildFailure(Exception):
    """Carry a stage failure through temporary-directory cleanup."""


def _diagnostic(stage, message, source=None):
    """Preserve raw compiler text and add coordinates only when explicitly reported."""
    item = {"severity": "error", "stage": stage, "message": message}
    if stage == "source":
        match = re.search(r"source:(\d+):(\d+):", message)
        line = match.group(1) if match else None
        if match:
            item["column"] = int(match.group(2))
        else:
            match = re.search(r"source line (\d+):", message)
            line = match.group(1) if match else None
        if line and int(line) > 0:
            item["line"] = int(line)
            item["file"] = str(source)
    return item


def _text(stream):
    """Read bounded diagnostic bytes without retaining an unbounded child log in RAM."""
    stream.seek(0)
    data = stream.read(LOG_LIMIT + 1)
    return data[:LOG_LIMIT].decode("utf-8", errors="replace"), len(data) > LOG_LIMIT


def _run(stage, argv, cwd, timeout, result):
    """Wait for one direct child; kill and reap it on timeout or cancellation."""
    record = {"stage": stage, "argv": list(map(str, argv))}
    result["steps"].append(record)
    with tempfile.TemporaryFile(dir=cwd) as stdout, tempfile.TemporaryFile(dir=cwd) as stderr:
        process = subprocess.Popen(record["argv"], cwd=cwd, stdin=subprocess.DEVNULL,
                                   stdout=stdout, stderr=stderr, shell=False,
                                   env=dict(os.environ, TMPDIR=str(cwd), TEMP=str(cwd), TMP=str(cwd)))
        try:
            process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
            record["timed_out"] = True
        except BaseException:
            process.kill()
            process.wait()
            raise
        record["returncode"] = process.returncode
        record["stdout"], record["stdout_truncated"] = _text(stdout)
        record["stderr"], record["stderr_truncated"] = _text(stderr)
    if record.get("timed_out"):
        raise BuildFailure("stage exceeded its timeout")
    if record["returncode"]:
        raise BuildFailure(record["stderr"] or record["stdout"] or
                           "tool exited with status %s" % record["returncode"])
    return record


def _artifact(path, signature=None):
    """Reject missing/empty stage outputs even if a broken tool reports success."""
    if path.is_symlink() or not path.is_file() or not path.stat().st_size:
        raise BuildFailure("tool produced no nonempty artifact")
    if signature:
        with path.open("rb") as stream:
            if stream.read(len(signature)) != signature:
                raise BuildFailure("tool produced an unexpected artifact signature")


def build(source, output, language, compiler, ir, linker, module="main", timeout=30,
          optimise=True, runner=None, expect_trace=None):
    """Build one source snapshot; publish only after all selected stages succeed.

    Returned dictionaries belong to the caller. No shell, global parser state or
    implicit sibling paths are used. Direct children are owned until reaped.
    """
    result = {"schema": 1, "ok": False, "published": False, "steps": [], "diagnostics": []}
    stage = "input"
    try:
        source = Path(source).expanduser().resolve(strict=True)
        raw_output = Path(output).expanduser().absolute()
        if raw_output.is_symlink():
            raise BuildFailure("output must not be a symbolic link")
        output = raw_output.resolve()
        result.update(source=str(source), output=str(output), language=language, module=module)
        if language not in LANGUAGES:
            raise BuildFailure("unknown source language")
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_.]{0,253}", module):
            raise BuildFailure("invalid module name")
        if not math.isfinite(timeout) or timeout <= 0:
            raise BuildFailure("timeout must be finite and positive")
        if expect_trace is not None and runner is None:
            raise BuildFailure("expected trace requires a runner")
        if not output.parent.is_dir():
            raise BuildFailure("output parent directory does not exist")
        tools = [Path(p).expanduser().resolve(strict=True) for p in (compiler, ir, linker)]
        if runner is not None:
            runner = Path(runner).expanduser().resolve(strict=True)
            tools.append(runner)
        for path in [source] + tools:
            if output == path or (output.exists() and output.samefile(path)):
                raise BuildFailure("output aliases a source or executable")
        with source.open("rb") as stream:
            data = stream.read(SOURCE_LIMIT + 1)
        if len(data) > SOURCE_LIMIT:
            raise BuildFailure("source exceeds the numeric profile's 64 KiB limit")
        # Stage beside the destination so the final replace stays on one filesystem.
        with tempfile.TemporaryDirectory(prefix=".ide-build-", dir=output.parent) as directory:
            work = Path(directory)
            snapshot = work / "input.src"
            snapshot.write_bytes(data)
            original, optimised, obj, movie = [work / name for name in
                                               ("source.fir", "optimised.fir", "module.swo", "movie.swf")]
            stage = "source"
            _run(stage, [tools[0], language, "--input", snapshot, original, "--module", module], work, timeout, result)
            _artifact(original)
            stage = "verify"
            _run(stage, [tools[1], "check", original], work, timeout, result)
            selected = original
            if optimise:
                stage = "optimise"
                _run(stage, [tools[1], "opt", original, optimised], work, timeout, result)
                _artifact(optimised)
                stage = "verify_optimised"
                _run(stage, [tools[1], "check", optimised], work, timeout, result)
                selected = optimised
            stage = "object"
            _run(stage, [tools[1], "object", selected, obj], work, timeout, result)
            _artifact(obj, b"SWO1")
            stage = "link"
            _run(stage, [tools[2], "link", movie, obj], work, timeout, result)
            _artifact(movie, b"FWS\x08")
            with movie.open("rb") as stream:
                header = stream.read(8)
            if len(header) != 8 or int.from_bytes(header[4:8], "little") != movie.stat().st_size:
                raise BuildFailure("SWF declared length does not match the artifact")
            if runner is not None:
                stage = "run"
                record = _run(stage, [runner, movie], work, timeout, result)
                if expect_trace is not None and (record["stdout_truncated"] or record["stdout"] != expect_trace):
                    raise BuildFailure("runner trace did not match the expected text")
            stage = "publish"
            digest = hashlib.sha256()
            with movie.open("rb") as stream:
                for block in iter(lambda: stream.read(LOG_LIMIT), b""):
                    digest.update(block)
            checksum = digest.hexdigest()
            os.replace(movie, output)
            result.update(ok=True, published=True, sha256=checksum)
    except (BuildFailure, OSError, ValueError) as error:
        result["diagnostics"].append(_diagnostic(stage, str(error), source))
    except KeyboardInterrupt:
        result["diagnostics"].append(_diagnostic(stage, "build cancelled", source))
    return result


def main(argv=None):
    """Emit exactly one JSON result for a parsed build request; return 0 or 1."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source")
    parser.add_argument("output")
    parser.add_argument("--language", choices=LANGUAGES, required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--ir", required=True)
    parser.add_argument("--linker", required=True)
    parser.add_argument("--module", default="main")
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--no-optimise", dest="optimise", action="store_false")
    parser.add_argument("--runner")
    parser.add_argument("--expect-trace")
    args = vars(parser.parse_args(argv))
    result = build(**args)
    print(json.dumps(result, ensure_ascii=False))
    return 0 if result["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
