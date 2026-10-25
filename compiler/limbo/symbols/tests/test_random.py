import unittest
from support import prefix, inspect


# Verify random without assuming a complete debugger.
class TestRandom(unittest.TestCase):
    # Verify independent compressed encoder.
    def test_independent_compressed_encoder(self):
        import random

        rng = random.Random(721)
        rows = []
        expected = []
        lastf = lastl = 0
        for pc in range(4096):
            points = []
            text = ""
            for end in range(2):
                file = rng.randrange(3)
                line = rng.randrange(100)
                column = rng.randrange(200)
                if file != lastf:
                    text += str(file) + ":"
                if line != lastl:
                    text += str(line) + "."
                text += str(column) + ("," if not end else " ")
                points.extend([file, line, column])
                lastf, lastl = file, line
            block = rng.randrange(20)
            rows.append(text + str(block))
            expected.append((pc, *points, block))
        self.assertEqual(inspect(prefix(rows, files=["a.b", "b.b", "c.b"]))[2], expected)
