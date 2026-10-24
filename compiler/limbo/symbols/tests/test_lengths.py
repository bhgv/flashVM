import unittest
from support import prefix, inspect


# Verify lengths without assuming a complete debugger.
class TestLengths(unittest.TestCase):
    # Verify record and input limits.
    def test_record_and_input_limits(self):
        self.assertIn(b"status 2", inspect(prefix([], module="X" * 1024), False))
        self.assertIn(b"status 2", inspect(prefix([], files=["X" * 1024]), False))
        self.assertIn(b"status 2", inspect(b"x" * (4 * 1024 * 1024 + 1), False))
