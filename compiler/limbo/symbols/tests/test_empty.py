import unittest
from support import prefix, inspect


# Verify empty without assuming a complete debugger.
class TestEmpty(unittest.TestCase):
    # Verify empty pc and file tables.
    def test_empty_pc_and_file_tables(self):
        v, f, e = inspect(prefix([], files=[]))
        self.assertEqual(v["pc"], "0")
        self.assertEqual(f, [])
        self.assertEqual(e, [])
        v, f, e = inspect(prefix([], files=["main.b"]))
        self.assertEqual(f, ["main.b"])
