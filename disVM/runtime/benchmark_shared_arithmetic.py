"""Local whole-profile cost of 15 jointly guarded integer operations."""
from benchmark_guarded_copies import main
from test_shared_arithmetic import GUEST


if __name__ == '__main__':
    # Keep byte arithmetic below wrap, while repeating all fifteen operations.
    guest = GUEST.replace('i < 32', 'i < 20000').replace('(i+64)', '((i & 31)+64)').replace('(i+3)', '((i & 31)+3)')
    main(guest, 'PASS arithmetic15')
