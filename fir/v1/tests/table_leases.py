"""Check automatic retained ownership through Coco/R, FIR optimization and Dis."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Receive explicit source/FIR/reference tools, optionally VM and mode. Own every
# temporary file; compare independent results and fail on missing scope effects.
def main():
    source_compiler, compiler = (Path(x).resolve() for x in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='retained table scopes ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image, debug = (work / n for n in
                                               ('input.b', 'input.fir', 'optimized.fir', 'out.dis', 'source.json'))
        cases = [
            ('t := table of big {40}; t[0] += 2; return t[0];', 42, 1),
            ('t := table of big {1,2}; a := t; a[-1] = t[0]+41; return t[-1];', 42, 1),
            ('t := table of big {20}; t[0] = t[0]+t[0]+2; return t[0];', 42, 1),
            ('t := table of big {21}; t[0] <<= int 1; return t[0];', 42, 1),
            ('t := table of big {40}; t.score = t[0]+2; return t.score;', 42, 1),
            ('t := table of big {score=40}; k := keysof t; t[k[0]] += 2; return t.score;', 42, 1),
            ('t := table of big {40,2}; t[0:-1] = t; return t[0]+t[-1];', 42, 1),
            ('t := table of big {40,2}; t[0:-1] = t[0:-1]; return t[0]+t[-1];', 42, 1),
            ('t := table of big {1,2}; t[0:1] = table of big {40}; return t[0]+t[1];', 42, 1),
            ('t := table of big {40}; if (t[0] == 40) {t[0] += 2; return t[0];} else {t[0] = 0; return t[0];}', 42, 2),
            ('t := table of big {40}; t[0] += 1; t[0] += 1; return t[0];', 42, 2),
            ('s := "40"; s[int 0:int -1] = "42"; return big s;', 42, 0),
        ]
        tutorial = Path(__file__).resolve().parents[1] / 'examples/tables/leases.b'
        tutorial_body = tutorial.read_text().split('value(): big\n{', 1)[1].rsplit('}', 1)[0]
        cases.append((tutorial_body, 42, 4))
        for body, expected, scopes in cases:
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {\n' + body + '\n}')
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work)
            events = json.loads(debug.read_text())['events']
            locks = [e for e in events if e['kind'] == 'table-lock']
            leaves = [e for e in events if e['kind'] == 'table-unlock']
            if len(locks) != scopes or len(leaves) != scopes:
                raise RuntimeError('missing source ownership events: ' + body)
            raw = source.read_bytes()
            for entry, leave in zip(locks, leaves):
                span = raw[entry['byte_offset']:entry['byte_offset'] + entry['byte_length']]
                if span not in (b't', b'a', b'document', b'alias') or any(entry[k] != leave[k] for k in
                                                    ('byte_offset', 'byte_length', 'line', 'column')):
                    raise RuntimeError('ownership boundary lost destination span')
            call([compiler, 'check', fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                text = selected.read_text()
                if len(re.findall(r'\btable.lock ', text)) != scopes or len(re.findall(r'\btable.unlock ', text)) != scopes:
                    raise RuntimeError('optimizer removed ownership effects')
                # Each lexical assignment scope encloses location capture and a write.
                for scope in re.findall(r'table.lock .*?table.unlock [^\n]+', text, re.S):
                    if not re.search(r'table\.(?:store|splice) ', scope) or not re.search(r'table\.(?:index|range) ', scope):
                        raise RuntimeError('lease does not enclose destination and commit')
                if reference:
                    result = call([reference, selected], work)
                    if result.stdout.strip() != str(expected):
                        raise RuntimeError('sequential ownership oracle mismatch')
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    for quantum in (1, 2048):
                        result = call([runtime, mode, '--quantum', quantum, '--stats', '--inspect-words', 0, 4 if 'if (' in body else 2, image], work)
                        values = [int(x) for x in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)]
                        if values != ([expected, 0, 1, 0] if 'if (' in body else [expected, 1]) or 'tables_live=0 keys_live=0' not in result.stdout + result.stderr:
                            raise RuntimeError('native retained scope result/root mismatch: ' + body + '\n' + result.stdout + result.stderr)

        # Even an otherwise unused pair is observable; DCE must keep the owner.
        fir.write_text('FIR 1.0\nmodule @scope\nfunc @main() -> void {\n^entry:\n'
                       '%t: table.big = table.new\ntable.lock %t\ntable.unlock %t\nreturn\n}\n')
        call([compiler, 'opt', fir, optimized], work)
        text = optimized.read_text()
        if 'table.lock %t' not in text or 'table.unlock %t' not in text or 'table.new' not in text:
            raise RuntimeError('dead-code elimination removed unused ownership effects')

        call([compiler, 'dis', optimized, image], work)
        if b'acquire\0' not in image.read_bytes() or b'release\0' not in image.read_bytes():
            raise RuntimeError('retained effects omitted checked module imports')
        fir.write_text('FIR 1.0\nmodule @old\nfunc @main() -> void {\n^entry:\n'
                       '%t: table.big = table.new\n%n: i64 = table.count %t\ntrace %n\nreturn\n}\n')
        call([compiler, 'dis', fir, image], work)
        if b'acquire\0' in image.read_bytes() or b'release\0' in image.read_bytes():
            raise RuntimeError('legacy FIR unnecessarily requires the new runtime API')

        # Invalid typed effects must fail before replacing an existing output file.
        for operation in ('table.lock', 'table.unlock'):
            fir.write_text('FIR 1.0\nmodule @bad\nfunc @main() -> void {\n^entry:\n'
                           '%x: i64 = const.i64 42\n' + operation + ' %x\nreturn\n}\n')
            image.write_bytes(b'OLD')
            call([compiler, 'dis', fir, image], work, False)
            if image.read_bytes() != b'OLD':
                raise RuntimeError('invalid lease published bytecode')
    print('PASS automatic table scopes: destination/RHS/store, optimizer, spans, types' +
          (' and both native scheduling quanta' if runtime else ''))


if __name__ == '__main__':
    main()
