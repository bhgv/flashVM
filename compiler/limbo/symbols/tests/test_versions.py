import unittest
from support import prefix, inspect


# Verify versions without assuming a complete debugger.
class TestVersions(unittest.TestCase):
    # Verify unknown versions rejected.
    def test_unknown_versions_rejected(self):
        for version in ["1.1", "2.0", "2.2", "3.0"]:
            with self.subTest(version=version):
                self.assertIn(
                    b"status 4",
                    inspect(prefix([]).replace(b"2.1", version.encode()), False),
                )
