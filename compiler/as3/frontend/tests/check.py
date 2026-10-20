"""AS3 syntax acceptance is separate from FIR/backend semantics."""
from pathlib import Path
import subprocess
import sys
import tempfile
exe = sys.argv[1]
good = [
    '', 'package { public class Main { public function Main() {} } }',
    'package demo.core { import flash.display.*; public class C extends Sprite implements I { private var n:Number = 1; public function f(x:int):Number { return x + n; } } }',
    'public interface I extends J { function f(x:int):Number; }',
    'var x:* = null; const y:Number = 2; x = y as Number; trace(x is Number);',
    'class C { override public function f():void {} protected var x:int; }',
    'if (true) { trace(1); } else trace(2);',
    'for (var i:int = 0; i < 10; i++) trace(i);',
    'try { throw 1; } catch(e:Error) { trace(e); } finally {}',
    'package p { public function f(x:Number=1,...args):void {} public const N:int=1; }',
    'class C { public function get value():Number { return 1; } public function set value(x:Number):void {} }',
    'namespace custom = "urn:custom"; use namespace custom; custom::f(); obj.custom::value = 1;',
    'class C { public namespace custom; custom function f():void {} use namespace custom; }',
    'for each (var item:* in items) trace(item);',
    'var each:Number = 1; each++;',
    'try {} catch(e:TypeError) {} catch(e:Error) {} finally {}',
    'function f(a:Number=1,b:Number=2,...rest:Array):void {}',
    '// EOF comment' , '/* stars **/ var x:Number = 1;',
]
good += [p.read_text() for p in sorted((Path(__file__).resolve().parents[2] / 'examples').glob('*.as'))]
good += ['function many('+','.join('p%d:Number'%i for i in range(2000))+'):void {}']
bad = ['function f(...rest,x:Number):void {}', 'function f(a:Number=1,b:Number):void {}',
       'function f(a:Number,):void {}', 'function f(...args=1):void {}',
       'for each(var x=0;x<3;x++) f();', 'class * {}', 'class void {}',
       'var x:void;', 'function f(x:void):void {}', 'package * {}',
       'class C { function first second():void {} }', 'class C { unknown(); }',
       'namespace = "urn:x";', 'use namespace ;', 'ns::;',
'package x { class C {', 'package { import ; }',
       'class C extends A, B {}', 'var x: = 1;', 'const = 2;',
       'interface I { function f() {} }', 'var x = <xml/>;',
       'var x:Vector.<int>;', '[Event(name="x")] class C {}',
        '/* missing', 'var x = 1; @',
       'package x { class C {} } trailing nonsense', 'var x = ;', 'trace(1 add 2);', 'trace(1 eq 2);']
with tempfile.TemporaryDirectory() as tmp:
    path = Path(tmp) / 'input.as'
    for accepted, cases in [(True, good), (False, bad)]:
        for source in cases:
            path.write_text(source)
            run = subprocess.run([exe, str(path)], capture_output=True, timeout=10)
            assert run.returncode == (0 if accepted else 1), (source, run.returncode, run.stderr)
    for data in [b'// \xff', b'var x = 1;\0']:
        path.write_bytes(data)
        assert subprocess.run([exe, str(path)], capture_output=True).returncode == 1
print(f'{len(good)+len(bad)+2} syntax cases; no semantic/backend claim')

# Help/version are successful commands, never treated as input filenames.
for option in ('--help', '--version'):
    result = subprocess.run([sys.argv[1], option], capture_output=True, timeout=5)
    assert result.returncode == 0 and result.stdout
    assert b'ERROR: AddressSanitizer' not in result.stderr and b'runtime error:' not in result.stderr
