#!/usr/bin/env python3
"""Measure one child only, so ru_maxrss is not polluted by earlier build children."""
import json
import subprocess
import sys
import time


# Peak RSS is process-wide, not an exact count of live managed-heap bytes.
def main():
    start = time.perf_counter()
    result = subprocess.run(sys.argv[1:], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            universal_newlines=True, timeout=120)
    elapsed = time.perf_counter() - start
    if result.returncode:
        raise RuntimeError(result.stderr or result.stdout)
    data = json.loads(result.stdout)
    data['process_wall_seconds'] = elapsed
    try:
        import resource
        usage = resource.getrusage(resource.RUSAGE_CHILDREN)
        data['peak_rss_bytes'] = int(usage.ru_maxrss * (1 if sys.platform == 'darwin' else 1024))
    except (ImportError, AttributeError):
        data['peak_rss_bytes'] = None
    print(json.dumps(data, sort_keys=True))


if __name__ == '__main__':
    main()
