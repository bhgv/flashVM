#!/usr/bin/env python3
"""Read actual VM fault ZIPs independently, checking CRC, JSON and source/code sections."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile

with tempfile.TemporaryDirectory(prefix='flashvm-crash-') as directory:
    root = Path(directory)
    archive = root / 'report.zip'
    subprocess.run([str(Path(sys.argv[1]).resolve()), str(archive)], cwd=root, check=True, timeout=60)
    with zipfile.ZipFile(archive) as report:
        assert report.testzip() is None
        assert json.loads(report.read('manifest.json'))['kind'] == 'FlashVM VM fault'
        trace = json.loads(report.read('backtrace.json'))
        assert trace['first_chance'] and trace['status'] == -6 and not trace['frames_truncated']
        assert len(trace['frames']) == 2
        assert trace['frames'][1]['function'] == 'outer'
        assert trace['frames'][1]['source'] == 'example.as' and trace['frames'][1]['line'] == 17
        for index, frame in enumerate(trace['frames']):
            assert frame['forced'] and frame['arguments'][0]['value'] == '42'
            assert report.read(f'bytecode/frame{index}.avm1')
            if frame['engine'] == 'jit':
                assert len(report.read(f'jit/frame{index}.bin')) == frame['jit_size']
        heap = json.loads(report.read('heap.json'))
        assert heap['global'] in {obj['id'] for obj in heap['objects']}
        global_object = next(obj for obj in heap['objects'] if obj['id'] == heap['global'])
        assert any(field['name'] == 'Object' for field in global_object['properties'])
        assert all('prototype' in obj and 'properties' in obj for obj in heap['objects'])
print('PASS fault ZIP CRC, JSON, arguments, source, globals/classes and active JIT bytes')
