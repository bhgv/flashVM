from support import BuildCase


# Group related build scenarios with independent temporary ownership.
class TestAliases(BuildCase):
    # Verify aliases and executable protection without publishing an invalid build.
    def test_aliases_and_executable_protection(self):
        import os

        original = self.source.read_bytes()
        result = self.build(output=self.source)
        self.assertFalse(result["ok"])
        self.assertEqual(self.source.read_bytes(), original)
        alias = self.root / "alias.swf"
        os.link(self.source, alias)
        result = self.build(output=alias)
        self.assertFalse(result["ok"])
        self.assertEqual(self.source.read_bytes(), original)
        result = self.build(output=self.tools[0])
        self.assertFalse(result["ok"])
        self.assertTrue(self.tools[0].read_bytes().startswith(b"#!"))

    # Verify symbolic output rejected without publishing an invalid build.
    def test_symbolic_output_rejected(self):
        alias = self.root / "alias.swf"
        alias.symlink_to(self.output)
        self.assertFalse(self.build(output=alias)["ok"])
        self.assertEqual(self.output.read_bytes(), b"previous artifact")
