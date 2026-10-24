import unittest
from support import prefix, inspect


# Verify basic without assuming a complete debugger.
class TestBasic(unittest.TestCase):
    # Verify explicit coordinates.
    def test_explicit_coordinates(self):
        v, f, e = inspect(prefix(["0:7.2,0:8.9 3"]))
        self.assertEqual(e, [(0, 0, 7, 2, 0, 8, 9, 3)])
        self.assertEqual(v["module"], "Example")
        self.assertEqual(f, ["main.b"])
