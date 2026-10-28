"""Execute the real C module tutorial through generated ABI, channels, GC and exception recovery."""
from pathlib import Path
import re
import subprocess
import sys


# Run both scheduler quanta in the selected actual dispatch mode; never emulate native results.
# CLI supplies runtime/module/mode; failures retain runtime stdout/stderr in the diagnostic.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    mode = sys.argv[3]
    for quantum in (1, 2048):
        result = subprocess.run([str(runtime), '--jit' if mode == 'jit' else '--no-jit',
                                 '--stats', '--quantum', str(quantum), '--max-instructions', '1000000', str(module)],
                                capture_output=True, text=True, timeout=30)
        if result.returncode or 'PASS native module: wrap=ok sum=42 strings=2048 caught=1 shutdown=ok' not in result.stdout:
            raise RuntimeError('native module tutorial failed: '+result.stdout+result.stderr)
        counters = re.search(r'native=(\d+).*gc_runs=(\d+)', result.stderr)
        if not counters or (int(counters[1]) > 0) != (mode == 'jit') or (quantum == 1 and int(counters[2]) == 0):
            raise RuntimeError('native module dispatch/GC evidence missing: '+result.stderr)
    print('PASS native module tutorial: generated ABI, wrapped big, rooted string returns, channels and caught error, '+mode)


if __name__ == '__main__':
    main()
