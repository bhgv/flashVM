import unittest
from support import prefix, inspect


# Verify zeros without assuming a complete debugger.
class TestZeros(unittest.TestCase):
    # Verify zero unknown positions and largest dis int.
    def test_zero_unknown_positions_and_largest_dis_int(self):
        v, f, e = inspect(
            prefix(["0,0 0", "2147483647.2147483647,2147483647 2147483647"])
        )
        self.assertEqual(e[0], (0, 0, 0, 0, 0, 0, 0, 0))
        self.assertEqual(e[1][-1], 2147483647)
