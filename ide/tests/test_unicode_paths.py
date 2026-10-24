from support import BuildCase


# Group related build scenarios with independent temporary ownership.
class TestUnicodepaths(BuildCase):
    # Verify counted paths and no shell without publishing an invalid build.
    def test_counted_paths_and_no_shell(self):
        source = self.root / "исходник ; $(echo nope).as"
        source.write_text("trace(42);")
        output = self.root / "готово с пробелами.swf"
        result = self.build(source=source, output=output, module="foo.bar")
        self.assertTrue(result["ok"], result)
        self.assertTrue(output.exists())
