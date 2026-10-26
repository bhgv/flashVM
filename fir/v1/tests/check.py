#!/usr/bin/env python3
"""Syntax/semantic rejection, round trips, optimiser and emitted SWF checks.
Set FIR_FLASHVM_RUNNER to run every positive case on the real public C API host.
The optional host is separate from these compiler-only checks.
"""
import os
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile

TOOL, SOURCE = map(str, map(Path, sys.argv[1:3]))
ROOT = Path(__file__).resolve().parents[1]
RUNNER = os.environ.get("FIR_FLASHVM_RUNNER")
LINKER = os.environ.get("FIR_SWL")
if not (not RUNNER or LINKER):
    raise AssertionError("FIR_FLASHVM_RUNNER requires FIR_SWL")
checks = 0


def call(*args, good=True):
    global checks
    result = subprocess.run(list(map(str, args)), capture_output=True, timeout=15)
    checks += 1
    if not (result.returncode == 0 if good else result.returncode in (1, 2)):
        raise AssertionError((args, result.returncode, result.stderr.decode(errors="replace")))
    if not (
        b"ERROR: AddressSanitizer" not in result.stderr and b"runtime error:" not in result.stderr
    ):
        raise AssertionError(result.stderr)
    return result


def module(body, extra=""):
    return "FIR 1.0\nmodule @test\n" + extra + "func @main() -> void {\n^entry:\n" + body + "\n}\n"


def movie(path):
    """Independent container/action framing checks, including branch boundaries."""
    data = path.read_bytes()
    if not (data[:4] == b"FWS\x08"):
        raise AssertionError("condition check failed: fir/v1/tests/check.py:44")
    if not (struct.unpack_from("<I", data, 4)[0] == len(data)):
        raise AssertionError("condition check failed: fir/v1/tests/check.py:45")
    nbits = data[8] >> 3
    cursor = 8 + (5 + 4 * nbits + 7) // 8 + 4
    tags = []
    while cursor < len(data):
        tag = struct.unpack_from("<H", data, cursor)[0]
        cursor += 2
        size = tag & 63
        if size == 63:
            size = struct.unpack_from("<I", data, cursor)[0]
            cursor += 4
        body = data[cursor : cursor + size]
        if not (len(body) == size):
            raise AssertionError("condition check failed: fir/v1/tests/check.py:57")
        cursor += size
        tags.append(tag >> 6)
        if tag >> 6 == 12:
            pos, boundaries, targets = 0, set(), []
            while pos < len(body):
                boundaries.add(pos)
                opcode = body[pos]
                pos += 1
                if opcode >= 128:
                    count = struct.unpack_from("<H", body, pos)[0]
                    pos += 2
                    if opcode in (0x99, 0x9D):
                        if not (count == 2):
                            raise AssertionError("condition check failed: fir/v1/tests/check.py:70")
                        targets.append(pos + 2 + struct.unpack_from("<h", body, pos)[0])
                    pos += count
                if not (pos <= len(body)):
                    raise AssertionError("condition check failed: fir/v1/tests/check.py:73")
            if not (body[-1] == 0):
                raise AssertionError("condition check failed: fir/v1/tests/check.py:74")
            if not (set(targets) <= boundaries):
                raise AssertionError("condition check failed: fir/v1/tests/check.py:75")
    if not (cursor == len(data) and tags == [12, 1, 0]):
        raise AssertionError("condition check failed: fir/v1/tests/check.py:76")


