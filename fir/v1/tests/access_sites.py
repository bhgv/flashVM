"""Check exact read/write occurrences, stable binding identities and staged site IDs."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


# Compile repeated names through the real Coco/R grammar; IDs are scoped to the exact debug artifact.
def main():
    compiler = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix='FIR access sites ') as temporary:
        root = Path(temporary)
        source, output, debug = (root / name for name in ('program.as', 'program.fir', 'program.json'))
        text = '// Я😀\nvar o = 2; o = o + 5 * o; o += o;\n'
        source.write_text(text)
        subprocess.run([str(compiler), 'as3', '--input', str(source), str(output), '-g', str(debug)],
                       env=dict(os.environ, TMPDIR=str(root)), check=True, timeout=10)
        record = json.loads(debug.read_text())
        events = record['events']
        if [event['site'] for event in events] != list(range(1, len(events) + 1)):
            raise RuntimeError('debug site identities are missing/reused')
        accesses = [event for event in events if event['kind'] in ('read', 'declare', 'assign')]
        if len([event for event in accesses if event['kind'] == 'read']) != 4:
            raise RuntimeError('explicit and compound implicit reads lack source sites')
        if len({event['binding'] for event in accesses}) != 1 or accesses[0]['binding'] < 1:
            raise RuntimeError('same lexical variable lost binding identity')
        raw = text.encode('utf-8')
        for event in accesses:
            start, size = event['byte_offset'], event['byte_length']
            if raw[start:start + size] != b'o' or event['name'] != 'o':
                raise RuntimeError('access points at the wrong repeated token')
            line_start = raw.rfind(b'\n', 0, start) + 1
            if event['line'] != 2 or event['column'] != start - line_start + 1:
                raise RuntimeError('source positions changed from UTF-8 byte coordinates')
        reads = [event for event in accesses if event['kind'] == 'read']
        if reads[0]['value'] != reads[1]['value'] or reads[0]['site'] == reads[1]['site']:
            raise RuntimeError('SSA identity was confused with the occurrence identity')
        text = ('implement S; S: module {value: fn(): int;}; value(): int {'
                'o := 1; {o := 2; o += 1;} return o;}')
        source.write_text(text)
        subprocess.run([str(compiler), 'limbo', '--input', str(source), str(output), '-g', str(debug)],
                       env=dict(os.environ, TMPDIR=str(root)), check=True, timeout=10)
        events = json.loads(debug.read_text())['events']
        declarations = [event for event in events if event['kind'] == 'declare']
        if len(declarations) != 2 or declarations[0]['binding'] == declarations[1]['binding']:
            raise RuntimeError('shadow bindings share an identity')
        if events[0]['site'] != 1:
            raise RuntimeError('debug site namespace did not restart for the next artifact')
    print('PASS source sites: repeated reads, writes, implicit compound read, UTF-8 spans and shadow bindings')


if __name__ == '__main__':
    main()
