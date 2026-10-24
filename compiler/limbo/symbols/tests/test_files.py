import unittest
from support import prefix, inspect


# Verify files without assuming a complete debugger.
class TestFiles(unittest.TestCase):
    # Verify cross file stop and following start.
    def test_cross_file_stop_and_following_start(self):
        v, f, e = inspect(prefix(["0:2.1,1:4.9 0", "3,0:8.4 1"], files=["a.b", "b.b"]))
        self.assertEqual(e[1], (1, 1, 4, 3, 0, 8, 4, 1))
        self.assertEqual(f, ["a.b", "b.b"])
