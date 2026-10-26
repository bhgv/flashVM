"""Exercise compiler origins → real C broker → BD1 → AS decoder and real Dis channels."""

import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


# Execute exact argv under one private writable directory and preserve diagnostics on unexpected failure.
def call(argv, root):
    result = subprocess.run(list(map(str, argv)), cwd=root, capture_output=True, text=True,
                            env=dict(os.environ, TMPDIR=str(root)), timeout=30)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout


# Compile a source with repeated names, select its actual origin and verify cross-language delivery.
# argv supplies broker producer, our source compiler, Node, bootstrap, runtime and native module headers.
def main():
    producer, source_compiler, node, bootstrap, runtime, headers = (Path(value).resolve() for value in sys.argv[1:])
    project = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='Broker language integration ') as temporary:
        root = Path(temporary)
        source, output, debug = (root / name for name in ('источник😀.as', 'program.fir', 'origins.json'))
        content = '// Я😀\nvar o = 2; o = o + 5 * o;\n'
        source.write_text(content)
        call([source_compiler, 'as3', '--input', source, output, '-g', debug], root)
        data = json.loads(debug.read_text())
        reads = [event for event in data['events'] if event['kind'] == 'read']
        if len(reads) != 2 or reads[0]['site'] == reads[1]['site']:
            raise RuntimeError('compiler did not distinguish repeated variable accesses')
        selected = reads[1]
        frame = call([producer, source, selected['line'], selected['column'], selected['byte_offset'],
                      selected['byte_length'], selected['binding'], hashlib.sha256(content.encode()).hexdigest(),
                      data['module']], root)
        # A diagnostic receiver must preserve arbitrary uint64 correlation IDs beyond AS Number precision.
        frame = frame[:43] + 'ffffffffffffffff' + frame[59:]
        (root / 'diagnostic.bd1').write_text(frame, encoding='ascii')
        decoded = json.loads(call([node, project / 'test_clients.js', root / 'diagnostic.bd1', source], root))
        if int(decoded['origin']['offset'], 16) != selected['byte_offset'] or int(decoded['origin']['column'], 16) != selected['column']:
            raise RuntimeError('AS decoder points to a different occurrence')
        call([bootstrap, '-I', project, '-o', root / 'brokerdiag.dis', project / 'brokerdiag.b'], root)
        call([bootstrap, '-I', project, '-I', headers, '-o', root / 'diagnostic.dis', project / 'examples/diagnostic.b'], root)
        for dispatch in ('--jit', '--no-jit'):
            result = call([runtime, dispatch, root / 'diagnostic.dis'], root)
            expected = ('DIAG 1 ffffffffffffffff ' + format(selected['line'], '08x') + ' '
                        + format(selected['column'], '08x') + ' ' + format(selected['byte_offset'], '016x')
                        + ' ' + str(source) + ' tcp://127.0.0.1:9001\n')
            if expected not in result:
                raise RuntimeError('actual Dis diagnostic channel lost exact source/ID metadata: ' + result)
    print('PASS compiler sites → C broker → portable BD1 → AS and actual Limbo/Dis channels, interpreter/JIT')


if __name__ == '__main__':
    main()
