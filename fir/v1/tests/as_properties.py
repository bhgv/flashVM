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
                '%again: any = as.global_get "owner"\n'
                'as.global_set "captured", %again\n')
        source.write_text('FIR 1.0\nmodule @view\nfunc @main() -> void {\n^entry:\n' + body + 'return\n}\n')
        command([compiler, 'check', source])
        command([optimizer, source, optimized])
        text = optimized.read_text()
        expected = ['as.global_get "owner"', 'as.global_get "value"', 'as.member_set',
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
        for replacement in ('%bad: any = as.global_get "__fir1$x"',
                            '%bad: any = as.global_get ""',
                            '%bad: i32 = as.global_get "owner"',
                            'as.member_set %value, "text", %missing',
                            'as.member_set %value, "text", %channel'):
            extra = '%channel: channel = limbo.chan_create\n' if '%channel' in replacement else ''
            requirement = 'requires flashvm.channels\n' if extra else ''
            source.write_text('FIR 1.0\nmodule @bad\n' + requirement +
                              'func @main() -> void {\n^entry:\n%value: i32 = const.i32 1\n' +
                              extra + replacement + '\nreturn\n}\n')
            command([compiler, 'check', source], success=False)
    print('PASS FIR dynamic AS: effect ordering, canonical roundtrip, typed operands and target admission')


if __name__ == '__main__':
    main()
