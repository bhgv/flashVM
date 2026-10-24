import unittest
from support import prefix, inspect


# Verify nul without assuming a complete debugger.
class TestNul(unittest.TestCase):
    # Verify nul is never hidden eof.
    def test_nul_is_never_hidden_eof(self):
        data = prefix(["1,2 0"])
        for at in [0, 5, 20, len(data)]:
            inspect(data[:at] + b"\0" + data[at:], False)