with tempfile.TemporaryDirectory() as directory:
    temp = Path(directory)
    inp, opt, swf = (temp / n for n in ("in.fir", "opt.fir", "out.swf"))

    def exercise(text, expected):
        inp.write_text(text, encoding="utf-8")
        call(TOOL, "check", inp)
        call(TOOL, "print", inp, temp / "printed.fir")
        call(TOOL, "check", temp / "printed.fir")
        call(TOOL, "opt", inp, opt)
        call(TOOL, "check", opt)
        for source in (inp, opt):
            call(TOOL, "object", source, temp / "out.swo")
            obj = (temp / "out.swo").read_bytes()
            if not (obj[:8] == b"SWO1\x01\x00\x00\x00"):
                raise AssertionError("condition check failed: fir/v1/tests/check.py:93")
            flags, namesize, codesize, checksum = struct.unpack_from("<IIII", obj, 8)
            if not (flags in (0, 1) and len(obj) == 24 + namesize + codesize):
                raise AssertionError("condition check failed: fir/v1/tests/check.py:95")
            h = 2166136261
            for byte in obj[24:]:
                h = ((h ^ byte) * 16777619) & 0xFFFFFFFF
            if not (h == checksum and obj[-1] == 0):
                raise AssertionError("condition check failed: fir/v1/tests/check.py:99")
            if LINKER:
                call(LINKER, "link", swf, "--extensions", temp / "out.swo")
                movie(swf)
            if RUNNER:
                result = call(RUNNER, swf).stdout.decode("utf-8")
                if not (result == expected):
                    raise AssertionError((source.read_text(), result, expected))

    for folder in sorted((ROOT / "examples").iterdir()):
        # Structural/diagnostic/table examples have separate tests and target contracts.
        # Tables deliberately reject SWO emission; tables.py covers their real C-core execution.
        if not folder.is_dir() or folder.name in ("structural", "diagnostics", "tables"):
            continue
        if folder.name in ("as1", "as2", "as3", "limbo"):
            source = folder / ("main.b" if folder.name == "limbo" else "main.as")
            call(SOURCE, folder.name, "--input", source, inp)
            exercise(inp.read_text(), (folder / "expected.txt").read_text())
            exercise((folder / "expected.fir").read_text(), (folder / "expected.txt").read_text())
        else:
            exercise((folder / "main.fir").read_text(), (folder / "expected.txt").read_text())

    for tool in (TOOL, SOURCE):
        for option in ("--help", "--version"):
            result = call(tool, option)
            if not (result.stdout):
                raise AssertionError("condition check failed: fir/v1/tests/check.py:122")
    # Every source profile lowers mutable names into fresh SSA definitions.
    for language in ("as1", "as2", "as3", "limbo"):
        folder = ROOT / "examples" / language
        program = (folder / ("variables.b" if language == "limbo" else "variables.as")).read_text()
        inp.write_text(program)
        call(SOURCE, language, "--input", inp, opt)
        expected = (folder / "variables.txt").read_text()
        exercise(opt.read_text(), expected)
        exercise((folder / "variables.fir").read_text(), expected)
    for program in (
        "var x = x + 1;",
        "x = 1;",
        "var x = 1; var x = 2;",
        "var x:String = 1;",
        "var x = 1; trace(y);",
        "var x;",
        "var " + "x" * 256 + " = 1;",
        " ".join(f"var x{i} = {i};" for i in range(257)),
    ):
        inp.write_text(program)
        opt.write_bytes(b"keep-output")
        call(SOURCE, "as2", "--input", inp, opt, good=False)
        if not (opt.read_bytes() == b"keep-output"):
            raise AssertionError("condition check failed: fir/v1/tests/check.py:145")
    inp.write_text("var x:Number = 1; trace(x);")
    call(SOURCE, "as1", "--input", inp, opt, good=False)
    inp.write_text("trace(1e2 - 2 * 3);")
    call(SOURCE, "as3", "--input", inp, opt)
    exercise(opt.read_text(), "94\n")

    for language in ("as1", "as2", "as3"):
        inp.write_text((ROOT / "examples" / language / "syntax.as").read_text())
        call(SOURCE, language, "--input", inp, opt)
        exercise(opt.read_text(), (ROOT / "examples" / language / "syntax.txt").read_text())
    inp.write_text(
        "const rate:Number=2, base:Number=0x10; var x=-(base + .5); x *= rate; trace(x);"
    )
    call(SOURCE, "as3", "--input", inp, opt)
    exercise(opt.read_text(), "-33\n")
    inp.write_text(
        "# Native Limbo comment\nimplement V; V: module { value: fn(): int; }; value(): int { x:=2r1010; y:int=16r20; x+=y; x *= -1; return -(x); }"
    )
    call(SOURCE, "limbo", "--input", inp, opt)
    exercise(opt.read_text(), "42\n")
    for expression, expected in [("-16r80000000", "-2147483648\n"), ("36r10", "36\n")]:
        inp.write_text(
            "implement V; V: module { value: fn(): int; }; value(): int { return "
            + expression
            + "; }"
        )
        call(SOURCE, "limbo", "--input", inp, opt)
        exercise(opt.read_text(), expected)
    for language, program in [
        ("as1", "const x=1;"),
        ("as2", "const x=1;"),
        ("as3", "const x=1; x+=2;"),
        ("as3", "const x=1; x=2;"),
        ("as2", "# wrong profile\ntrace(1);"),
        ("as2", "trace(16r20);"),
        ("as2", "var x=1; trace(++x);"),
        ("as2", "trace(--1);"),
        ("as2", "trace(" + "+ " * 100 + "1);"),
    ]:
        inp.write_text(program)
        opt.write_bytes(b"keep")
        call(SOURCE, language, "--input", inp, opt, good=False)
        if not (opt.read_bytes() == b"keep"):
            raise AssertionError("condition check failed: fir/v1/tests/check.py:188")
    for literal in ["1r0", "37r1", "2r2", "16r80000000", "1.0", "0xff"]:
        inp.write_text(
            "implement V; V: module { value: fn(): int; }; value(): int { return " + literal + "; }"
        )
        call(SOURCE, "limbo", "--input", inp, opt, good=False)

    for language in ("as1", "as2", "as3"):
        inp.write_text("var x=12; x/=2; trace(x/2); trace(1 / -0); trace(1 / -(0)); trace(0/0);")
        call(SOURCE, language, "--input", inp, opt)
        if not (
            "source_language " + language in opt.read_text()
            and "source_semantics avm1_numeric" in opt.read_text()
        ):
            raise AssertionError("condition check failed: fir/v1/tests/check.py:198")
        exercise(opt.read_text(), "3\n-Infinity\n-Infinity\nNaN\n")
        if not ("source_language " + language in opt.read_text() and "f64.div" in opt.read_text()):
            raise AssertionError("condition check failed: fir/v1/tests/check.py:203")
    exercise(
        module(
            "%a: i32 = const.i32 +0x2a\n%b: i32 = const.i32 -0x80000000\ntrace %a\ntrace %b\nreturn"
        ),
        "42\n-2147483648\n",
    )
    exercise(
        module(
            "%a: f64 = const.f64 +0x10\n%b: f64 = const.f64 +.5\n%x: f64 = f64.div %a, %b\ntrace %x\nreturn"
        ),
        "32\n",
    )
    for extra in [
        "source_language as2\nsource_semantics limbo_i32\n",
        "source_language limbo\nsource_semantics avm1_numeric\n",
        "source_language as4\nsource_semantics avm1_numeric\n",
        "source_language as3\n",
        "source_semantics avm1_numeric\n",
    ]:
        inp.write_text(module("return", extra))
        call(TOOL, "check", inp, good=False)
    inp.write_text(module("%x: i32 = const.i32 0x80000000\nreturn"))
    call(TOOL, "check", inp, good=False)
    inp.write_text("implement V; V: module { value: fn(): int; }; value(): int { return 4/2; }")
    call(SOURCE, "limbo", "--input", inp, opt, good=False)

    # Full-width integer products catch double precision loss and shift mistakes.
    random.seed(611)
    pairs = [(2147483647, 2147483647), (-2147483648, -1), (-65537, 65539), (0, -1)]
    pairs += [
        (random.randint(-(2**31), 2**31 - 1), random.randint(-(2**31), 2**31 - 1))
        for _ in range(12)
    ]
    for a, b in pairs:
        expected = ((a * b + 2**31) % 2**32) - 2**31
        exercise(
            module(
                f"%a: i32 = const.i32 {a}\n%b: i32 = const.i32 {b}\n%c: i32 = i32.mul.wrap %a, %b\ntrace %c\nreturn"
            ),
            str(expected) + "\n",
        )

    # Dynamic addition, strict doubles, escapes and comment delimiters in strings.
    exercise(
        module(
            '%a: string = const.string "a//b/*c*/"\n%b: string = const.string "!"\n%c: any = as.add %a, %b\ntrace %c\nreturn'
        ),
        "a//b/*c*/!\n",
    )
    exercise(
        module(
            "%a: f64 = const.f64 -0\n%b: f64 = const.f64 2.5\n%c: f64 = f64.add %a, %b\ntrace %c\nreturn"
        ),
        "2.5\n",
    )
    exercise(
        module(
            "%a: i32 = const.i32 1\n%b: i32 = copy %a\n%z: i32 = const.i32 0\n%c: i32 = i32.add.wrap %b, %z\ntrace %c\nreturn"
        ),
        "1\n",
    )
    exercise(
        module(
            '%f: bool = const.bool false\nif %f, ^yes, ^no\n^yes:\nreturn\n^no:\n%v: string = const.string "false arm"\ntrace %v\nreturn'
        ),
        "false arm\n",
    )

    # Prune a known-false arm containing effects and an otherwise live cycle.
    cfg = module(
        '%f: bool = const.bool false\nif %f, ^dead, ^live\n^dead:\n%s: string = const.string "unexecuted"\ntrace %s\njump ^cycle\n^cycle:\njump ^cycle\n^live:\n%n: i32 = const.i32 42\ntrace %n\nreturn'
    )
    exercise(cfg, "42\n")
    rewritten = opt.read_text()
    if not ("^dead" not in rewritten and "^cycle" not in rewritten and ("if " not in rewritten)):
        raise AssertionError("condition check failed: fir/v1/tests/check.py:278")
    # CSE is ordered and local; FP arithmetic is never folded or reassociated.
    exercise(
        module(
            "%a: f64 = const.f64 1.5\n%b: f64 = const.f64 2\n%x: f64 = f64.add %a, %b\n%y: f64 = f64.add %a, %b\ntrace %x\ntrace %y\nreturn"
        ),
        "3.5\n3.5\n",
    )
    if not (opt.read_text().count("f64.add") == 1):
        raise AssertionError("condition check failed: fir/v1/tests/check.py:286")
    exercise(
        module(
            '%a: string = const.string "4"\n%x: f64 = as.to_number %a\n%y: f64 = as.to_number %a\nreturn'
        ),
        "",
    )
    if not (opt.read_text().count("as.to_number") == 2):
        raise AssertionError("condition check failed: fir/v1/tests/check.py:293")
    # No x + 0 identity for strict f64 or dynamic AS addition.
    exercise(
        module(
            "%a: f64 = const.f64 -0\n%b: f64 = const.f64 0\n%x: f64 = f64.add %a, %b\n%y: any = as.add %a, %b\ntrace %x\nreturn"
        ),
        "0\n",
    )
    if not ("f64.add" in opt.read_text() and "as.add" in opt.read_text()):
        raise AssertionError("condition check failed: fir/v1/tests/check.py:301")

    bad = [
        module("trace %missing\nreturn"),
        module("%x: i32 = const.i32 1\n%x: i32 = const.i32 2\nreturn"),
        module("%x: bool = const.i32 1\nreturn"),
        module("%x: i32 = const.i32 2147483648\nreturn"),
        module("%x: i32 = const.i32 1.0\nreturn"),
        module("%x: f64 = const.f64 1e999\nreturn"),
        module("%x: i32 = const.i32 1\nif %x, ^entry, ^entry"),
        module("jump ^missing"),
        module("return\ntrace %x"),
        module("%x: i32 = copy %x\nreturn"),
        module(""),
        module("return\n^dead:\nreturn"),
        module("return\n^entry:\nreturn"),
        module("%x: channel = limbo.chan_create\nreturn"),
        module('%x: string = const.string "bad\\q"\nreturn'),
        module("return") + "trailing",
        module("return").replace("FIR 1.0", "FIR 2.0"),
        "// before version\n" + module("return"),
        module("return").replace("return", "branch"),
        module("return").replace("@main", "@other"),
        module("return") + "/* unterminated",
        module(
            "%x: i32 = const.i32 1\n%b: bool = const.bool true\nif %b, ^left, ^right\n^left:\n%v: i32 = copy %x\njump ^right\n^right:\ntrace %v\nreturn"
        ),
        module("%v: i32 = copy %later\n%later: i32 = const.i32 1\nreturn"),
        module('%s: string = const.string "' + "x" * 256 + '"\nreturn'),
    ]
    for text in bad:
        inp.write_text(text)
        swf.write_bytes(b"preserve-existing-output")
        call(TOOL, "check", inp, good=False)
        call(TOOL, "object", inp, swf, good=False)
        if not (swf.read_bytes() == b"preserve-existing-output"):
            raise AssertionError("condition check failed: fir/v1/tests/check.py:336")
    for data in (b"FIR 1.0\n\x00", b"FIR 1.0\n//\xff", b"FIR 1.0\n//\xed\xa0\x80"):
        inp.write_bytes(data)
        call(TOOL, "check", inp, good=False)
    for text in (
        "trace(name);",
        "trace(1 % 2);",
        "trace(1); trailing",
        "trace(" + "(" * 100 + "1" + ")" * 100 + ");",
        "trace(1); /*",
    ):
        inp.write_text(text)
        call(SOURCE, "as2", "--input", inp, opt, good=False)
    inp.write_text(
        "implement Answer; Answer: module { value: fn(): int; }; value(): int { return 1.0; }"
    )
    call(SOURCE, "limbo", "--input", inp, opt, good=False)
    # Size rejection must not touch an existing destination.
    inp.write_bytes(b"/" * 65537)
    opt.write_bytes(b"keep-output")
    call(SOURCE, "as2", "--input", inp, opt, good=False)
    if not (opt.read_bytes() == b"keep-output"):
        raise AssertionError("condition check failed: fir/v1/tests/check.py:357")
    inp.write_bytes(b"// invalid UTF-8 \xff\ntrace(1);")
    call(SOURCE, "as1", "--input", inp, opt, good=False)
    # Incompatible symbols and capacity overflows fail cleanly.
    inp.write_text(
        module("\n".join(f"%v{i}: i32 = const.i32 {i}" for i in range(4096)) + "\nreturn")
    )
    call(TOOL, "check", inp, good=False)
    # Long branch must be rejected without truncating an existing output.
    inp.write_text(
        module(
            "jump ^far\n^middle:\n"
            + "\n".join(f"%v{i}: i32 = const.i32 {i}" for i in range(1500))
            + "\njump ^far\n^far:\n%f: bool = const.bool false\nif %f, ^middle, ^end\n^end:\nreturn"
        )
    )
    call(TOOL, "check", inp)
    swf.write_bytes(b"preserve-existing-output")
    call(TOOL, "object", inp, swf, good=False)
    if not (swf.read_bytes() == b"preserve-existing-output"):
        raise AssertionError("condition check failed: fir/v1/tests/check.py:376")
print("%d subprocess checks; real FlashVM execution: %s" % (checks, bool(RUNNER)))
