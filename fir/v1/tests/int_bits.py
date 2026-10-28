"""Check append-only signed32 bit operations through source, optimizer and real VMs."""
from pathlib import Path
import json
import os
import re
import subprocess
import sys
import tempfile


# Execute literal tool arguments with a deadline; return streams for expected success/failure.
def call(arguments, success=True):
    result = subprocess.run(list(map(str, arguments)), capture_output=True, text=True, timeout=20)
    if (result.returncode == 0) != success:
        raise RuntimeError(result.stdout + result.stderr)
    return result


# Map Python's unbounded integer result to exact signed32 without implementation-defined casts.
def signed(value):
    value &= 0xffffffff
    return value if value < 0x80000000 else value - 0x100000000


# Explicit argv tools own compilation/execution. Source/IR artifacts stay private here.
# Optional VM/mode and explicit SWF host/linker environment enable actual target acceptance.
def main():
    source_cli, compiler = [Path(value).resolve() for value in sys.argv[1:3]]
    reference = None if sys.argv[3] == "-" else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='exact int bit ops ') as directory:
        root = Path(directory)
        source, fir, optimized, image = [root / name for name in ('input.b', 'raw.fir', 'opt.fir', 'out.dis')]

        # Inspect values and actual dispatch in both raw/optimized images, including GC-at-each-step.
        def execute(expected, swf=False):
            call([compiler, 'check', fir])
            call([compiler, 'opt', fir, optimized])
            for selected in (fir, optimized):
                if reference and call([reference, selected]).stdout.strip() != str(expected):
                    raise RuntimeError('signed32 reference result differs')
                call([compiler, 'dis', selected, image])
                if runtime:
                    traces = len(re.findall(r'^\s*trace ', selected.read_text(), re.M))
                    for quantum in (1, 2048):
                        result = call([runtime, mode, '--quantum', quantum, '--stats', '--inspect-words', 0, 2 * traces, image])
                        words = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)]
                        flags = words[traces:]
                        stats = re.search(r'instructions=(\d+) native=(\d+)', result.stderr)
                        if (len(words) != 2 * traces or any(flag not in (0, 1) for flag in flags) or flags.count(1) != 1 or
                                [word for word, flag in zip(words[:traces], flags) if flag] != [expected] or
                                any(word for word, flag in zip(words[:traces], flags) if not flag)):
                            raise RuntimeError('actual Dis bit result/capture mismatch')
                        if not stats or int(stats[1]) == 0 or (int(stats[2]) > 0) != (mode == '--jit'):
                            raise RuntimeError('actual Dis bit dispatch mismatch')
                        if 'tables_live=0 keys_live=0' not in result.stderr:
                            raise RuntimeError('bit fixture retained table/key owners')
                if swf and os.environ.get('FIR_FLASHVM_RUNNER'):
                    obj, movie = root / 'bits.swo', root / 'bits.swf'
                    call([compiler, 'object', selected, obj])
                    call([os.environ['FIR_SWL'], 'link', movie, obj])
                    if float(call([os.environ['FIR_FLASHVM_RUNNER'], movie]).stdout.strip()) != expected:
                        raise RuntimeError('actual SWF bit result differs')

        # Endpoints and unlike signs exercise native WORD64 normalization, not double arithmetic.
        values = [-2147483648, -2147483647, -1, 0, 1, 42, 1073741824, 2147483647]
        for left in values:
            for right in values:
                for op, expected in [('&', signed(left & right)), ('|', signed(left | right)), ('^', signed(left ^ right))]:
                    source.write_text('implement M; M: module {value: fn(): int;}; value(): int {'
                                      'a := ' + str(left) + '; b := ' + str(right) + '; return a ' + op + ' b;}')
                    call([source_cli, 'limbo', '--input', source, fir])
                    execute(expected, True)
        for body, expected, swf in [
            ('N: type int; a: N = N -2147483648; return ~a;', 2147483647, True),
            ('a := 123; a &= 63; a ^= 17; a |= 1; return a;', 43, True),
            ('return 1 | 2 ^ 3 & 6 + 1;', 1, True),
            ('return (1 + 2 << 3) | 2;', 26, False),
            ('a := 1; a <<= 31; a >>= 31; return a;', -1, False),
            ('a := 42; if ((a & 63) == 42) {return 42;} else {return 0;}', 42, True),
            ('a := 43; if ((a & 63) == 42) {return 0;} else {return a ^ 1;}', 42, True),
        ]:
            source.write_text('implement M; M: module {value: fn(): int;}; value(): int {' + body + '}')
            call([source_cli, 'limbo', '--input', source, fir])
            execute(expected, swf)

        # The typed source profile reuses the same primitive instead of a widening sandwich.
        source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                          't := table of big {a = big -2147483648, b = big 42};'
                          'return big ((int t.a) | (int t.b));}')
        call([source_cli, 'limbo', '--input', source, fir])
        if ' = i32.or ' not in fir.read_text() or ' = big.or ' in fir.read_text():
            raise RuntimeError('typed source did not use the direct signed32 primitive')
        execute(-2147483606)

        # Loaded operands force exact int complement through the hosted backend without folding.
        # Nested complement must retain signed endpoints and evaluate its checked input once.
        for value in values:
            for expression, expected in [('~(int t.a)', signed(~value)),
                                         ('~~(int t.a)', value),
                                         ('(~(int t.a)) ^ (int t.a)', -1)]:
                source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                                  't := table of big {a = big ' + str(value) + '};'
                                  'return big (' + expression + ');}')
                call([source_cli, 'limbo', '--input', source, fir])
                text = fir.read_text()
                if ' = i32.xor ' not in text or ' = big.not ' in text:
                    raise RuntimeError('typed complement retained its widening sandwich')
                if text.count(' = table.load ') != expression.count('t.a'):
                    raise RuntimeError('typed complement reevaluated a borrowed operand')
                execute(expected)

        # A real table load prevents folding and forces the hosted Dis emitter for the new ops.
        for op in ('and', 'or', 'xor'):
            fir.write_text('''FIR 1.0
module @loaded_bits
func @main() -> void {
^entry:
 %t: table.big = table.new
 %k: string = const.string "n"
 %slot: table.slot = table.index %t, %k
 %n: i64 = const.i64 -2147483648
 table.store %slot, %n
 %loaded: i64 = table.load %slot
 %a: i32 = value.int %loaded
 %b: i32 = const.i32 42
 %bits: i32 = i32.'''+op+''' %a, %b
 trace %bits
 return
}
''')
            execute({'and': 0, 'or': -2147483606, 'xor': -2147483606}[op])

        # Checked shifts remain eager even when unused; a skipped terminal arm stays lazy.
        for body in ('a := 1; unused := a << 32; return 42;', 'a := -1; unused := a >> -1; return 42;'):
            source.write_text('implement M; M: module {value: fn(): int;}; value(): int {' + body + '}')
            call([source_cli, 'limbo', '--input', source, fir])
            call([compiler, 'opt', fir, optimized])
            for selected in (fir, optimized):
                if reference:
                    call([reference, selected], False)
                call([compiler, 'dis', selected, image])
                if runtime:
                    call([runtime, mode, image], False)
        source.write_text('implement M; M: module {value: fn(): int;}; value(): int {'
                          'if (1 == 0) {return 1 << 32;} else {return 42;}}')
        call([source_cli, 'limbo', '--input', source, fir])
        execute(42)

        # Direct pure bits may die, but a checked producer must still throw exactly before trace.
        source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                          'parsed := int "bad"; unused := ~(parsed & int 0); return big 42;}')
        call([source_cli, 'limbo', '--input', source, fir])
        call([compiler, 'opt', fir, optimized])
        if ' = i32.and ' in optimized.read_text() or ' = i32.xor ' in optimized.read_text():
            raise RuntimeError('unused pure bit operation was not removed')
        for selected in (fir, optimized):
            if reference:
                call([reference, selected], False)
            call([compiler, 'dis', selected, image])
            if runtime and 'invalid big conversion' not in call([runtime, mode, image], False).stderr:
                raise RuntimeError('bit DCE erased its checked producer failure')

        # Live checked shifts remain outside SWF coverage and must preserve old object bytes.
        source.write_text('implement M; M: module {value: fn(): int;}; value(): int {a := 21; return a << 1;}')
        call([source_cli, 'limbo', '--input', source, fir])
        obj = root / 'preserved.swo'
        obj.write_bytes(b'OLD')
        call([compiler, 'object', fir, obj], False)
        if obj.read_bytes() != b'OLD':
            raise RuntimeError('unsupported live checked SWF shift replaced old object')

        # Strict source kind boundaries and condition precedence must not become implicit coercions.
        for language, text in [
            ('as1', 'trace(1 & 2);'), ('as2', 'var a=1; a ^= 2; trace(a);'),
            ('as3', 'trace(~1);'),
            ('limbo', 'implement M; M: module {value: fn(): real;}; value(): real {return 1.0 | 2.0;}'),
            ('limbo', 'implement M; M: module {value: fn(): int;}; value(): int {a:=1; if(a & 3 == 1){return 42;}else{return 0;}}'),
        ]:
            source.write_text(text)
            fir.write_text('old FIR')
            call([source_cli, language, '--input', source, fir, '--diagnostics-json'], False)
            if fir.read_text() != 'old FIR':
                raise RuntimeError('rejected bit source replaced staged output')

        # Shared source positions must identify each authored bit/complement operator after Unicode.
        text = '# π\nimplement M; M: module {value: fn(): int;}; value(): int {return ~(7 & 3) ^ 42;}'
        source.write_text(text)
        debug = root / 'debug.json'
        call([source_cli, 'limbo', '--input', source, fir, '-g', debug])
        spans = {(event['byte_offset'], event['byte_length']) for event in json.loads(debug.read_text())['events']
                 if event['kind'] == 'binary'}
        for token in (b'~', b'&', b'^'):
            if (text.encode().index(token), len(token)) not in spans:
                raise RuntimeError('bit lowering lost authored operator position')
        text = '# π\nimplement M; M: module {value: fn(): int;}; value(): int {a:=42; a &= 63; a ^= 0; a <<= 1; a >>= 1; return a;}'
        source.write_text(text)
        call([source_cli, 'limbo', '--input', source, fir, '-g', debug])
        spans = {(event['byte_offset'], event['byte_length']) for event in json.loads(debug.read_text())['events']
                 if event['kind'] == 'binary'}
        for token in (b'&=', b'^=', b'<<=', b'>>='):
            if (text.encode().index(token), len(token)) not in spans:
                raise RuntimeError('compound bit lowering lost authored operator position')
        # The hosted profile must also retain the exact unary token after UTF-8 source text.
        text = '# π\nimplement M; M: module {value: fn(): big;}; value(): big {' \
               't := table of big {x = big -43}; return big (~(int t.x));}'
        source.write_text(text)
        call([source_cli, 'limbo', '--input', source, fir, '-g', debug])
        events = json.loads(debug.read_text())['events']
        complement = re.search(r'%v(\d+): i32 = i32.xor ', fir.read_text())
        if not complement:
            raise RuntimeError('typed complement omitted direct xor')
        position = text.encode().index(b'~')
        if not any(event['kind'] == 'table-operation' and event['name'] == 'i32.xor' and
                   event['value'] == int(complement[1]) and event['byte_offset'] == position and
                   event['byte_length'] == 1 for event in events):
            raise RuntimeError('typed complement lost its authored UTF-8 token span')
    print('PASS signed32 bits: endpoints, raw/opt, both Dis emitters, SWF, scope and debug spans')


if __name__ == '__main__':
    main()
