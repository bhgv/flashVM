"""Verify dynamic AS operations through FIR parsing, effects, SWO and target rejection."""
from pathlib import Path
import subprocess
import sys
import tempfile


# Run an explicit compiler tool; return stdout/stderr and fail on an unexpected status.
# Input/output paths are caller-owned; no environment-specific SDK lookup occurs here.
def command(arguments, success=True):
    result = subprocess.run([str(value) for value in arguments], capture_output=True, text=True)
    if (result.returncode == 0) != success:
        raise RuntimeError('unexpected compiler result: ' + result.stdout + result.stderr)
    return result


# Check type/arity and effect order independently of source-language grammar emission.
def main():
    compiler, optimizer = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix='fir-as-properties-') as temporary:
        work = Path(temporary)
        source = work / 'view.fir'
        optimized = work / 'view.opt.fir'
        body = ('%owner: any = as.global_get "owner"\n'
                '%value: any = as.global_get "value"\n'
                'as.member_set %owner, "text", %value\n'
                '%received: any = as.member_call %owner, "receive"\n'
                '%sent: any = as.member_call %owner, "send", %received\n'
                '%first: as.args = as.arg %received\n'
                '%last: as.args = as.arg %value, %first\n'
                '%drawn: any = as.member_call_args %owner, "draw", %last\n'
                '%member: any = as.member_get %owner, %received\n'
                'as.member_put %owner, %last\n'
                '%again: any = as.global_get "owner"\n'
                'as.global_set "captured", %again\n')
        source.write_text('FIR 1.0\nmodule @view\nfunc @main() -> void {\n^entry:\n' + body + 'return\n}\n')
        command([compiler, 'check', source])
        command([optimizer, source, optimized])
        text = optimized.read_text()
        expected = ['as.global_get "owner"', 'as.global_get "value"', 'as.member_set',
                    '%received: any = as.member_call %owner, "receive"',
                    '%sent: any = as.member_call %owner, "send", %received',
                    '%drawn: any = as.member_call_args %owner, "draw", %last',
                    '%member: any = as.member_get %owner, %received',
                    'as.member_put %owner, %last',
                    'as.global_get "owner"', 'as.global_set "captured"']
        offset = 0
        for operation in expected:
            found = text.find(operation, offset)
            if found < 0:
                raise RuntimeError('optimizer removed/reordered an observable AS operation')
            offset = found + len(operation)
        command([compiler, 'check', optimized])
        command([compiler, 'object', optimized, work / 'view.swo'])
        output = work / 'unchanged.dis'
        output.write_bytes(b'previous valid product')
        command([compiler, 'dis', optimized, output], success=False)
        if output.read_bytes() != b'previous valid product':
            raise RuntimeError('unsupported Dis target damaged a previous product')
        frame_header = ('source_language as2\nsource_semantics avm1_numeric\n'
                        'source_entry avm1_frame\n')
        empty_frame = 'func @main() -> void {\n^entry:\nreturn\n}\n'
        source.write_text('FIR 1.0\nmodule @frame\n' + frame_header + empty_frame)
        command([optimizer, source, optimized])
        if frame_header not in optimized.read_text():
            raise RuntimeError('optimizer discarded frame entry metadata')
        command([compiler, 'object', optimized, work / 'frame.swo'])
        command([compiler, 'dis', optimized, output], success=False)
        if output.read_bytes() != b'previous valid product':
            raise RuntimeError('non-AVM1 target overwrote an output for an AVM1 callback')
        source.write_text('FIR 1.0\nmodule @frame\nsource_entry avm1_frame\n' + empty_frame)
        command([compiler, 'check', source], success=False)
        method_header = frame_header.replace('avm1_frame', 'avm1_method @drawItem')
        source.write_text('FIR 1.0\nmodule @frame\n' + method_header + empty_frame)
        command([optimizer, source, optimized])
        if method_header not in optimized.read_text():
            raise RuntimeError('optimizer discarded named root method metadata')
        command([compiler, 'object', optimized, work / 'method.swo'])
        args = '%v: i32 = const.i32 1\n%a0: as.args = as.arg %v\n'
        args += ''.join(f'%a{i}: as.args = as.arg %v, %a{i-1}\n' for i in range(1, 9))
        source.write_text('FIR 1.0\nmodule @bad\nfunc @main() -> void {\n^entry:\n' + args + 'return\n}\n')
        command([compiler, 'check', source], success=False)
        for replacement in ('%bad: any = as.global_get "__fir1$x"',
                            '%bad: any = as.global_get ""',
                            '%bad: i32 = as.global_get "owner"',
                            '%bad: any = as.member_call %owner, ""',
                            '%bad: any = as.member_call %value, "method"',
                            '%bad: i32 = as.member_call %owner, "method"',
                            '%bad: any = as.member_call %owner, "method", %channel',
                            '%bad: any = as.member_call_args %owner, "method", %value',
                            '%bad: i32 = as.member_get %owner, %value',
                            '%bad: any = as.member_get %owner, %channel',
                            'as.member_put %owner, %value',
                            '%one: as.args = as.arg %value\nas.member_put %owner, %one',
                            '%one: as.args = as.arg %value\n%two: as.args = as.arg %value, %one\n'
                            '%three: as.args = as.arg %value, %two\nas.member_put %owner, %three',
                            '%bad: as.args = as.arg %channel',
                            '%bad: as.args = as.arg %value, %value',
                            '%bad: bool = as.to_boolean %channel',
                            '%bad: i32 = as.to_boolean %value',
                            'as.member_set %value, "text", %missing',
                            'as.member_set %value, "text", %channel'):
            extra = '%channel: channel = limbo.chan_create\n' if '%channel' in replacement else ''
            requirement = 'requires flashvm.channels\n' if extra else ''
            source.write_text('FIR 1.0\nmodule @bad\n' + requirement +
                              'func @main() -> void {\n^entry:\n%value: i32 = const.i32 1\n'
                              '%owner: any = as.global_get "owner"\n' +
                              extra + replacement + '\nreturn\n}\n')
            command([compiler, 'check', source], success=False)
    print('PASS FIR dynamic AS: effect ordering, canonical roundtrip, typed operands and target admission')


if __name__ == '__main__':
    main()
