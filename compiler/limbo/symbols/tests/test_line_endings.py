import unittest
from support import prefix, inspect


# Verify line endings without assuming a complete debugger.
class TestLineendings(unittest.TestCase):
    # Verify bare cr and missing final terminator.
    def test_bare_cr_and_missing_final_terminator(self):
        inspect(prefix(["1,2 0"], tail=b"").replace(b"\n", b"\r"), False)
        inspect(prefix(["1,2 0"], tail=b"")[:-1], False)
