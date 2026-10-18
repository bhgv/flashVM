#!/usr/bin/env python3
"""Run the real multi-VM tutorial and verify strict native counters and CLI rejection."""
import re
import subprocess
import sys


def run(executable, *arguments):
    """Capture one independently owned tutorial process with a bounded execution time."""
    return subprocess.run([executable] + list(arguments), capture_output=True,
                          text=True, timeout=20)


def counters(output):
    """Read the documented two-VM and survivor evidence, rejecting extra/missing results."""
    initial = re.findall(r'^VM ([01]): result=42 native=(\d+) backend=\S+$', output, re.M)
    survivor = re.findall(r'^survivor VM 1: result=42 native=(\d+)$', output, re.M)
    if len(initial) != 2 or [item[0] for item in initial] != ['0', '1'] or len(survivor) != 1:
        raise RuntimeError('Missing/invalid tutorial result evidence: ' + output)
    return int(initial[0][1]), int(initial[1][1]), int(survivor[0])


def main():
    """Do not count interpreter fallback as a successful native-only tutorial run."""
    executable = sys.argv[1]
    default = run(executable)
    if default.returncode:
        raise RuntimeError(default.stdout + default.stderr)
    first, second, survivor = counters(default.stdout)
    if first or survivor < second or (second > 0 and survivor <= second):
        raise RuntimeError('Default policy/survivor counters are invalid')
    invalid = run(executable, '--wrong')
    if invalid.returncode != 1 or invalid.stdout or invalid.stderr != 'usage: jit_instances [--both-jit]\n':
        raise RuntimeError('Invalid option was not rejected before VM execution')
    strict = run(executable, '--both-jit')
    if strict.returncode:
        if strict.returncode == 1 and (
            'Both-JIT mode requires an available native backend' in strict.stderr or
            'Multiple-VM JIT example failed' in strict.stderr and
            re.fullmatch(r'VM 0: result=42 native=0 backend=\S+\n', strict.stdout)
        ):
            print('SKIP strict native execution unavailable; default/invalid CLI verified')
            return 77
        raise RuntimeError(strict.stdout + strict.stderr)
    first, second, survivor = counters(strict.stdout)
    if first < 1 or second < 1 or survivor <= second:
        raise RuntimeError('Strict mode did not prove both native owners and a fresh survivor entry')
    print('PASS multi-VM tutorial: default, strict native, survivor and invalid option')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
