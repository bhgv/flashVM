#!/usr/bin/env python3
"""Read the exact owner allocator blob and check its pool leader against native headers."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


# Read committed bytes without trusting dirty files or updating the checkout.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inferno-source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--ref', help='Explicit owner commit; default selected Dis snapshot')
    args = parser.parse_args()
    manifest = json.loads((ROOT / 'disVM/UPSTREAM.json').read_text())
    revision = subprocess.check_output(['git', '-C', str(args.inferno_source), 'rev-parse',
                                        '--verify', '--end-of-options', (args.ref or manifest['revision']) + '^{commit}'],
                                       universal_newlines=True).strip()
    blob = subprocess.check_output(['git', '-C', str(args.inferno_source), 'show',
                                    revision + ':emu/port/alloc.c'])
    match = re.search(rb'\{\s*"heap"\s*,\s*1\s*,\s*32\*1024\*1024\s*,\s*(\d+)\s*,', blob)
    if match:
        quantum = match.group(1).decode()
    else:
        expression = re.search(rb'PoolQuantum\s*=\s*(.*?)(?:\n\};)', blob, re.DOTALL)
        if not expression:
            raise ValueError('Allocator initializer changed; review quantum/layout manually')
        quantum = '((' + ' '.join(expression.group(1).decode().split()) + ') - 1)' 
    upstream = ROOT / 'disVM/upstream'
    with tempfile.TemporaryDirectory(prefix='gc-layout-') as directory:
        binary = Path(directory) / 'layout'
        command = [os.environ.get('CC', 'cc'), '-O2', '-DGC_POOL_HEAP_QUANTA=' + str(quantum),
                   '-I' + str(ROOT / 'disVM/host/include')]
        command += ['-I' + str(upstream / include) for include in ['include', 'libinterp']]
        command += [str(Path(__file__).with_name('inferno_layout.c')), '-o', str(binary)]
        subprocess.run(command, check=True)
        data = json.loads(subprocess.check_output([str(binary)], universal_newlines=True))
    data.update(repository=manifest['repository'], revision=revision, header_revision=manifest['revision'],
                allocator_sha256=hashlib.sha256(blob).hexdigest(), heap_quanta_expression=quantum,
                scope='Native layout gate only; not a GC/allocator throughput benchmark')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(data, indent=2) + '\n')
    print(json.dumps(data, indent=2))


if __name__ == '__main__':
    main()
