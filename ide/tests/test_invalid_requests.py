from support import BuildCase

class TestInvalidrequests(BuildCase):
    def test_invalid_requests(self):
        for kwargs in [dict(language='unknown'),dict(module='../bad'),dict(module=''),dict(timeout=0),dict(timeout=float('nan')),dict(timeout=float('inf')),dict(source=self.root/'absent'),dict(compiler=self.root/'absent'),dict(output=self.root/'absent'/'movie.swf')]:
            with self.subTest(kwargs=kwargs):self.rejected(self.build(**kwargs),'input')
    def test_source_limit(self):
        self.source.write_bytes(b' '*65537);self.rejected(self.build(),'input')
