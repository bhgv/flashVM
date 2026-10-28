"""Check strict bool FIR and direct int predicates through real compiler/VM paths."""
from pathlib import Path
import json
import os
import re
import subprocess
import sys
import tempfile


# Run explicit tool arguments with a deadline; return captured streams or reject status mismatch.
def call(arguments, success=True):
    result = subprocess.run(list(map(str, arguments)), capture_output=True, text=True, timeout=20)
    if (result.returncode == 0) != success:
        raise RuntimeError(result.stdout + result.stderr)
    return result


# Own temporary source/IR/images. Optional VM mode checks actual dispatch, never a compiler oracle alone.
def main():
    source_cli, compiler = [Path(value).resolve() for value in sys.argv[1:3]]
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    header = 'FIR 1.0\nmodule @predicates\nfunc @main() -> void {\n^entry:\n'
    wrapper = 'implement M; M: module {value: fn(): big;}; value(): big {'
    with tempfile.TemporaryDirectory(prefix='typed predicates ') as directory:
        root = Path(directory)
        source, fir, optimized, image = [root/name for name in ('input.b', 'raw.fir', 'opt.fir', 'out.dis')]

        # Compare raw/optimized results, native dispatch and temporary owner release for one trace.
        def execute(expected, swf=False):
            call([compiler, 'check', fir])
            call([compiler, 'opt', fir, optimized])
            for selected in (fir, optimized):
                if reference and call([reference, selected]).stdout.strip() != str(expected):
                    raise RuntimeError('predicate reference result differs')
                call([compiler, 'dis', selected, image])
                if runtime:
                    result = call([runtime, mode, '--quantum', 1, '--stats', '--inspect-words', 0, 2, image])
                    words = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)]
                    stats = re.search(r'instructions=(\d+) native=(\d+)', result.stderr)
                    if words != [expected, 1] or not stats or int(stats[1]) == 0 or (int(stats[2]) > 0) != (mode == '--jit'):
                        raise RuntimeError('predicate value/capture/dispatch differs')
                    if 'tables_live=0 keys_live=0' not in result.stderr:
                        raise RuntimeError('predicate fixture retained table/key owners')
                if swf and os.environ.get('FIR_FLASHVM_RUNNER'):
                    obj, movie = root/'bool.swo', root/'bool.swf'
                    call([compiler, 'object', selected, obj])
                    call([os.environ['FIR_SWL'], 'link', movie, obj])
                    trace = call([os.environ['FIR_FLASHVM_RUNNER'], movie]).stdout.strip().lower()
                    if trace not in (str(expected), 'true' if expected else 'false'):
                        raise RuntimeError('actual linked SWF bool result differs')

        # Scalar raw FIR covers both bool values, equality pairs, nesting, branches and SWF actions.
        for left in (0, 1):
            for right in (0, 1):
                definitions = '%a: bool = const.bool '+('true' if left else 'false')+'\n' + \
                              '%b: bool = const.bool '+('true' if right else 'false')+'\n'
                for expression, expected in [('bool.not %a', 1-left), ('bool.eq %a, %b', int(left == right))]:
                    fir.write_text(header+definitions+'%r: bool = '+expression+'\ntrace %r\nreturn\n}\n')
                    execute(expected, True)
                fir.write_text(header+definitions+'%r: bool = bool.eq %a, %b\n%n: bool = bool.not %r\n'
                               'if %n, ^yes, ^no\n^yes:\ntrace %a\njump ^done\n^no:\ntrace %b\n'
                               'jump ^done\n^done:\nreturn\n}\n')
                # Separate branch captures have two value slots followed by two validity slots.
                call([compiler, 'opt', fir, optimized])
                for selected in (fir, optimized):
                    call([compiler, 'dis', selected, image])
                    if runtime:
                        traces = len(re.findall(r'^\s*trace ', selected.read_text(), re.M))
                        result = call([runtime, mode, '--quantum', 1, '--inspect-words', 0, 2 * traces, image])
                        words = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)]
                        expected = ([left, 0, 1, 0] if left != right else [0, right, 0, 1]) if traces == 2 else [left if left != right else right, 1]
                        if words != expected:
                            raise RuntimeError('bool branch PC/capture mapping differs')

        # Real table reads keep signed32 comparisons live through the hosted emitter.
        values = [-2147483648, -1, 0, 1, 42, 2147483647]
        relations = [('==', lambda a, b: a == b), ('!=', lambda a, b: a != b),
                     ('<', lambda a, b: a < b), ('<=', lambda a, b: a <= b),
                     ('>', lambda a, b: a > b), ('>=', lambda a, b: a >= b)]
        for left in values:
            for right in values:
                for relation, compare in relations:
                    source.write_text(wrapper+'t := table of big {a = big '+str(left)+', b = big '+str(right)+'};'
                                      'a := int t.a; b := int t.b; return big (a '+relation+' b);}')
                    call([source_cli, 'limbo', '--input', source, fir])
                    text = fir.read_text()
                    if (' = i32.eq ' not in text and ' = i32.lt.signed ' not in text) or ' = big.eq ' in text or ' = big.lt' in text:
                        raise RuntimeError('typed int comparison retained a widening path')
                    if text.count(' = table.load ') != 2:
                        raise RuntimeError('comparison reevaluated an operand')
                    execute(int(compare(left, right)))

        # Bool equality/inversion need no numeric conversion, including nested logical expressions.
        for left in (0, 1):
            for right in (0, 1):
                for expression, expected in [('a == b', left == right), ('a != b', left != right),
                                             ('!(a == b)', left != right), ('!!a', bool(left)),
                                             ('!(a && b)', not(left and right)), ('!(a || b)', not(left or right))]:
                    source.write_text(wrapper+'t := table of big {a = big '+str(left)+', b = big '+str(right)+'};'
                                      'a := (int t.a) != int 0; b := (int t.b) != int 0; return big ('+expression+');}')
                    call([source_cli, 'limbo', '--input', source, fir])
                    if ' = big.eq ' in fir.read_text() or ' = big.lt' in fir.read_text():
                        raise RuntimeError('bool expression retained numeric comparison')
                    execute(int(expected))

        # Arithmetic beside a bool operation must retain its own hosted switch cases.
        for expression, expected in [('a + b', 50), ('a - b', 34), ('a * b', 336)]:
            source.write_text(wrapper+'t := table of big {a = big 42, b = big 8};'
                              'a := int t.a; b := int t.b; flag := !(a == b);'
                              'return big ('+expression+') + big flag;}')
            call([source_cli, 'limbo', '--input', source, fir])
            execute(expected+1)

        # Unknown loaded bools exercise algebraic rewrites, not constant-only folding.
        for expression, expected, remaining_not in [('!!a', 1, False), ('a == a', 1, False),
                                                   ('a == (int 0 == int 0)', 1, False),
                                                   ('a == (int 0 != int 0)', 0, True)]:
            source.write_text(wrapper+'t := table of big {a = big 0};'
                              'a := (int t.a) == int 0; return big ('+expression+');}')
            call([source_cli, 'limbo', '--input', source, fir])
            execute(expected)
            text = optimized.read_text()
            if ' = bool.eq ' in text or (' = bool.not ' in text) != remaining_not:
                raise RuntimeError('bool algebraic rewrite missed an unknown operand')

        # Strict source typing and rejected programs preserve the caller's old output.
        for expression in ['(int 0) == big 0', '(int 0) == (int 0 == int 0)',
                           '(int 0 == int 0) < (int 1 == int 1)', '!(int 0)']:
            source.write_text(wrapper+'return big ('+expression+');}')
            fir.write_text('OLD')
            call([source_cli, 'limbo', '--input', source, fir], False)
            if fir.read_text() != 'OLD':
                raise RuntimeError('rejected predicate replaced staged output')

        # Pure predicates may die while checked operand failures remain observable.
        source.write_text(wrapper+'a := int "bad"; unused := !((a < int 0) == (a == int 0)); return big 42;}')
        call([source_cli, 'limbo', '--input', source, fir])
        call([compiler, 'opt', fir, optimized])
        if any(' = '+op+' ' in optimized.read_text() for op in ('bool.not', 'bool.eq', 'i32.eq', 'i32.lt.signed')):
            raise RuntimeError('unused pure predicates survived dead-code elimination')
        for selected in (fir, optimized):
            if reference:
                call([reference, selected], False)
            call([compiler, 'dis', selected, image])
            if runtime and 'invalid big conversion' not in call([runtime, mode, image], False).stderr:
                raise RuntimeError('predicate elimination erased a checked producer failure')

        # Debug origins identify each authored typed operator after a multibyte prefix.
        text = '# π\n'+wrapper+'a := int 1; b := int 2; return big !((a < b) == (a != b));}'
        source.write_text(text)
        debug = root/'debug.json'
        call([source_cli, 'limbo', '--input', source, fir, '-g', debug])
        events = json.loads(debug.read_text())['events']
        for op, token in [('i32.lt.signed', b'<'), ('i32.eq', b'!='), ('bool.eq', b'=='), ('bool.not', b'!')]:
            position = text.encode().index(token)
            if not any(event['kind'] == 'table-operation' and event['name'] == op and
                       event['byte_offset'] == position and event['byte_length'] == len(token) for event in events):
                raise RuntimeError('typed predicate lost authored token provenance')
    print('PASS typed predicates: strict bool, signed endpoints, raw/opt, both Dis paths and debug origins')


if __name__ == '__main__':
    main()
