"""Compile source aliases to canonical FIR and real Dis; no legacy compiler is invoked."""
from pathlib import Path
import json
import os
import re
import subprocess
import sys
import tempfile


# Execute explicit tools with bounded completion; failures retain source diagnostics for review.
def call(arguments, success=True):
    result = subprocess.run(list(map(str, arguments)), capture_output=True, text=True, timeout=15)
    if (result.returncode == 0) != success:
        raise RuntimeError(result.stdout + result.stderr)
    return result


# Alias and direct annotations must produce identical FIR; optional runtime checks actual values/dispatch.
def main():
    source_cli, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    runtime = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else None
    dispatch = '--jit' if len(sys.argv) > 4 and sys.argv[4] == 'jit' else '--no-jit'
    cases = [
        ('Chain: type Base; Base: type int;', 'Base: type real;', 'Chain', 'x: Chain = Chain 42; return Chain x;',
         'int', 'x: int = 42; return x;', 42),
        ('', 'Number: type R; R: type real;', 'Number', 'x: Number = Number 85.0; return Number (x / 2.0);',
         'real', 'x: real = 85.0; return x / 2.0;', 42.5),
        ('', 'B: type big; T: type table of big; S: type string;', 'B',
         't: T = table of big {type = B 1}; k: S = S "type"; t.type = B 42; return B t[k];',
         'big', 't: table of big = table of big {type = big 1}; k: string = string "type"; t.type = big 42; return big t[k];', 42),
    ]
    with tempfile.TemporaryDirectory(prefix='Limbo FIR aliases ') as temporary:
        root = Path(temporary)
        source, fir, direct, optimized, image, debug = (root / name for name in
                                                       ('input.b', 'alias.fir', 'direct.fir', 'opt.fir', 'out.dis', 'debug.json'))
        for global_aliases, local_aliases, result, body, primitive, direct_body, expected in cases:
            text = ('implement M; ' + global_aliases + ' M: module {' + local_aliases +
                    ' value: fn(): ' + result + ';}; value(): ' + result + ' {' + body + '}')
            source.write_text(text)
            call([source_cli, 'limbo', '--input', source, fir, '-g', debug])
            for event in json.loads(debug.read_text())['events']:
                if event['byte_offset'] + event['byte_length'] > len(text.encode()):
                    raise RuntimeError('alias debug origin escaped source snapshot')
            source.write_text('implement M; M: module {value: fn(): ' + primitive +
                              ';}; value(): ' + primitive + ' {' + direct_body + '}')
            call([source_cli, 'limbo', '--input', source, direct])
            if fir.read_bytes() != direct.read_bytes():
                raise RuntimeError('aliases changed canonical FIR operations or semantics')
            call([compiler, 'check', fir])
            call([compiler, 'opt', fir, optimized])
            for selected in (fir, optimized):
                if primitive in ('int', 'real') and os.environ.get('FIR_FLASHVM_RUNNER'):
                    if not os.environ.get('FIR_SWL'):
                        raise RuntimeError('SWF execution requires explicit linker')
                    obj, movie = root / 'alias.swo', root / 'alias.swf'
                    call([compiler, 'object', selected, obj])
                    call([os.environ['FIR_SWL'], 'link', movie, obj])
                    observed = call([os.environ['FIR_FLASHVM_RUNNER'], movie])
                    if float(observed.stdout.strip()) != expected:
                        raise RuntimeError('aliased numeric SWF result differs')
                call([compiler, 'dis', selected, image])
                if runtime:
                    for quantum in (1, 2048):
                        real = primitive == 'real'
                        result = call([runtime, dispatch, '--quantum', quantum, '--stats',
                                       '--inspect-reals' if real else '--inspect-words', 0, 1, image])
                        pattern = r'^REAL 0 (.+)$' if real else r'^DATA 0 (-?\d+)$'
                        values = re.findall(pattern, result.stdout, re.M)
                        counters = re.search(r'instructions=(\d+) native=(\d+)', result.stderr)
                        if len(values) != 1 or float(values[0]) != expected or not counters or int(counters[1]) == 0:
                            raise RuntimeError('alias native execution result/counters failed: ' + result.stdout + result.stderr)
                        if (int(counters[2]) > 0) != (dispatch == '--jit'):
                            raise RuntimeError('alias run did not use requested actual dispatch')
        # Both operations of a text-to-int alias cast retain the alias token, not its operand.
        text = ('# π\nimplement M; M: module {I: type int; B: type big; value: fn(): B;};'
                ' value(): B {x := I "42"; return B x;}')
        source.write_text(text)
        call([source_cli, 'limbo', '--input', source, fir, '-g', debug])
        operations = [(event['name'], event['byte_offset'], event['byte_length'])
                      for event in json.loads(debug.read_text())['events'] if event['kind'] == 'table-operation']
        narrow = text.encode().index(b'I "42"')
        widen = text.encode().index(b'B x')
        if operations != [('value.big', narrow, 1), ('value.int', narrow, 1), ('value.big', widen, 1)]:
            raise RuntimeError('alias casts lost exact authored debug spans')

        # Exercise the entire 128-declaration budget without recursive host resolution.
        # A forward chain must emit exactly the same canonical module as no aliases.
        chain = ''.join('A%d: type %s;' % (index, 'int' if index == 127 else 'A%d' % (index + 1))
                        for index in range(128))
        source.write_text('implement M;' + chain +
                          'M: module {value: fn(): A0;}; value(): A0 {return A0 42;}')
        call([source_cli, 'limbo', '--input', source, fir])
        source.write_text('implement M; M: module {value: fn(): int;}; value(): int {return 42;}')
        call([source_cli, 'limbo', '--input', source, direct])
        if fir.read_bytes() != direct.read_bytes():
            raise RuntimeError('full forward chain changed canonical FIR')

        # Runtime cast effects survive optimization; skipped branches stay lazy.
        # All targets are aliases, while int narrowing and text/key parsing stay canonical.
        if runtime:
            prefix = ('implement M; M: module {B: type big; I: type int; S: type string;'
                      ' value: fn(): B;}; value(): B {')
            runtime_cases = [
                ('text: S = S "4294967295"; n: I = I text; return big n + B 43;', True),
                ('t := table of big {["42"] = big 7}; keys := keysof t; return B (I keys[0]);', True),
                ('if (big 0 == big 1) {unused := I "bad";} return B 42;', True),
                ('unused := I "bad"; return B 42;', False),
            ]
            for body, accepted in runtime_cases:
                source.write_text(prefix + body + '}')
                call([source_cli, 'limbo', '--input', source, fir, '-g', debug])
                call([compiler, 'opt', fir, optimized])
                for selected in (fir, optimized):
                    call([compiler, 'dis', selected, image])
                    observed = call([runtime, dispatch, '--quantum', 1, '--inspect-words', 0, 2, image], accepted)
                    if accepted and 'DATA 0 42\nDATA 8 1\n' not in observed.stdout:
                        raise RuntimeError('aliased checked cast changed runtime value')
                    if not accepted and 'invalid big conversion' not in observed.stderr:
                        raise RuntimeError('optimizer erased or changed an unused failing alias cast')

        for language in ('as1', 'as2', 'as3'):
            source.write_text('var type = 7; trace(type + 1);')
            call([source_cli, language, '--input', source, fir])
            call([compiler, 'check', fir])
        rejected = [
            'implement M; {value: fn(): int;}; value(): int {return 1;}',
            'implement ;',
            'implement M; M: type int; M: module {value: fn(): int;}; value(): int {return 1;}',
            'implement M; A: type B; B: type A; M: module {value: fn(): int;}; value(): int {return 1;}',
            'implement M; Missing: type Nope; M: module {value: fn(): int;}; value(): int {return 1;}',
            'implement M; M: module {R: type real; value: fn(): int;}; value(): int {return R 42;}',
            'implement M; M: module {T: type table of big; value: fn(): big;}; value(): big {t := table of big; return T t;}',
            'implement M; A: type A; M: module {value: fn(): int;}; value(): int {return 1;}',
            'implement M; A: type int; A: type real; M: module {value: fn(): int;}; value(): int {return 1;}',
            'implement M; M: module {A: type string; value: fn(): A;}; value(): A {return 1;}',
            'implement M; M: module {A: type int; value: fn(): A;}; value(): real {return 1.0;}',
            'implement M; M: module {A: type int; value: fn(): A;}; value(): A {A := 1; return A;}',
        ]
        # File edges cannot bind into the implementing module; unused cycles/misses also fail.
        rejected.extend([
            'implement M; A: type B; M: module {B: type int; value: fn(): int;}; value(): int {return 1;}',
            'implement M; A: type B; B: type C; C: type A; M: module {value: fn(): int;}; value(): int {return 1;}',
            'implement M; A: type int; M: module {value: fn(): A;}; value(): A {return ' + 'A ' * 65 + '1;}',
        ])
        rejected.append('implement M;' + ''.join('A%d: type A%d;' % (index, (index + 1) % 128)
                                                 for index in range(128)) +
                        'M: module {value: fn(): int;}; value(): int {return 1;}')
        rejected.append('implement M; ' + ''.join('A%d: type int;' % i for i in range(129)) +
                        'M: module {value: fn(): int;}; value(): int {return 1;}')
        for text in rejected:
            source.write_text(text)
            fir.write_text('old FIR')
            debug.write_text('old debug')
            result = call([source_cli, 'limbo', '--input', source, fir, '-g', debug, '--diagnostics-json'], False)
            if json.loads(result.stderr)['kind'] not in ('semantic', 'syntax') or fir.read_text() != 'old FIR' or debug.read_text() != 'old debug':
                raise RuntimeError('alias rejection lost diagnostics or staged-output retention')
        # Diagnostics use the authored unresolved RHS, including UTF-8 prefix bytes.
        text = '# π\nimplement M; Broken: type Missing; M: module {value: fn(): int;}; value(): int {return 1;}'
        source.write_text(text)
        result = call([source_cli, 'limbo', '--input', source, fir, '--diagnostics-json'], False)
        diagnostic = json.loads(result.stderr)
        offset = text.encode().index(b'Missing')
        if diagnostic['byte_offset'] != offset or diagnostic['byte_length'] != len('Missing'):
            raise RuntimeError('forward alias diagnostic lost exact RHS token location')

    print('PASS source aliases: forward binding, canonical casts, int/real/big/table FIR, scope, bounds and staged diagnostics')


if __name__ == '__main__':
    main()
