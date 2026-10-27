"""Verify ordered hybrid table construction through Coco/R, FIR and real Dis."""
import json
from pathlib import Path
import re
import random
import sys
import tempfile

from table_mutation import call


# Explicit tools/optional runtime are argv. All files and subprocesses are private;
# original and optimized modules must agree with source-level integer oracles.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='hybrid constructors ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image = (work / name for name in
                                        ('source.b', 'source.fir', 'opt.fir', 'source.dis'))

        # body is a big/table source fragment; expected is an independent scalar
        # oracle. Every output is private and checked before/after optimization.
        def execute(body, expected):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                              '# Позиции Unicode перед полями.\n' + body + '}')
            debug = work / 'source.json'
            try:
                call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work)
            except RuntimeError as failure:
                raise RuntimeError(body + '\n' + str(failure)) from failure
            call([compiler, 'check', fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference and call([reference, selected], work).stdout.strip() != str(expected):
                    raise RuntimeError('constructor reference mismatch: ' + body)
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
                        raise RuntimeError('actual constructor Dis capture/flags mismatch: ' + body)
                    counters = re.search(r'instructions=(\d+) native=(\d+)', result.stderr)
                    if (not counters or int(counters[1]) < 1 or
                            (int(counters[2]) > 0) != (mode == '--jit')):
                        raise RuntimeError('constructor module used wrong actual dispatch mode')
                    if not re.search(r'tables_live=0 keys_live=0', result.stderr):
                        raise RuntimeError('constructor retained table or snapshot owners')

        cases = (
            ('t := table of big {}; return len t + countof t;', 0),
            ('t := table of big {42}; return t[0];', 42),
            ('return table of big {42}[0];', 42),
            ('t := table of big {len (table of big {1, 2}), countof (table of big {a = 40}) + 39}; return t[0] + t[1];', 42),
            ('t := table of big {40, 2,}; return t[0] + t[1];', 42),
            ('t := table of big {a = 40, b = 2}; return t.a + t.b;', 42),
            ('t := table of big {["Ж😀"] = 42,}; return t["Ж😀"];', 42),
            ('t := table of big {40, named = 5, 2, ["x"] = 7}; '
             'return t[0] + t[1] + len t + countof t - 6;', 42),
            ('t := table of big {[0] = 99, 42}; return t[0];', 42),
            ('t := table of big {99, [0] = 42}; return t[0];', 42),
            ('t := table of big {a = 1, ["a"] = 42}; return t.a;', 42),
            ('x := 40; t := table of big {x, x + 2}; return t[1];', 42),
            ('a := 1; b := 1; t := table of big {big (a == b), 41}; return t[0] + t[1];', 42),
            ('s := table of big {a = 7}; t := table of big {big (delete s.a), big (delete s.a)}; '
             'return t[0] * 42 + t[1];', 42),
            ('s := table of big {a = 7}; t := table of big {[big (delete s.a)] = big (delete s.a)}; '
             'return t[1] + 42;', 42),
            ('s := table of big {["42"] = 7}; keys := keysof s; '
             't := table of big {[keys[0]] = 42}; delete s["42"]; return t["42"];', 42),
            ('t := table of big {old = 40}; if(1 == 1){t := table of big {t.old, 2}; return t[0] + t[1];}else{return 0;}', 42),
            ('t := table of big {40, 2,}; part := t[0:2]; keys := keysof part; '
             'return part[0] + part[1] + len keys - 2;', 42),
            ('t := table of big {a # line comment\n = 40, b /* block */ = 2, # trailing\n}; '
             'return t.a + t.b;', 42),
            ('n := - /* sign trivia */ 40; t := table of big {n, 82}; return t[0] + t[1];', 42),
        )
        for body, expected in cases:
            execute(body, expected)

        # Independent dictionary model checks mixed fields, collisions and zero-based
        # positional numbering; explicitly keyed fields never advance that counter.
        randomizer = random.Random(1648)
        for number in range(32):
            fields = []
            model = {}
            position = 0
            for item in range(3 + number % 12):
                value = randomizer.randrange(-1000, 1001)
                kind = randomizer.randrange(3)
                if kind == 0:
                    fields.append(str(value))
                    model[position] = value
                    position += 1
                elif kind == 1:
                    key = randomizer.choice(('a', 'b', 'field'))
                    fields.append(key + ' = ' + str(value))
                    model[key] = value
                else:
                    key = randomizer.randrange(5)
                    fields.append('[' + str(key) + '] = ' + str(value))
                    model[key] = value
            prefix = 0
            while prefix in model:
                prefix += 1
            accesses = ['t[' + (json.dumps(key) if isinstance(key, str) else str(key)) + ']'
                        for key in model]
            expected = sum(model.values()) + prefix * 997 + len(model) * 31
            execute('t := table of big {' + ', '.join(fields) + ',}; return ' +
                    ' + '.join(accesses) + ' + len t * 997 + countof t * 31;', expected)

        # A store's authored origin is the named key, opening bracket or first
        # positional expression token, including after preceding Unicode bytes.
        execute('t := table of big {first /* comment */ = 40, ["ключ"] = 2, 0,}; '
                'return t.first + t["ключ"];', 42)
        raw = source.read_bytes()
        expected_origins = {(raw.index(b'first'), 5), (raw.index(b'['), 1), (raw.index(b'0,}'), 1)}
        stores = {(event['byte_offset'], event['byte_length'])
                  for event in json.loads((work / 'source.json').read_text())['events']
                  if event['kind'] == 'table-store'}
        if not expected_origins.issubset(stores):
            raise RuntimeError('constructor stores lost exact authored key/element spans')

        # Key validation must precede a failing RHS, even for an unused owner.
        for body, diagnostic in (
            ('unused := table of big {[-1] = 1 / 0}; return 42;', 'invalid key/range'),
            ('unused := table of big {a = 1 / 0}; return 42;', 'zero divide'),
            ('unused := table of big {42, [0] = big "bad"}; return 42;', 'invalid big conversion'),
        ):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            call([source_compiler, 'limbo', '--input', source, fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for path in (fir, optimized):
                if reference:
                    failure = call([reference, path], work, False)
                    if failure.stdout or diagnostic not in failure.stderr:
                        raise RuntimeError('constructor reference changed first eager error')
                call([compiler, 'dis', path, image], work)
                if runtime:
                    failure = call([runtime, mode, '--inspect-words', 0, 2, image], work, False)
                    if 'DATA ' in failure.stdout or diagnostic not in failure.stderr:
                        raise RuntimeError('constructor VM changed first eager error')

        for body in ('t := table of big {,}; return 42;', 't := table of big {1,,2}; return 42;',
                     't := table of big {1 2}; return 42;', 't := table of big {a = }; return 42;',
                     't := table of big {[0] 42}; return 42;', 't := table of big {int 42}; return 42;',
                     't := table of big {a = "42"}; return 42;',
                     't := table of big {[int 0] = 42}; return 42;',
                     't := table of big {nested = table of big {42}}; return 42;'):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            fir.write_text('OLD FIR')
            debug = work / 'source.json'
            debug.write_text('OLD DEBUG')
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
            if fir.read_text() != 'OLD FIR' or debug.read_text() != 'OLD DEBUG':
                raise RuntimeError('constructor rejection replaced staged output/debug')
        source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                          't := table of big {42}; return t[0];}')
        call([source_compiler, 'limbo', '--input', source, fir], work)
        object_file = work / 'old.swo'
        object_file.write_text('OLD OBJECT')
        call([compiler, 'object', fir, object_file], work, False)
        if object_file.read_text() != 'OLD OBJECT':
            raise RuntimeError('unsupported table SWF path replaced output')

        # A flat field list is iterative; SSA limits fail without parser stack growth.
        source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                          't := table of big {' + ','.join(['1'] * 1500) + '}; return 42;}')
        fir.write_text('OLD FIR')
        failure = call([source_compiler, 'limbo', '--input', source, fir], work, False)
        if 'limit' not in failure.stderr or fir.read_text() != 'OLD FIR':
            raise RuntimeError('constructor resource limit lost error/output preservation')
    print('PASS hybrid constructors: ordered fields, collisions, snapshot keys, trivia, spans, types and eager errors' +
          (' with actual Dis ' + mode if runtime else ''))


if __name__ == '__main__':
    main()
