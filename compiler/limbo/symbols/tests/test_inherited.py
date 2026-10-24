import unittest
from support import prefix, inspect


# Verify inherited without assuming a complete debugger.
class TestInherited(unittest.TestCase):
    # Verify stop state carries to next start.
    def test_stop_state_carries_to_next_start(self):
        v, f, e = inspect(prefix(["2.1,4.9 0", "3,4 1"]))
        self.assertEqual(e[1], (1, 0, 4, 3, 0, 4, 4, 1))
