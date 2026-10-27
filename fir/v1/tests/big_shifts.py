"""Verify typed int casts and checked 64-bit shifts through Coco/R, FIR, reference and optional DisVM."""
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile


# Run an explicit tool in a private workspace; retain complete diagnostic output on a mismatch.
def call(args, work, success=True):
    result = subprocess.run(list(map(str, args)), cwd=work, capture_output=True, text=True,
                            env=dict(os.environ, TMPDIR=str(work)), timeout=40)
    if (result.returncode == 0) != success:
        raise RuntimeError(str(args)+'\n'+result.stdout+result.stderr)
    return result


# Interpret arbitrary integer bits without relying on a host signed shift or floating conversion.
def signed(value, width):
    return (value+(1 << (width-1))) % (1 << width) - (1 << (width-1))


# Decode nonnegative operands from compiler-generated Dis for actual opcode inspection.
def operand(data, at):
    first = data[at]
    at += 1
    width = 3 if first & 192 == 192 else 1 if first & 128 else 0
    value = first & (63 if width else 255)
    for _ in range(width):
        value = (value << 8) | data[at]
        at += 1
    return value, at


# Read instruction tags with the real variable-width file addressing forms, not byte substring guesses.
def opcodes(image):
    data = image.read_bytes()
    at = 0
    header = []
    for _ in range(9):
        value, at = operand(data, at)
        header.append(value)
    code = []
    for _ in range(header[3]):
        code.append(data[at])
        mode = data[at+1]
        at += 2
        if mode & 192:
            _, at = operand(data, at)
        for kind in ((mode >> 3) & 7, mode & 7):
            if kind != 3:
                _, at = operand(data, at)
                if kind & 4:
                    _, at = operand(data, at)
    return code


