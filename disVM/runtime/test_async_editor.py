"""Execute real Dis/AS editor envelopes with background raster and no capture barrier."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from test_graphical_editor import envelope, shortcut


# Run the actual editor with uncaptured replay; verify Unicode edits, saved bytes and dispatch.
# All paths are explicit; temporary data is owned here. No timing threshold or fake raster is used.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix='async native editor ') as temporary:
        directory = Path(temporary)
        source = directory / 'unicode.b'
        source.write_text('seed', encoding='utf-8')
        replay = directory / 'input.bin'
        frames = [envelope(5, text='alpha\nЯ😀')]
        frames += shortcut('S')
        frames += [envelope(1, 65539)]
        replay.write_bytes(b''.join(frames))
        result = subprocess.run(
            [str(runtime), '--stats', '--jit' if mode == 'jit' else '--no-jit',
             '--document', str(source), '--font', str(font), '--events', str(replay), str(module)],
            cwd=directory, capture_output=True, text=True, timeout=45)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        counters = re.search(r'instructions=(\d+) native=(\d+)', result.stderr)
        if not counters or int(counters[1]) == 0 or ((int(counters[2]) > 0) != (mode == 'jit')):
            raise RuntimeError('incorrect actual Dis execution counters')
        expected = 'alpha\nЯ😀seed'
        views = [bytes.fromhex(line.split(' ', 6)[6]).decode('utf-8')
                 for line in result.stdout.splitlines() if line.startswith('VIEW ')]
        if not views or views[0] != 'seed' or views[-1] != expected:
            raise RuntimeError('background editor lost owner-published Unicode views')
        if source.read_text(encoding='utf-8') != expected:
            raise RuntimeError('background editor save did not persist Dis-owned text')
    print('PASS uncaptured asynchronous editor: real Dis/AS, Unicode edit/save/close, ' + mode)


if __name__ == '__main__':
    main()
