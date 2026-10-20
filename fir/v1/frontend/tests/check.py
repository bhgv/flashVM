#!/usr/bin/env python3
"""Structural FIR acceptance is distinct from executable-IR verification/code generation."""
from pathlib import Path
import subprocess
import sys
import tempfile

prefix = "FIR 1.0\nmodule @sample\n"
function = "func @main() -> void {\n^entry:\nreturn\n}\n"
good = [
    prefix + function,
    prefix + 'file !1 "a.as"\nloc !10 !1:12:5\nrequires @limbo.channels version 1\n' +
    'func @increment(%value:any)->any {\n^entry:\n%one:any=const.any number 1\n'
    '%result:any=as.add %value,%one loc !10\nreturn %result\n}\n',
    prefix + 'type @C=ref<@Other>\nglobal @n:i64=0\nimport func @f(%v:i32)->i32\n'
    'import global @external:array<tuple<i32,bool>>\nexport @main as "entry"\n' + function,
    prefix + 'func @sum(%count:i32)->i32 {\n^entry:\n%z:i32=const.i32 0\njump ^loop(%z,%z)\n'
    '^loop(%i:i32,%total:i32):\n%c:bool=i32.lt.signed %i,%count\nif %c,^body,^done(%total)\n'
    '^body:\n%n:i32=i32.add.wrap %total,%i\njump ^loop(%i,%n)\n'
    '^done(%answer:i32):\nreturn %answer\n}\n' + function,
    prefix + 'func @call(%f:func(i32,any)->any,%x:as.string)->any {\n^entry:\n'
    'invoke as.call %f(%x) normal ^ok catch ^failed\n'
    '^ok(%v:any):\nreturn %v\n^failed(%error:any):\nreturn %error\n}\n',
]
# A detached frontend does not depend on sibling executable examples.
examples = Path(__file__).resolve().parents[2] / "examples"
if examples.is_dir():
    good += [p.read_text() for p in examples.rglob("*.fir")]
bad = [
    "", "FIR 2.0\nmodule @m\n", "// comment\n" + prefix + function,
    "\ufeff" + prefix + function, "FIR 1.0", "FIR 1.0 \nmodule @m\n",
    prefix + 'func @x(%v:)->void {^entry:return}',
    prefix + 'func @x()->void {}',
    prefix + 'func @x()->void {^entry:if %c,^yes}',
    prefix + 'func @x()->void {^entry:jump ^yes(,)}',
    prefix + 'func @x()->void {^entry:%v:array<>=copy %x}',
    prefix + 'func @x()->void {^entry:invoke as.call %f() normal ^ok}',
    prefix + 'func @x()->void {^entry:%v:any=const.any number "x"}',
    prefix + 'func @x()->void {^entry:%v:i32=const.i32 1 /* unterminated}',
]
with tempfile.TemporaryDirectory(prefix="fir-syntax-") as directory:
    path = Path(directory) / "input.fir"
    for valid, cases in ((True, good), (False, bad)):
        for source in cases:
            path.write_text(source, encoding="utf-8")
            result = subprocess.run([sys.argv[1], str(path)], capture_output=True,
                                    text=True, timeout=5)
            assert (result.returncode == 0) == valid, (valid, source, result.stderr)
    for source in (prefix + function, good[1], good[3], good[4]):
        for end in range(len(source)):
            path.write_text(source[:end], encoding="utf-8")
            result = subprocess.run([sys.argv[1], str(path)], capture_output=True, timeout=5)
            assert result.returncode in (0, 1), (end, result.stderr)
print("PASS extended FIR syntax, legacy examples, negative and truncated input")
