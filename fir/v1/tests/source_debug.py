"""Exercise real source debug emission, SSA origins, scopes and failed-output retention."""
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


# Every generated unoptimized SSA definition must have an authored origin event.
def main():
    compiler, ir = sys.argv[1:]
    body = ('implement Debug; Debug: module { value: fn(): int; };\n'
            'value(): int { # 中\n x:=7; { x:=-(x+1); } { y:=1; x+=y; } return x; }\n')
    with tempfile.TemporaryDirectory(prefix='fir-debug-') as directory:
        root = Path(directory)
        inp, output, debug = root / 'debug.b', root / 'debug.fir', root / 'debug.json'
        inp.write_text(body, encoding='utf-8')
        cmd = [compiler, 'limbo', '--input', str(inp), str(output), '-g', str(debug)]
        subprocess.run(cmd, check=True, timeout=15)
        record = json.loads(debug.read_text())
        assert record['format'] == 'FIRDBG1' and record['stage'] == 'source-unoptimized-fir'
        assert record['source'] == body
        events = record['events']
        values = {int(v) for v in re.findall(r'%v(\d+):', output.read_text())}
        origins = {e['value'] for e in events if e['kind'] in ('constant', 'binary')}
        assert origins == values, (origins, values)
        bindings = [e for e in events if e['kind'] == 'declare' and e['name'] == 'x']
        assert [e['scope'] for e in bindings] == [0, 1], bindings
        entered = [e for e in events if e['kind'] == 'scope-enter']
        assert [e['scope'] for e in entered] == [1, 2]
        assert [e['scope_depth'] for e in entered] == [1, 1]
        assigned = [e for e in events if e['kind'] == 'assign' and e['name'] == 'x']
        assert assigned[-1]['scope'] == 0
        for e in events:
            start, length = e['byte_offset'], e['byte_length']
            assert start + length <= len(body.encode())
            prefix = body.encode()[:start]
            assert e['line'] == prefix.count(b'\n') + 1
            assert e['column'] == len(prefix.rsplit(b'\n', 1)[-1]) + 1
        subprocess.run([ir, 'check', str(output)], check=True, timeout=15)
        # Source failure must preserve both previously delivered artifacts.
        inp.write_text('implement ;')
        output.write_bytes(b'old fir')
        debug.write_bytes(b'old debug')
        result = subprocess.run(cmd + ['--diagnostics-json'], capture_output=True, timeout=15)
        assert result.returncode == 1 and json.loads(result.stderr)['kind'] == 'syntax'
        assert output.read_bytes() == b'old fir' and debug.read_bytes() == b'old debug'
        for extra in (['-g', str(debug)], ['-g']):
            result = subprocess.run(cmd + extra, capture_output=True, timeout=15)
            assert result.returncode == 2
    print('PASS own source compiler FIRDBG1 values, scopes, UTF-8 and rollback')


if __name__ == '__main__':
    main()
