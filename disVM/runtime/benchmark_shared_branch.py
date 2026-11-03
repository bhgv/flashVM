"""Whole-profile cost of eighteen typed branches with stable arrays outside loop."""
from benchmark_guarded_copies import main
from test_shared_branch import GUEST


if __name__ == '__main__':
    # Retain dynamic values/comparisons while excluding per-iteration allocation.
    before = '''    for (i := 0; i < 8; i++) {
        b := array[2] of byte;
        w := array[2] of int;
        l := array[2] of big;'''
    after = '''    b := array[2] of byte;
    w := array[2] of int;
    l := array[2] of big;
    for (i := 0; i < 20000; i++) {'''
    if GUEST.count(before) != 1 or GUEST.count('score != 72') != 1:
        raise RuntimeError('branch benchmark guest drift')
    guest = GUEST.replace(before, after).replace('score != 72', 'score != 180000')
    main(guest, 'PASS branches18')
