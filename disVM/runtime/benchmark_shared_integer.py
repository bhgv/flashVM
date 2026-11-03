"""Local whole-profile cost of four Limbo-emitted integer conversions."""
from benchmark_guarded_copies import main
from test_shared_integer import GUEST


if __name__ == '__main__':
    guest = GUEST.replace('i < 64', 'i < 20000').replace('(i+128)', '(i & 255)')
    guest = guest.replace('w != i+128', 'w != (i & 255)')
    main(guest, 'PASS integer conversions64')
