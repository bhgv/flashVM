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


# Carry a stage failure through temporary-directory cleanup.
class BuildFailure(Exception):
    """Carry a stage failure through temporary-directory cleanup."""


# Preserve raw compiler text and add coordinates only when explicitly reported.
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


# Reject duplicate JSON keys rather than silently accepting the final spelling.
def _unique_object(pairs):
    """Build a JSON object with an unambiguous field value for every key."""
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("duplicate diagnostic key")
        result[key] = value
    return result


# Admit coordinates only when a complete record agrees with the captured bytes.
def _source_diagnostic(message, data, source):
    """Validate schema 1, preserve byte spans and add Unicode-character positions.

    Malformed, unlocated or foreign diagnostics retain raw text without guessed
    positions. A caller must compare source_sha256 (and its own live revision)
    before using a late result to navigate or edit a live document.
    """
    item = {"severity": "error", "stage": "source", "message": message}
    try:
        record = json.loads(message, object_pairs_hook=_unique_object)
        if not isinstance(record, dict) or type(record.get("schema")) is not int:
            return item
        kinds = {"argument", "input", "encoding", "limit", "resource", "syntax", "semantic", "output"}
        if record["schema"] != 1 or record.get("kind") not in kinds:
            return item
        if not isinstance(record.get("message"), str):
            return item
        record["message"].encode("utf-8")
        for key in ("code", "byte_offset", "byte_length", "line", "column"):
            if type(record.get(key)) is not int or not 0 <= record[key] <= 2147483647:
                return item
        if type(record.get("has_location")) is not bool:
            return item
        if (record["kind"] == "syntax") != (record["code"] > 0):
            return item
        if not record["has_location"]:
            # No coordinate fields are meaningful in an explicitly unlocated record.
            return item
        start, length = record["byte_offset"], record["byte_length"]
        end = start + length
        if start > len(data) or end > len(data):
            return item
        data.decode("utf-8")
        prefix = data[:start].decode("utf-8")
        selected = data[start:end].decode("utf-8")
        byte_prefix = data[:start]
        line = byte_prefix.count(b"\n") + 1
        column = len(byte_prefix.rsplit(b"\n", 1)[-1]) + 1
        if (line, column) != (record["line"], record["column"]):
            return item
        item.update(
            message=record["message"], kind=record["kind"], code=record["code"],
            file=str(source), source_sha256=hashlib.sha256(data).hexdigest(),
            byte_offset=start, byte_length=length, byte_column=column,
            character_offset=len(prefix), character_length=len(selected),
            line=line, column=len(prefix.rsplit("\n", 1)[-1]) + 1,
            coordinate_units="unicode_characters",
        )
    except (ValueError, TypeError, UnicodeError, RecursionError):
        pass
    return item


# Read bounded diagnostic bytes without retaining an unbounded child log in RAM.
def _text(stream):
    """Read bounded diagnostic bytes without retaining an unbounded child log in RAM."""
    stream.seek(0)
    data = stream.read(LOG_LIMIT + 1)
    return data[:LOG_LIMIT].decode("utf-8", errors="replace"), len(data) > LOG_LIMIT


# Wait for one direct child; kill and reap it on timeout or cancellation.
def _run(stage, argv, cwd, timeout, result):
    """Wait for one direct child; kill and reap it on timeout or cancellation."""
    record = {"stage": stage, "argv": list(map(str, argv))}
    result["steps"].append(record)
    with tempfile.TemporaryFile(dir=cwd) as stdout, tempfile.TemporaryFile(
        dir=cwd
    ) as stderr:
        process = subprocess.Popen(
            record["argv"],
            cwd=cwd,
            stdin=subprocess.DEVNULL,
            stdout=stdout,
            stderr=stderr,
            shell=False,
            env=dict(os.environ, TMPDIR=str(cwd), TEMP=str(cwd), TMP=str(cwd)),
        )
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
        raise BuildFailure(
            record["stderr"]
            or record["stdout"]
            or "tool exited with status %s" % record["returncode"]
        )
    return record


# Reject missing/empty stage outputs even if a broken tool reports success.
def _artifact(path, signature=None):
    """Reject missing/empty stage outputs even if a broken tool reports success."""
    if path.is_symlink() or not path.is_file() or not path.stat().st_size:
        raise BuildFailure("tool produced no nonempty artifact")
    if signature:
        with path.open("rb") as stream:
            if stream.read(len(signature)) != signature:
                raise BuildFailure("tool produced an unexpected artifact signature")


# Build one source snapshot; publish only after all selected stages succeed.
def build(
    source,
    output,
    language,
    compiler,
    ir,
    linker,
    module="main",
    timeout=30,
    optimise=True,
    runner=None,
    expect_trace=None,
    structured_diagnostics=False,
):
    """Build one source snapshot; publish only after all selected stages succeed.

    Returned dictionaries belong to the caller. No shell, global parser state or
    implicit sibling paths are used. Direct children are owned until reaped.
    """
    result = {
        "schema": 1,
        "ok": False,
        "published": False,
        "steps": [],
        "diagnostics": [],
    }
    stage = "input"
    try:
        source = Path(source).expanduser().resolve(strict=True)
        raw_output = Path(output).expanduser().absolute()
        if raw_output.is_symlink():
            raise BuildFailure("output must not be a symbolic link")
        output = raw_output.resolve()
        result.update(
            source=str(source), output=str(output), language=language, module=module
        )
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
        tools = [
            Path(p).expanduser().resolve(strict=True) for p in (compiler, ir, linker)
        ]
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
        result["source_sha256"] = hashlib.sha256(data).hexdigest()
        # Stage beside the destination so the final replace stays on one filesystem.
        with tempfile.TemporaryDirectory(
            prefix=".ide-build-", dir=output.parent
        ) as directory:
            work = Path(directory)
            snapshot = work / "input.src"
            snapshot.write_bytes(data)
            original, optimised, obj, movie = [
                work / name
                for name in ("source.fir", "optimised.fir", "module.swo", "movie.swf")
            ]
            stage = "source"
            source_command = [tools[0], language, "--input", snapshot, original, "--module", module]
            if structured_diagnostics:
                source_command.append("--diagnostics-json")
            _run(
                stage,
                source_command,
                work,
                timeout,
                result,
            )
            _artifact(original)
            stage = "verify"
            _run(stage, [tools[1], "check", original], work, timeout, result)
            selected = original
            if optimise:
                stage = "optimise"
                _run(
                    stage, [tools[1], "opt", original, optimised], work, timeout, result
                )
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
            if (
                len(header) != 8
                or int.from_bytes(header[4:8], "little") != movie.stat().st_size
            ):
                raise BuildFailure("SWF declared length does not match the artifact")
            if runner is not None:
                stage = "run"
                record = _run(stage, [runner, movie], work, timeout, result)
                if expect_trace is not None and (
                    record["stdout_truncated"] or record["stdout"] != expect_trace
                ):
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
        if stage == "source" and structured_diagnostics:
            result["diagnostics"].append(_source_diagnostic(str(error), data, source))
        else:
            result["diagnostics"].append(_diagnostic(stage, str(error), source))
    except KeyboardInterrupt:
        result["diagnostics"].append(_diagnostic(stage, "build cancelled", source))
    return result


# Emit exactly one JSON result for a parsed build request; return 0 or 1.
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
    parser.add_argument("--structured-diagnostics", action="store_true",
                        help="Opt in to fir1_source schema-1 byte diagnostics")
    args = vars(parser.parse_args(argv))
    result = build(**args)
    print(json.dumps(result, ensure_ascii=False))
    return 0 if result["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
