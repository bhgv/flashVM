from support import BuildCase

class TestUnicodepaths(BuildCase):
    def test_counted_paths_and_no_shell(self):
        source=self.root/'исходник ; $(echo nope).as';source.write_text('trace(42);')
        output=self.root/'готово с пробелами.swf'
        result=self.build(source=source,output=output,module='foo.bar');self.assertTrue(result['ok'],result);self.assertTrue(output.exists())
