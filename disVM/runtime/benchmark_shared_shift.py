"""Measure six actual guest shifts; full startup-profile cost, not isolated locks."""
from benchmark_guarded_copies import main
from test_shared_shift import GUEST


if __name__ == '__main__':
    # Bound counts to 0..7 as in the correctness guest; enlarge only repetition.
    main(GUEST.replace('i < 32', 'i < 20000'), 'PASS shifts6')
