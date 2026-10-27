"""Check hosted exception recovery, native frame unwinding and terminal budgets."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile


# Run explicit matching native tools; local fixtures own every product. Fail with
# complete VM diagnostics, and bound loop cases without depending on a UI session.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    with tempfile.TemporaryDirectory(prefix='hosted exception boundary ') as directory:
        work = Path(directory)
        source = work / 'main.b'
        image = work / 'main.dis'
        cases = (
            ('prefix', '''implement E; include "sys.m";
E: module {init: fn(); child: fn();};
child() {raise "remote: timeout";}
init() {
    s := load Sys Sys->PATH;
    caught := 0;
    {child();} exception message {
    "remote:*" => caught = 1;
    * => raise message;
    }
    if (!caught) {raise "fail: exception prefix";}
    s->print("PASS prefix and nested frame recovery\\n");
}
''', 0, 'PASS prefix and nested frame recovery', None),
            ('unhandled', '''implement E; E: module {init: fn();};
init() {{raise "unhandled probe";} exception message {
"different" => raise "fail: wrong handler";
}}
''', 1, 'unhandled probe', None),
            ('budget', '''implement E; include "sys.m";
E: module {init: fn();};
init() {
    s := load Sys Sys->PATH;
    x := 0;
    {for (;;) {x++;}} exception message {
    * => s->print("MUST_NOT_CATCH\\n");
    }
}
''', 1, 'instruction limit exceeded', 80),
        )
        for name, text, expected_code, expected_text, limit in cases:
            source.write_text(text)
            subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(source)],
                           capture_output=True, text=True, check=True)
            for mode in ('--jit', '--no-jit'):
                arguments = [str(runtime), mode, '--quantum', '1', '--stats']
                if limit:
                    arguments += ['--max-instructions', str(limit)]
                result = subprocess.run(arguments + [str(image)], capture_output=True, text=True,
                                        timeout=15, env=dict(os.environ, TMPDIR=str(work)))
                output = result.stdout + result.stderr
                if result.returncode != expected_code or expected_text not in output or 'MUST_NOT_CATCH' in output:
                    raise RuntimeError(name + ' ' + mode + '\n' + output)
                if limit and 'instructions=80 ' not in output:
                    raise RuntimeError('handler bypassed instruction accounting: ' + output)
    print('PASS hosted exception prefixes, nested frames, unhandled errors and terminal budgets')


if __name__ == '__main__':
    main()
