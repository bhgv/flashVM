import unittest
from support import prefix, inspect


# Verify cuts without assuming a complete debugger.
class TestCuts(unittest.TestCase):
    # Verify every cut before complete prefix is rejected.
    def test_every_cut_before_complete_prefix_is_rejected(self):
        data = prefix(["2.1,3 0"], tail=b"")
        for size in range(len(data)):
            with self.subTest(size=size):
                inspect(data[:size], False)
