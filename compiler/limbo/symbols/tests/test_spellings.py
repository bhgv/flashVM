import unittest
from support import prefix, inspect


# Verify spellings without assuming a complete debugger.
class TestSpellings(unittest.TestCase):
    # Verify file bytes and spaces are preserved.
    def test_file_bytes_and_spaces_are_preserved(self):
        files = ["  leading space.b", "dir/исходник.b", "tab\tname.b"]
        v, f, e = inspect(prefix(["0:1.0,2:2.0 0"], files=files))
        self.assertEqual(f, files)
