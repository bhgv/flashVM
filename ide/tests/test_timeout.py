from support import BuildCase


# Group related build scenarios with independent temporary ownership.
class TestTimeout(BuildCase):
    # Verify timeout stops and cleans without publishing an invalid build.
    def test_timeout_stops_and_cleans(self):
        for stage in ["source", "verify", "optimise", "object", "link", "run"]:
            with self.subTest(stage=stage):
                self.configure(sleep=stage)
                self.rejected(self.build(timeout=1, runner=self.tools[3]), stage)
