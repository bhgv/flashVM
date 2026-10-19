#!/usr/bin/env python3
"""Check complete-input acceptance and rejection in separate parser processes."""
import subprocess
import sys
import tempfile
from pathlib import Path

CASES = [('hello',
  'implement Command; include "sys.m"; sys: Sys; Command: module { init: fn(nil: ref '
  'Draw->Context, argv: list of string); }; init(nil: ref Draw->Context, argv: list of '
  'string) { sys = load Sys Sys->PATH; sys->print("привет 中文\\n"); }',
  True),
 ('types',
  'implement Test; Point: adt { x,y: int; next: cyclic ref Point; }; T: type (int,string); '
  'x: array of int; n: con 16rff;',
  True),
 ('control',
  'implement T; f(x:int):int { for(i:=0;i<10;i++) { if(i==x) return i; } return -1; }',
  True),
 ('case',
  'implement T; f(x:int) { case x { 0 => x++; 1 or 2 to 4 => x--; * => return; } }',
  True),
 ('alt', 'implement T; f(c:chan of int) { alt { x := <-c => x++; * => return; } }', True),
 ('array',
  'implement T; f() { a := array[3] of {0,1,2}; x := a[1:]; c := chan of int; }',
  True),
 ('tuple', 'implement T; f() { (a,b) := (1,2); }', True),
 ('comments', 'implement T; n: con 2; # eof', True),
 ('bad-expression', 'implement T; f() { x := 1 + ; }', False),
 ('bad-type', 'implement T; x: array int;', False),
 ('unclosed', 'implement T; f() {', False),
 ('trailing', 'implement T; @', False),
 ('unicode-char', "implement T; x: con '中'; y: con '\\u0041';", True),
 ('bad-char', "implement T; x: con 'ab';", False),
 ('send', 'implement T; f(c:chan of int) { c <- = 3; }', True),
 ('pick',
  'implement T; f(x: ref T) { pick p := x { A or B => return; * => return; } }',
  True),
 ('adt-pick', 'implement T; T: adt { pick { A or B => x:int; C => y:string; } };', True),
 ('case-string',
  'implement T; f(x:string) { case x { "=>" => x="# =>"; * => return; } }',
  True),
 ('utf8-bom', '\ufeffimplement T; x: con "中";', True)]


def main():
    """Bound each subprocess so recovery bugs cannot hang the test suite."""
    with tempfile.TemporaryDirectory() as directory:
        for name, source, valid in CASES:
            path = Path(directory) / (name + '.src')
            path.write_text(source, encoding='utf-8')
            result = subprocess.run([sys.argv[1], str(path)], capture_output=True,
                                    text=True, timeout=5)
            if result.returncode != (0 if valid else 1):
                raise RuntimeError(name + ': ' + str(result.returncode) + '\n' + result.stderr)
        # The driver rejects malformed encodings and NUL rather than truncating.
        for data in (b'\xff', b'\xe0\x80\x80', b'\xed\xa0\x80', b'\xf4\x90\x80\x80',
                     b'\xc2', b'\x00'):
            path = Path(directory) / 'invalid-encoding.src'
            path.write_bytes(data)
            result = subprocess.run([sys.argv[1], str(path)], capture_output=True,
                                    text=True, timeout=5)
            if result.returncode != 1 or 'UTF-8' not in result.stderr:
                raise RuntimeError('Encoding validation failed: ' + repr(data))
        missing = subprocess.run([sys.argv[1], str(Path(directory) / 'missing')],
                                 capture_output=True, timeout=5)
        if missing.returncode != 2:
            raise RuntimeError('Missing source must report an I/O failure')
    print(str(len(CASES)) + ' syntax cases passed')


if __name__ == '__main__':
    main()

# Help/version are successful commands, never treated as input filenames.
for option in ('--help', '--version'):
    result = subprocess.run([sys.argv[1], option], capture_output=True, timeout=5)
    assert result.returncode == 0 and result.stdout
    assert b'ERROR: AddressSanitizer' not in result.stderr and b'runtime error:' not in result.stderr
