#!/usr/bin/env python3
"""Check complete-input acceptance and rejection in separate parser processes."""
import subprocess
import sys
import tempfile
from pathlib import Path

CASES = [
 ('instantiated-casts', 'implement L; f() { x := Box[int] value; y := m->Box[array of byte] value; }', True),
 ('bad-instantiated-cast', 'implement L; f() { x := Box[1+2] value; }', False),

 ('export-conversions', 'implement L; f() { x := a[int value + 1]; y := a[list of {1,2}]; z := g[int, array of byte]; }', True),
 ('export-bad-conversion', 'implement L; f() { x := a[int !]; }', False),

 ('named-cast', 'implement L; f() { x := Point value; y := m->Point value; z := m->Point other->Point value; }', True),
 ('cast-precedence', 'implement L; f() { x := Point value + other; y := a * b + c; z := m->f(value); }', True),
 ('cast-missing-operand', 'implement L; f() { x := m->Point !; }', False),

 ('labels', 'implement L; f() { outer: for(;;) { inner: while() { continue outer; } break outer; } select: case x { 1 => break select; } ready: alt { <-c => break ready; } choice: pick v := p { A => break choice; } }', True),
 ('label-comments','implement L; f() { outer # comment\n : # comment\n while() break outer; x: int = 1; }',True),
 ('list-literal','implement L; f() { xs := list of {1,2,3}; v := hd xs; }',True),
 ('array-comma','implement L; f() { a := array[] of {1,2,3,}; b := array[8] of {0 => 1,* => 0,}; }',True),
 ('array-comment-comma','implement L; f() { a := array[] of {1, # tail\n}; }',True),
 ('deref-slices','implement L; f() { x := *p; y := a[1:]; z := a[0:2]; a[1:] = b; }',True),
 ('bad-slice-empty','implement L; f() { x := a[]; }',False),
 ('slice-lower-omitted','implement L; f() { x := a[:2]; }',True),
 ('bad-array-empty','implement L; f() { x := array[] of {}; }',False),
 ('bad-array-size','implement L; f() { x := array[] of int; }',False),
 ('bad-list-empty','implement L; f() { x := list of {}; }',False),
 ('bad-label-return','implement L; f() { invalid: return; }',False),
 ('bad-array-double-comma','implement L; f() { x := array[] of {1,,}; }',False),
('hello',
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

CASES += [
 ('exceptions','implement A,B; E: exception; E2: exception(int,string); f() raises(E,E2) { { raise E; } exception e { E => raise; * => exit; } }',True),
 ('polymorphic','Pair: adt[T] { x:T; }; f[T](x:T):T { return x; }',True),
 ('constraints','f[T](x:T):T for { T => show:fn(x:T); } raises(E) { return x; }',True),
 ('extended-expressions','f(){ x:=a[:2]; y:=a[:]; z:=2**3**4; z**=2; a:=array of byte "hello"; xs:=list of {1,2,}; g[int,array of int](1); }',True),
 ('fixed','T:type fixed(16,8); f(x:fixed(16)){ y:=fixed(16,8) x; }',True),
 ('bad-poly-empty','f[](x:int){}',False),
 ('bad-poly-type','Pair:adt[T]{x:T;}; p:Pair[];',False),
 ('bad-exception','f(){ { x(); } exception e { "x" return; } }',False),
 ('bad-raise','f(){raise 1+;}',False),
 ('bad-power','f(){x:=2**;}',False),
 ('bad-varargs','f(*,x:int){}',False),
 ('bad-for-constraint','f[T](x:T) for {} {}',False),
 ('bad-empty-case','f(){case x {}}',False),
 ('bad-fixed','x:fixed();',False),
]


CASES += [('syntax-example', (Path(__file__).resolve().parents[2]/'examples/Syntax.b').read_text(), True)]

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
