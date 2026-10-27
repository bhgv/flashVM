"""Check compound big/table updates, single capture, error order and Dis execution."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Interpret a mathematical integer as the signed modulo-2^64 big representation.
# value is borrowed numeric input; the result owns no storage and cannot throw.
def signed(value):
    value &= (1 << 64) - 1
    return value if value < (1 << 63) else value - (1 << 64)


# Compute truncating signed division with an exact Python oracle, including MIN/-1.
def quotient(left, right):
    magnitude = abs(left) // abs(right)
    return signed(-magnitude if (left < 0) != (right < 0) else magnitude)


# Explicit CLI paths select compiler/reference/runtime. Temporary artifacts and
# subprocesses are private; errors include full diagnostics and never retry writes.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='compound table updates ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image, debug = (work / name for name in
                                               ('source.b', 'source.fir', 'opt.fir', 'source.dis', 'source.json'))

        # Inputs determine a source module; expected is independent of the emitted
        # instruction stream. Check original/optimized outputs on every available path.
        def execute(body, expected):
            text = ('implement M; M: module {value: fn(): big;}; value(): big {\n'
                    '# UTF-8 origin: составные присваивания.\n' + body + '\n}')
            source.write_text(text)
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work)
            raw = text.encode('utf-8')
            compounds = {b'+=', b'-=', b'*=', b'/=', b'%=', b'&=', b'|=', b'^=', b'<<=', b'>>='}
            seen = set()
            for event in json.loads(debug.read_text())['events']:
                if event['kind'] == 'table-operation' and event['name'].startswith('big.'):
                    start = event['byte_offset']
                    spelling = raw[start:start + event['byte_length']]
                    if spelling in compounds:
                        seen.add((start, event['byte_length']))
            required = {(match.start(), match.end() - match.start())
                        for match in re.finditer(rb'<<=|>>=|[+*/%&|^\-]=', raw)}
            if not required.issubset(seen):
                raise RuntimeError('compound arithmetic lost its exact operator byte span')
            call([compiler, 'check', fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference and call([reference, selected], work).stdout.strip() != str(expected):
                    raise RuntimeError('compound reference mismatch: ' + body)
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    traces = len(re.findall(r'^\s*trace ', selected.read_text(), re.MULTILINE))
                    result = call([runtime, mode, '--quantum', 1, '--stats', '--inspect-words',
                                   0, traces * 2, image], work)
                    values = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.MULTILINE)]
                    flags = values[traces:]
                    if (len(values) != traces * 2 or any(flag not in (0, 1) for flag in flags) or
                            [value for value, flag in zip(values[:traces], flags) if flag] != [expected] or
                            any(value for value, flag in zip(values[:traces], flags) if not flag)):
                        raise RuntimeError('actual Dis compound capture/flags mismatch: ' + body)
                    counters = re.search(r'instructions=(\d+) native=(\d+)', result.stderr)
                    if (not counters or int(counters[1]) < 1 or
                            (int(counters[2]) > 0) != (mode == '--jit')):
                        raise RuntimeError('compound module used the wrong dispatch mode')
                    if not re.search(r'tables_live=0 keys_live=0', result.stderr):
                        raise RuntimeError('compound expression retained table/key owners')

        operators = {
            '+=': lambda a, b: signed(a + b),
            '-=': lambda a, b: signed(a - b),
            '*=': lambda a, b: signed(a * b),
            '/=': quotient,
            '%=': lambda a, b: signed(a - quotient(a, b) * b),
            '&=': lambda a, b: signed(a & b),
            '|=': lambda a, b: signed(a | b),
            '^=': lambda a, b: signed(a ^ b),
            '<<=': lambda a, b: signed(a << b),
            '>>=': lambda a, b: signed(a >> b),
        }
        for spelling, operation in operators.items():
            left, right = (-9, 3)
            rhs = ('int ' if spelling in ('<<=', '>>=') else 'big ') + str(right)
            expected = operation(left, right)
            execute('x := big -9; x ' + spelling + ' ' + rhs + '; return x;', expected)
            for target, setup in (('t[0]', ''), ('t.answer', ''), ('t[ks[0]]', 'ks := keysof t;')):
                initial = 't.answer' if target == 't[ks[0]]' else target
                execute('t := table of big; ' + initial + ' = big -9; ' + setup +
                        target + ' ' + spelling + ' ' + rhs + '; return ' + target + ';', expected)
        for body, expected in (
                ('x := big 9223372036854775807; x += 1; return x;', -(1 << 63)),
                ('x := big -9223372036854775808; x -= 1; return x;', (1 << 63) - 1),
                ('x := big -9223372036854775808; x /= -1; return x;', -(1 << 63)),
                ('x := big -9223372036854775808; x %= -1; return x;', 0),
                ('x := big -1; x >>= int 63; return x;', -1),
                ('x := big 1; x <<= int 63; return x;', -(1 << 63)),
                ('t := table of big; t[0] = big 40; t.marker = big 1; '
                 't[big (delete t.marker) - 1] += 2; return t[0];', 42),
                ('t := table of big; t.answer = big 40; '
                 't.answer += big (delete t.answer) + 1; return t.answer;', 42),
                ('t := table of big; t.answer = big 40; '
                 't.answer += big ((1 < 2) || (delete t.answer)) + 1; return t.answer;', 42),
                ('t := table of big; t.answer = big 40; '
                 't.answer += len table of big + 2; return t.answer;', 42),
                ('t := table of big; t.answer = big 40; alias := t; alias.answer += 2; return t.answer;', 42),
                ('x := big 40; if(x < 41){x += 2; return x;}else{x -= 1; return x;}', 42),
        ):
            execute(body, expected)

        # Missing old values fail before RHS exceptions; invalid shifts and zero
        # divisors do not publish a destination write. Both IR variants retain errors.
        for body, diagnostic in (
                ('t := table of big; t.missing += 1 / 0; return 42;', 'missing key'),
                ('t := table of big; t.x = big 42; ks := keysof t; delete t.x; '
                 't[ks[0]] += 1 / 0; return 42;', 'missing key'),
                ('t := table of big; t.x = big 42; t.x /= 0; return 42;', 'zero divide'),
                ('x := big 42; x <<= int 64; return x;', None),
        ):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            call([source_compiler, 'limbo', '--input', source, fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference:
                    failure = call([reference, selected], work, False)
                    if diagnostic and diagnostic not in failure.stderr:
                        raise RuntimeError('compound reference error priority changed')
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    failure = call([runtime, mode, image], work, False)
                    if diagnostic and diagnostic not in failure.stderr:
                        raise RuntimeError('compound native error priority changed: ' + failure.stderr)

        for body in ('x: big; x += 1; return x;', 'x := "40"; x += 2; return 42;',
                     'x := int 40; x += 2; return big x;', 'x := 40; x <<= 1; return x;',
                     't := table of big; t.x = big 40; t.x += "2"; return 42;',
                     't := table of big; t.x = big 40; t.x <<= int 1.5; return 42;'):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            fir.write_text('PRIOR FIR')
            debug.write_text('PRIOR DEBUG')
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
            if fir.read_text() != 'PRIOR FIR' or debug.read_text() != 'PRIOR DEBUG':
                raise RuntimeError('compound type rejection replaced caller output')
    print('PASS compound big/table updates: ten operators, captures, aliases, lazy RHS, signed boundaries and errors' +
          (' with actual Dis ' + mode if runtime else ''))


if __name__ == '__main__':
    main()
