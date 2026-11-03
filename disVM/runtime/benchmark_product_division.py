"""Local whole guarded-profile cost with all24 integer arithmetic variants."""
from benchmark_guarded_copies import main
from test_shared_arithmetic import GUEST


if __name__ == '__main__':
    guest = GUEST.replace('i < 32', 'i < 20000').replace('(i+64)', '((i & 31)+64)').replace('(i+3)', '((i & 31)+3)')
    main(guest, 'PASS arithmetic24')
