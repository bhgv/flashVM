import unittest
from support import prefix, inspect


# Verify counts without assuming a complete debugger.
class TestCounts(unittest.TestCase):
    # Verify count syntax and limits.
    def test_count_syntax_and_limits(self):
        for count in ["-1", "+1", "1x", "2147483648", "999999999999999999999999"]:
            data = b"limbo .sbl 2.1\nExample\n" + count.encode() + b"\n"
            inspect(data, False)
        self.assertIn(b"status 2", inspect(b"limbo .sbl 2.1\nExample\n4097\n", False))
        self.assertIn(
            b"status 2", inspect(b"limbo .sbl 2.1\nExample\n0\n262145\n", False)
        )
