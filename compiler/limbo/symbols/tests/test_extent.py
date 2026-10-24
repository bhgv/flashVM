import unittest
from support import prefix, inspect


# Verify extent without assuming a complete debugger.
class TestExtent(unittest.TestCase):
    # Verify exact prefix boundary.
    def test_exact_prefix_boundary(self):
        for crlf in [False, True]:
            head = prefix(["0,0 0"], tail=b"", crlf=crlf)
            v, f, e = inspect(head + b"tail\n")
            self.assertEqual(int(v["prefix"]), len(head))
