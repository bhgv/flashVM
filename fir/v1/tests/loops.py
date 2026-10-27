"""Exercise source while/for-in through optimization, reference and native Dis."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Receive source/FIR/reference tools and optional runtime/mode. Own staged files;
# compare independent results, exact source spans and bounded failure behavior.
def main():
    source_compiler, compiler = (Path(x).resolve() for x in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    cases = [
        ('i := big 0; sum := big 0; while (i < 7) {sum += i; i += 1;} return sum*2;', 42),
        ('i := big 42; while (i < 0) {i = 0;} return i;', 42),
        ('i := big 0; sum := big 0; while (i < 3) {j := big 0; while (j < 2) {sum += 7; j += 1;} i += 1;} return sum;', 42),
        ('i := big 0; sum := big 0; while (i < 3 && sum < 100) {part := i; sum += part; i += 1;} return sum+39;', 42),
        ('t := table of big {10,11,score=21}; sum := big 0; for (k,v in t) {sum += v; t[k] = v+1;} return sum;', 42),
        ('t := table of big {10,11,score=21}; sum := big 0; for (k,v in t) {sum += v; t.extra = 999;} return sum;', 42),
        ('t := table of big {10,11,score=21}; sum := big 0; for (k,v in t) {sum += v; delete t[k];} return sum+len t;', 42),
        ('t := table of big {10,11,21}; sum := big 0; for (k,v in t) {sum += v; t[1] = 0;} return sum+11;', 42),
        ('t := table of big; sum := big 42; for (k,v in t) {sum = 0;} return sum;', 42),
        ('t := table of big {2,3}; u := table of big {4,5}; sum := big 0; for (k,v in t) {for (j,w in u) {sum += v*w;}} return sum-3;', 42),
        ('k := big 100; t := table of big {1,2}; for (k,v in t) {t[k] = v;} return k-58;', 42),
        ('t := table of big {1,2}; for (k,v in t) {i := big 0; while (i < 2) {v += 10; i += 1;} t[k] = v;} return t[0]+t[1]-1;', 42),
    ]
    cases.extend([
        ('i := big 9007199254740993; sum := big 0; while (i < 9007199254740995) {sum += i; i += 1;} return sum-18014398509481945;',42),
        ('i := big 0; while (i < 3) {i += 1;} while (i < 7) {i += 1;} return i*6;',42),
        ('i := big 0; sum := big 0; while (i > -3) {sum += i; i -= 1;} return sum+45;',42),
        ('value := big 40; t := table of big {1,2}; for (k,value in t) {t[k] = value+1;} return value+2;',42),
        ('t := table of big {counter=0}; while (t.counter < 7) {t.counter += 1;} return t.counter*6;',42),
    ])
    tutorial = Path(__file__).resolve().parents[1]/'examples/tables/loops.b'
    body = tutorial.read_text().split('value(): big\n{',1)[1].rsplit('}',1)[0]
    cases.append((body,42))
    with tempfile.TemporaryDirectory(prefix='source loops ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image, debug = (work/n for n in ('input.b','input.fir','opt.fir','out.dis','source.json'))
        for body, expected in cases:
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {\n'+body+'\n}')
            call([source_compiler,'limbo','--input',source,fir,'-g',debug],work)
            raw = source.read_bytes()
            for event in json.loads(debug.read_text())['events']:
                if event['kind'].startswith('while-'):
                    span=raw[event['byte_offset']:event['byte_offset']+event['byte_length']]
                    if span not in (b'while',b'for'):
                        raise RuntimeError('loop event lost authored keyword span')
            call([compiler,'check',fir],work)
            call([compiler,'opt',fir,optimized],work)
            for selected in (fir,optimized):
                if reference:
                    result=call([reference,selected],work)
                    if result.stdout.strip()!=str(expected):
                        raise RuntimeError('loop reference mismatch: '+body+'\n'+result.stdout)
                call([compiler,'dis',selected,image],work)
                if runtime:
                    for quantum in (1,2048):
                        result=call([runtime,mode,'--quantum',quantum,'--stats','--inspect-words',0,4 if 'if (' in body else 2,image],work)
                        values=[int(x) for x in re.findall(r'^DATA \d+ (-?\d+)$',result.stdout,re.M)]
                        wanted=[expected,0,1,0] if 'if (' in body else [expected,1]
                        if values!=wanted or 'tables_live=0 keys_live=0' not in result.stdout+result.stderr:
                            raise RuntimeError('native loop result/root mismatch: '+body+'\n'+result.stdout+result.stderr)

        # Loop-carried reference rebindings need future typed cells; reject them now.
        invalid=[
            ('t := table of big; while (len t < 1) {t = table of big {42};} return 42;', 'loop-carried'),
            ('x: big; i := big 0; while (i < 1) {x = 42; i += 1;} return x;', 'loop-carried'),
            ('t := table of big; for (k,k in t) {} return 42;', 'distinct'),
            ('t := table of big; for (k,v in t) {} return v;', 'initialized'),
            ('for (k,v in "abc") {} return 42;', 'requires table'),
        ]
        for body, message in invalid:
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {\n'+body+'\n}')
            fir.write_bytes(b'KEEP')
            result=call([source_compiler,'limbo','--input',source,fir],work,False)
            if message not in result.stderr or fir.read_bytes()!=b'KEEP':
                raise RuntimeError('loop diagnostic/publication mismatch: '+result.stderr)

        # A key snapshot is independent, but payload reads remain live and checked.
        source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                          't := table of big {10,11}; for (k,v in t) {delete t[1];} return 42;}')
        call([source_compiler,'limbo','--input',source,fir],work)
        if reference:
            result=call([reference,fir],work,False)
            if 'missing key' not in result.stderr or result.stdout:
                raise RuntimeError('removed snapshot key was silently read')
        call([compiler,'dis',fir,image],work)
        if runtime:
            result=call([runtime,mode,'--quantum',1,image],work,False)
            if 'missing key' not in result.stderr:
                raise RuntimeError('native removed snapshot key was silently read')

        # Cell capabilities cannot be forged, copied or populated with int cells.
        for body in ('%s: i64.slot = big.local\n%c: i64.slot = copy %s',
                     '%s: i64.slot = big.local\n%i: i32 = const.i32 1\nbig.store %s, %i',
                     '%t: table.big = table.new\n%v: i64 = big.load %t',
                     '%s: i64 = big.local'):
            fir.write_text('FIR 1.0\nmodule @BadCell\nfunc @main() -> void {\n^entry:\n'+body+'\nreturn\n}\n')
            image.write_bytes(b'KEEP')
            call([compiler,'dis',fir,image],work,False)
            if image.read_bytes()!=b'KEEP':
                raise RuntimeError('invalid big cell published bytecode')

        # Infinite scalar loops and repeated owner creation have explicit budgets.
        for body in ('i := big 0; while (i == 0) {i += 0;} return 42;',
                     'i := big 0; while (i == 0) {t := table of big;} return 42;'):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'+body+'}')
            call([source_compiler,'limbo','--input',source,fir],work)
            if reference:
                result=call([reference,fir],work,False)
                if 'budget' not in result.stderr or result.stdout:
                    raise RuntimeError('unbounded reference loop')
            call([compiler,'dis',fir,image],work)
            if runtime:
                result=call([runtime,mode,'--max-instructions',10000,image],work,False)
                if not any(message in result.stderr for message in ('execution budget exceeded','instruction limit exceeded')):
                    raise RuntimeError('unbounded native loop: '+body+'\n'+result.stdout+result.stderr)
    print('PASS source loops: mutable big locals, while, snapshot for-in, nesting, GC and budgets')


if __name__=='__main__':
    main()
