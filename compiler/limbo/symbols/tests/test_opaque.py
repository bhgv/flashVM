import unittest
from support import prefix, inspect


# Verify opaque without assuming a complete debugger.
class TestOpaque(unittest.TestCase):
    # Verify tail is not claimed as type validation.
    def test_tail_is_not_claimed_as_type_validation(self):
        v, f, e = inspect(prefix(["1,2 0"], tail=b"not a validated type table\n"))
        self.assertEqual(len(e), 1)
