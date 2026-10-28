"""Check ordered binary64 predicates, unordered values and Limbo terminal real CFG."""
from pathlib import Path
import json
import math
import os
import re
import struct
import subprocess
import sys
import tempfile


# Run explicit tools with a deadline; return copied streams or reject an unexpected status.
def call(arguments, success=True):
    result = subprocess.run(list(map(str, arguments)), capture_output=True, text=True, timeout=20)
    if (result.returncode == 0) != success:
        raise RuntimeError(result.stdout + result.stderr)
    return result


# Build a value from finite FIR constants only; NaN/Infinity must arise at execution time.
def definition(name, value):
    if math.isnan(value):
        return f'%{name}0: f64 = const.f64 0.0\n%{name}: f64 = f64.div %{name}0, %{name}0\n'
    if math.isinf(value):
        return f'%{name}0: f64 = const.f64 '+('-1.0' if value < 0 else '1.0')+f'\n%{name}1: f64 = const.f64 0.0\n%{name}: f64 = f64.div %{name}0, %{name}1\n'
    return f'%{name}: f64 = const.f64 {value!r}\n'


# Own private source/IR/images. Optional runtime/mode verifies actual dispatch and capture bits.
def main():
    source_cli, compiler = [Path(value).resolve() for value in sys.argv[1:3]]
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    header = 'FIR 1.0\nmodule @real_conditions\nfunc @main() -> void {\n^entry:\n'
    wrapper = 'implement M; M: module {value: fn(): real;}; value(): real {'
    with tempfile.TemporaryDirectory(prefix='ordered real conditions ') as directory:
        root = Path(directory)
        source, fir, optimized, image = [root/name for name in ('input.b', 'raw.fir', 'opt.fir', 'out.dis')]

        # Inspect integer bits for both bool and binary64 captures in one stable view.
        # Raw and simplified CFG may have different capture counts; exactly one flag must be set.
        def execute(expected, real=False, swf=False):
            call([compiler, 'check', fir])
            call([compiler, 'opt', fir, optimized])
            for selected in (fir, optimized):
                if reference:
                    observed = float(call([reference, selected]).stdout.strip())
                    if observed != expected:
                        raise RuntimeError('independent C99 real predicate result differs')
                call([compiler, 'dis', selected, image])
                if runtime:
                    traces = len(re.findall(r'^\s*trace ', selected.read_text(), re.M))
                    result = call([runtime, mode, '--quantum', 1, '--stats', '--inspect-words', 0, 2*traces, image])
                    words = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)]
                    flags = words[traces:]
                    stats = re.search(r'instructions=(\d+) native=(\d+)', result.stderr)
                    if len(words) != 2*traces or flags.count(1) != 1 or any(flag not in (0, 1) for flag in flags):
                        raise RuntimeError('real predicate branch capture shape differs')
                    live = words[flags.index(1)]
                    wanted = struct.unpack('>q', struct.pack('>d', expected))[0] if real else expected
                    if live != wanted or any(word for word, flag in zip(words[:traces], flags) if not flag):
                        raise RuntimeError('real predicate value/unused capture bits differ')
                    if not stats or int(stats[1]) == 0 or (int(stats[2]) > 0) != (mode == '--jit'):
                        raise RuntimeError('actual real predicate dispatch differs')
                    if 'tables_live=0 keys_live=0' not in result.stderr:
                        raise RuntimeError('real scalar fixture retained hosted owners')
                if swf and os.environ.get('FIR_FLASHVM_RUNNER'):
                    obj, movie = root/'real.swo', root/'real.swf'
                    call([compiler, 'object', selected, obj])
                    call([os.environ['FIR_SWL'], 'link', movie, obj])
                    trace = call([os.environ['FIR_FLASHVM_RUNNER'], movie]).stdout.strip().lower()
                    observed = 1.0 if trace == 'true' else 0.0 if trace == 'false' else float(trace)
                    # Numeric trace is display text, not a binary64 payload channel.
                    equal = observed == expected or (real and math.isclose(observed, expected, rel_tol=2e-14, abs_tol=0.0))
                    if not equal:
                        raise RuntimeError(f'actual SWF result {observed} differs from {expected}:\n{selected.read_text()}')

        # The executable reader must accept the same annotated version line as the syntax frontend.
        header_body = 'module @header\nfunc @main() -> void {\n^entry:\n%yes: bool = const.bool true\ntrace %yes\nreturn\n}\n'
        for version in ('FIR 1.0 // π version comment\n', 'FIR 1.0\t// comment\r\n'):
            fir.write_text(version+header_body)
            execute(1, swf=True)
        for version in ('FIR 1.0 \n', 'FIR 1.0// missing separator\n', '// leading comment\nFIR 1.0\n', '\ufeffFIR 1.0\n'):
            fir.write_text(version+header_body)
            image.write_bytes(b'OLD')
            call([compiler, 'dis', fir, image], False)
            if image.read_bytes() != b'OLD':
                raise RuntimeError('rejected physical header replaced staged Dis')

        # The same ordered predicates run in scalar and true hosted frames with GC each step.
        values = [-math.inf, -1.0, -0.0, 0.0, math.inf, math.nan]
        operations = [('eq', lambda a, b: a == b), ('lt', lambda a, b: a < b), ('le', lambda a, b: a <= b)]
        for left in values:
            for right in values:
                for operation, compare in operations:
                    for hosted in (False, True):
                        owner = '%t: table.big = table.new\n' if hosted else ''
                        fir.write_text(header+owner+definition('a', left)+definition('b', right)+
                                       '%copy: f64 = copy %a\n%r: bool = f64.'+operation+' %copy, %b\ntrace %r\nreturn\n}\n')
                        execute(int(compare(left, right)), swf=not hosted)

        # Source finite endpoints, subnormal and adjacent values above 2^53 exercise all six relations.
        values = [-1e308, -1.0, -0.0, 0.0, 5e-324, 1e-310, 9007199254740992.0, 9007199254740994.0]
        relations = [('==', lambda a, b: a == b), ('!=', lambda a, b: a != b),
                     ('<', lambda a, b: a < b), ('<=', lambda a, b: a <= b),
                     ('>', lambda a, b: a > b), ('>=', lambda a, b: a >= b)]
        for left in values:
            for right in values:
                for relation, compare in relations:
                    source.write_text(wrapper+f'a := {left!r}; b := {right!r}; if(a {relation} b)'
                                      '{return 42.5;}else{return -7.25;}}')
                    call([source_cli, 'limbo', '--input', source, fir])
                    if ' = f64.' not in fir.read_text() or ': i32 =' in fir.read_text():
                        raise RuntimeError('real source lost homogeneous comparison typing')
                    execute(42.5 if compare(left, right) else -7.25, real=True, swf=True)

        # Runtime unordered/overflow values must not turn <=/>= into the inverse of <.
        for expression, left in [('0.0 / 0.0', math.nan), ('1.0 / 0.0', math.inf),
                                 ('1e308 * 1e308', math.inf), ('-1e308 * 1e308', -math.inf)]:
            for relation, compare in relations:
                source.write_text(wrapper+'a := '+expression+'; if(a '+relation+' 0.0)'
                                  '{return 42.5;}else{return -7.25;}}')
                call([source_cli, 'limbo', '--input', source, fir])
                execute(42.5 if compare(left, 0.0) else -7.25, real=True, swf=True)

        # Shared numeric readers must retain representable subnormals in every AS adapter too.
        for language in ('as1', 'as2', 'as3'):
            source.write_text('trace(5e-324);')
            call([source_cli, language, '--input', source, fir])
            execute(5e-324, real=True, swf=True)

        # Values rounding to the normal/subnormal boundary remain finite on the host reader.
        for literal in ('5e-324', '-5e-324', '2.2250738585072014e-308', '2.2250738585072013e-308'):
            source.write_text(wrapper+'return '+literal+';}')
            call([source_cli, 'limbo', '--input', source, fir])
            execute(float(literal), real=True, swf=True)

        # A selected terminal arm may contain local aliases; an unselected arm must stay lazy.
        source.write_text(wrapper+'R: type real; a: R = R 2.5; if(a <= 3.0){'
                          'R: type real; b: R = R 17.0; if(b > a){return b * a;}else{return -1.0;}}'
                          'else{return 0.0 / 0.0;}}')
        call([source_cli, 'limbo', '--input', source, fir])
        execute(42.5, real=True, swf=True)

        # Hosted real predicates do not silently introduce an unsupported real trace ABI.
        fir.write_text(header+'%t: table.big = table.new\n'+definition('a', 1.0)+'trace %a\nreturn\n}\n')
        image.write_bytes(b'OLD')
        call([compiler, 'dis', fir, image], False)
        if image.read_bytes() != b'OLD':
            raise RuntimeError('rejected hosted real trace replaced staged Dis')

        # Ordered predicates are pure, but a checked non-real producer must retain its failure.
        fir.write_text(header+'%s: string = const.string "bad"\n%parsed: i64 = value.big %s\n%checked: i32 = value.int %parsed\n'+
                       definition('a', 1.0)+'%unused: bool = f64.le %a, %a\ntrace %checked\nreturn\n}\n')
        call([compiler, 'opt', fir, optimized])
        if ' = f64.le ' in optimized.read_text():
            raise RuntimeError('unused pure real comparison survived DCE')
        for selected in (fir, optimized):
            if reference:
                call([reference, selected], False)
            call([compiler, 'dis', selected, image])
            if runtime:
                call([runtime, mode, image], False)

        # Type boundaries and resource staging remain strict despite adding real CFG.
        for body in ['a := 1.0; if(a < 2){return 1.0;}else{return 2.0;}',
                     'a := 1.0; if(a){return 1.0;}else{return 2.0;}',
                     'return 1.0 < 2.0;', 'if(1.0<2.0){return 1.0;}else{return 2;}', 'return 1e-400;']:
            source.write_text(wrapper+body+'}')
            fir.write_text('OLD')
            call([source_cli, 'limbo', '--input', source, fir], False)
            if fir.read_text() != 'OLD':
                raise RuntimeError('invalid real condition replaced staged FIR')

        # Restored relation marks must point at the exact token after a Unicode prefix.
        for relation in ('==', '!=', '<', '<=', '>', '>='):
            text = '# π😀\n'+wrapper+'a:=1.0; if(a '+relation+' 2.0){return 42.5;}else{return -7.25;}}'
            source.write_text(text)
            debug = root/'debug.json'
            call([source_cli, 'limbo', '--input', source, fir, '-g', debug])
            events = json.loads(debug.read_text())['events']
            position = text.encode().index(relation.encode())
            if not any(event['kind'] == 'compare' and event['byte_offset'] == position and
                       event['byte_length'] == len(relation) for event in events):
                raise RuntimeError('real condition lost its exact authored UTF-8 token')
    print('PASS ordered real conditions: NaN/Infinity/zero, scalar/hosted Dis, source CFG, SWF and origins')


if __name__ == '__main__':
    main()
