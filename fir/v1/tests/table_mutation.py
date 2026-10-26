"""Check count, membership and deleting table operations from Coco/R to real Dis."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


# Execute explicit tools in a private cwd; preserve complete diagnostics on mismatch.
# args are borrowed paths/flags, work owns all outputs; failures raise without retries.
def call(args, work, success=True):
    result = subprocess.run(list(map(str, args)), cwd=work, capture_output=True, text=True,
                            env=dict(os.environ, TMPDIR=str(work)), timeout=40)
    if (result.returncode == 0) != success:
        raise RuntimeError(str(args) + '\n' + result.stdout + result.stderr)
    return result


# Verify original/optimized FIR, exact observable errors and optional native execution.
# CLI arguments select compiler/reference/runtime binaries; no bootstrap parser is used here.
def main():
    source_compiler, compiler = (Path(x).resolve() for x in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='table mutation ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image, debug = (work / name for name in ('input.b', 'input.fir', 'opt.fir', 'out.dis', 'source.json'))

        # Each source has one return unless stated; captures are values then executed flags.
        def execute(body, expected, traces=1):
            source.write_text('implement M; M: module { value: fn(): big; }; value(): big {\n' + body + '\n}\n')
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work)
            symbols = json.loads(debug.read_text())
            spelling = {'table.has': b'in', 'table.delete': b'delete', 'table.count': b'countof'}
            raw = symbols['source'].encode('utf-8')
            for event in symbols['events']:
                if event.get('name') in spelling:
                    start = event['byte_offset']
                    if raw[start:start + event['byte_length']] != spelling[event['name']]:
                        raise RuntimeError('new operator lost its exact source/debug span')
            call([compiler, 'check', fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference and call([reference, selected], work).stdout.strip() != str(expected):
                    raise RuntimeError('reference result mismatch for ' + body)
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    output = call([runtime, mode, '--quantum', '1', '--inspect-words', '0', traces * 2, image], work).stdout
                    values = [expected] + [0] * (traces - 1) + [1] + [0] * (traces - 1)
                    wanted = ''.join('DATA ' + str(i * 8) + ' ' + str(v) + '\n' for i, v in enumerate(values))
                    if wanted not in output:
                        raise RuntimeError('native capture/flags mismatch\n' + output + '\n' + body)

        execute('t := table of big; return countof t;', 0)
        execute('t := table of big; t[0] = big 0; t[5] = big 9; t["0"] = big 7; '
                'return countof t * 10 + len t;', 31)
        execute('t := table of big; t[0] = big 0; t["0"] = big 4; '
                'return big (0 in t) * 100 + big ("0" in t) * 10 + big (1 in t);', 110)
        execute('t := table of big; t[0] = big 7; t[1] = big 8; t.answer = big 42; '
                'before := countof t; keys := keysof t; present := "answer" in t; '
                'removed := delete t[0]; absent := delete t.missing; oldkey := keys[0] in t; '
                'if (present) { return t.answer + (before - 3) + (countof t - 2) + len t + '
                '(len keys - 3) + (big removed - 1) + big absent + big oldkey; } else { return 0; }', 42, 2)
        execute('t := table of big; t[0] = big 10; t[1] = big 20; alias := t; '
                'delete alias[0]; t[0] = big 7; return countof t * 10 + len t + t[0];', 29)
        execute('t := table of big; t.first = big 42; keys := keysof t; '
                'deleted := delete t[keys[0]]; t.first = big 19; '
                'return big deleted * 100 + t[keys[0]] + countof t + len keys;', 121)
        execute('t := table of big; t.a = big 1; keys := keysof t; delete t.a; '
                'deleted_again := delete t[keys[0]]; return big deleted_again + big (keys[0] in t);', 0)
        execute('t := table of big; t.a = big 1; b := "a" in t; '
                't[typeof b] = big 40; return t["int"] + big (int b) + big (int (delete t.a));', 42)
        execute('t := table of big; t[1] = big 1; return big (1 | 0 in t);', 1)
        execute('t := table of big; t["ключ"] = big 41; exists := "ключ" in t; '
                'return t["ключ"] + big exists;', 42)
        execute('t := table of big; t.x = big 1; t[string ("x" in t)] = big 41; '
                'return t["1"] + big (int ("x" in t));', 42)


        # Memory effects must not be erased when no scalar result is used.
        source.write_text('implement M; M: module { value: fn(): big; }; value(): big {'
                          ' t := table of big; t.x = big 9; delete t.x; return countof t; }')
        call([source_compiler, 'limbo', '--input', source, fir], work)
        call([compiler, 'opt', fir, optimized], work)
        if 'table.delete' not in optimized.read_text() or 'table.count' not in optimized.read_text():
            raise RuntimeError('optimizer removed an observable memory operation')

        # Numeric key construction is eager, including an unused membership/delete.
        for body in ('x := -1 in t; return 42;', 'delete t[-1]; return 42;',
                     'keys := keysof t; x := keys[0] in t; return 42;'):
            source.write_text('implement M; M: module { value: fn(): big; }; value(): big { t := table of big; ' + body + ' }')
            call([source_compiler, 'limbo', '--input', source, fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference:
                    call([reference, selected], work, False)
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    call([runtime, mode, image], work, False)

        # Wrong static operands cannot publish a partially replaced output file.
        for body in ('t := table of big; return countof (keysof t);',
                     't := table of big; return big (t in 1);',
                     't := table of big; x: int = int 0; return big (x in t);',
                     't := table of big; delete t; return 0;',
                     't := table of big; if (countof t) { return 1; } else { return 0; }'):
            source.write_text('implement M; M: module { value: fn(): big; }; value(): big { ' + body + ' }')
            fir.write_text('UNCHANGED')
            call([source_compiler, 'limbo', '--input', source, fir, '--diagnostics-json'], work, False)
            if fir.read_text() != 'UNCHANGED':
                raise RuntimeError('source failure replaced caller output')

        # Bool-to-big also belongs to the pointer-free scalar backend, without a module ABI.
        fir.write_text('FIR 1.0\nmodule @logical\nfunc @main() -> void {\n^entry:\n'
                       ' %b: bool = const.bool true\n %n: i64 = value.big %b\n trace %n\n return\n}\n')
        call([compiler, 'opt', fir, optimized], work)
        for selected in (fir, optimized):
            if reference and call([reference, selected], work).stdout.strip() != '1':
                raise RuntimeError('scalar bool conversion reference mismatch')
            call([compiler, 'dis', selected, image], work)
            if runtime and 'DATA 0 1\n' not in call([runtime, mode, '--inspect-words', '0', '1', image], work).stdout:
                raise RuntimeError('scalar bool conversion native mismatch')

        # Public textual FIR enforces location/table types, not just source grammar.
        for operation in ('%r: i64 = table.count %x', '%r: bool = table.has %x', '%r: bool = table.delete %x'):
            fir.write_text('FIR 1.0\nmodule @bad\nfunc @main() -> void {\n^entry:\n %x: i64 = const.i64 1\n ' + operation + '\n return\n}\n')
            rejected = call([compiler, 'check', fir], work, False)
            if 'operand/result type mismatch' not in rejected.stderr:
                raise RuntimeError('malformed FIR fixture failed before static type validation: ' + rejected.stderr)
    print('PASS table count/in/delete: grammar, types, effects, aliases, snapshots, exact values and errors' + (' with native ' + mode if runtime else ''))


if __name__ == '__main__':
    main()
