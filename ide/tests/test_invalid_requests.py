from support import BuildCase


# Group related build scenarios with independent temporary ownership.
class TestInvalidrequests(BuildCase):
    # Verify invalid requests without publishing an invalid build.
    def test_invalid_requests(self):
        for kwargs in [
            dict(language="unknown"),
            dict(module="../bad"),
            dict(module=""),
            dict(timeout=0),
            dict(timeout=float("nan")),
            dict(timeout=float("inf")),
            dict(source=self.root / "absent"),
            dict(compiler=self.root / "absent"),
            dict(output=self.root / "absent" / "movie.swf"),
        ]:
            with self.subTest(kwargs=kwargs):
                self.rejected(self.build(**kwargs), "input")

    # Verify source limit without publishing an invalid build.
    def test_source_limit(self):
        self.source.write_bytes(b" " * 65537)
        self.rejected(self.build(), "input")
