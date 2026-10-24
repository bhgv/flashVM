import unittest
from support import prefix, inspect


# Verify coordinates without assuming a complete debugger.
class TestCoordinates(unittest.TestCase):
    # Verify malformed coordinates.
    def test_malformed_coordinates(self):
        for row in [
            "1.2 3 0",
            "1.2,3",
            "1.2,3 0x",
            "1:2.3,4 0",
            "-1,2 0",
            "2147483648,2 0",
            "1,2 -1",
            "1,2 0 ",
            "1:2:3,4 0",
            "1,2.3.4 0",
        ]:
            with self.subTest(row=row):
                inspect(prefix([row]), False)