# argv supplies source compiler, FIR compiler, reference runner (or '-'), optionally VM and dispatch.
# Constant and runtime-loaded operands are compared to independent Python integer oracles.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = 'jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else 'no-jit'
    with tempfile.TemporaryDirectory(prefix='big shifts ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image, debug = (work/name for name in ('source.b','input.fir','opt.fir','image.dis','debug.json'))

        # Check selected traces and static execution flags before/after optimization.
        def execute(expected, required=()):
            call([compiler, 'opt', fir, optimized], work)
            for path in (fir, optimized):
                if reference:
                    result = call([reference, path], work)
                    if result.stdout != ''.join(str(value)+'\n' for value in expected):
                        raise RuntimeError('reference shift/cast result differs')
                call([compiler, 'dis', path, image], work)
                if required and not set(required) <= set(opcodes(image)):
                    raise RuntimeError('dynamic shifts did not emit existing SHLL/SHRL opcodes')
                if runtime:
                    traces = len(re.findall(r'^\s*trace ', path.read_text(), re.MULTILINE))
                    result = call([runtime, '--'+mode, '--quantum', 1, '--stats', '--inspect-words', 0, 2*traces, image], work)
                    rows = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.MULTILINE)]
                    flags = rows[traces:]
                    stats = re.search(r'instructions=(\d+) native=(\d+)', result.stderr)
                    if len(rows) != 2*traces or any(flag not in (0,1) for flag in flags):
                        raise RuntimeError('shift capture extent/flags differ')
                    if [value for value,flag in zip(rows[:traces],flags) if flag] != expected:
                        raise RuntimeError('actual Dis shift/cast differs from independent oracle')
                    if any(value for value,flag in zip(rows[:traces],flags) if not flag):
                        raise RuntimeError('unselected shift path published a value')
                    if not stats or (int(stats[2]) > 0) != (mode == 'jit'):
                        raise RuntimeError('shift used the wrong dispatch mode')

        cases = [('return 1 << int 0;',1), ('return 1 << int 63;',-(1 << 63)),
                 ('return -9223372036854775808 >> int 63;',-1), ('return -1 << int 1;',-2),
                 ('return 1 + 2 << int 3;',24), ('return (8 >> int 1) | 1;',5),
                 ('return 32 >> int 1 >> int 2;',4), ('return big (int 4294967295);',-1),
                 ('return big (int 2147483648);',-(1 << 31)), ('return big (int -9223372036854775808);',0),
                 ('return big (int 4294967296);',0), ('n: int = int 63; return (-1 << n) >> n;',-1),
                 ('n: int; n = int 63; return 1 << n;',-(1 << 63)),
                 ('t := table of big; n := int 2147483648; t[string n] = big 42; return t["-2147483648"];',42),
                 ('t := table of big; t["int"] = big 42; return t[typeof (int 1)];',42),
                 ('t := table of big; t.x = big -1; n := int t.x; return big n;',-1),
                 ('if(0 == 0){return 42;}else{return 1 << int -1;}',42)]
        for body, expected in cases:
            text = 'implement S; S: module {value: fn(): big;}; value(): big {'+body+'}'
            source.write_text(text)
            call([source_compiler,'limbo','--input',source,fir,'-g',debug],work)
            execute([expected])
            if '<<' in body or '>>' in body:
                events = [event for event in json.loads(debug.read_text())['events']
                          if event.get('name') in ('big.shl','big.shr')]
                if len(events) != body.count('<<') + body.count('>>'):
                    raise RuntimeError('shift source occurrence is missing')
                # The event spelling lives in the operation's name field in FIRDBG1.
                for event in events:
                    span = text.encode()[event['byte_offset']:event['byte_offset']+event['byte_length']]
                    if span not in (b'<<',b'>>'):
                        raise RuntimeError('shift origin lost its exact two-byte operator')
            for event in json.loads(debug.read_text())['events']:
                if event.get('name') == 'value.int':
                    span = text.encode()[event['byte_offset']:event['byte_offset']+event['byte_length']]
                    if span != b'int':
                        raise RuntimeError('int cast origin lost its authored prefix')
        # Runtime-loaded operands prevent a constant-only success. Every valid count is tested.
        body = ['^entry:','%t: table.big = table.new','%key: i64 = const.i64 0',
                '%slot: table.slot = table.index %t, %key']
        expected = []
        values = [-(1 << 63), (1 << 63)-1, -1, 0, 9007199254740993, -9007199254740993]
        for index,value in enumerate(values):
            body += [f'%v{index}: i64 = const.i64 {value}', f'table.store %slot, %v{index}',
                     f'%a{index}: i64 = table.load %slot']
            for count in range(64):
                prefix = f'{index}_{count}'
                body += [f'%n{prefix}: i32 = const.i32 {count}',
                         f'%c{prefix}: i32 = copy %n{prefix}',
                         f'%l{prefix}: i64 = big.shl %a{index}, %c{prefix}',
                         f'%r{prefix}: i64 = big.shr %a{index}, %c{prefix}',
                         f'trace %l{prefix}',f'trace %r{prefix}']
                expected += [signed(value << count,64), value >> count]
        body += ['return']
        fir.write_text('FIR 1.0\nmodule @ShiftMatrix\nfunc @main() -> void {\n'+'\n'.join(body)+'\n}\n')
        execute(expected, (127,128))
        # Counts outside 0..63 fail even when their result is unused after opt.
        for count in (-1,64,65,2147483647,-2147483648):
            for shift in ('<<','>>'):
                source.write_text('implement S; S: module {value: fn(): big;}; value(): big {'
                                  f'unused := 1 {shift} int {count}; return 42;'+ '}')
                call([source_compiler,'limbo','--input',source,fir],work)
                call([compiler,'opt',fir,optimized],work)
                for path in (fir,optimized):
                    if reference:
                        failed = call([reference,path],work,False)
                        if failed.stdout or 'invalid argument' not in failed.stderr:
                            raise RuntimeError('reference lost unused invalid-shift error')
                    call([compiler,'dis',path,image],work)
                    if runtime:
                        failed = call([runtime,'--'+mode,'--inspect-words',0,2,image],work,False)
                        if 'DATA ' in failed.stdout or 'invalid argument' not in failed.stderr:
                            raise RuntimeError('VM lost unused invalid-shift error')
        # Public FIR must enforce the mixed operand contract independently of source lowering.
        prior_image = image.read_bytes()
        for kind, operation, result_type in [('i64','big.shl','i64'), ('i32','big.shr','i32')]:
            fir.write_text('FIR 1.0\nmodule @WrongShift\nfunc @main() -> void {\n^entry:\n'
                           '%a: i64 = const.i64 1\n%n: '+kind+' = const.'+kind+' 1\n'
                           '%r: '+result_type+' = '+operation+' %a, %n\nreturn\n}\n')
            call([compiler,'check',fir],work,False)
            call([compiler,'dis',fir,image],work,False)
            if image.read_bytes() != prior_image:
                raise RuntimeError('rejected public FIR shift changed the output image')
        # Reject wrong static kinds before publication, including an implicit big count.
        for bad in ['return 1 << 1;', 'return (int 1) << 1;', 'return int (table of big);',
                    't := table of big; t.x = int 1; return t.x;', 'return int 1;']:
            source.write_text('implement S; S: module {value: fn(): big;}; value(): big {'+bad+'}')
            fir.write_bytes(b'prior FIR')
            call([source_compiler,'limbo','--input',source,fir],work,False)
            if fir.read_bytes() != b'prior FIR':
                raise RuntimeError('invalid shift/type changed published output')
    print('PASS big shifts/int casts: all 64 counts, signed extrema, native opcodes, reflection, precedence and errors'+
          (', actual '+mode if runtime else ''))


if __name__ == '__main__':
    main()
