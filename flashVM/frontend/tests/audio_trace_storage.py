"""Check signed little-endian WAV bytes, bounded buffers, opt-out and recoverable save failures."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave

with tempfile.TemporaryDirectory(prefix='flashvm-trace-storage-') as folder:
    base = Path(folder) / 'capture'
    subprocess.run([sys.argv[1], str(base)], check=True)
    with wave.open(str(base) + '.wav') as audio:
        assert (audio.getnframes(), audio.getframerate(), audio.getnchannels(), audio.getsampwidth()) == (160, 8, 2, 2)
        assert audio.readframes(160) == struct.pack('<hh', 0x1234, -2) * 160
    report = Path(str(base) + '.txt').read_text()
    assert 'frames=160 truncated=1 dropped_events=4' in report
    assert not list(Path(folder).glob('*.tmp'))
    # Failure to save diagnostic bytes must neither crash nor create fake successful evidence.
    subprocess.run([sys.argv[1], str(Path(folder) / 'absent' / 'capture')], check=True)
    assert not (Path(folder) / 'absent').exists()
