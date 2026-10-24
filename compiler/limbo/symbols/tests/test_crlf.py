import unittest
from support import prefix, inspect


# Verify crlf without assuming a complete debugger.
class TestCrlf(unittest.TestCase):
    # Verify crlf matches lf.
    def test_crlf_matches_lf(self):
        rows = ["2.1,9 0", "4,5 1"]
        a = inspect(prefix(rows))
        b = inspect(prefix(rows, crlf=True))
        self.assertEqual(a[1:], b[1:])
        self.assertGreater(int(b[0]["prefix"]), int(a[0]["prefix"]))
