from support import BuildCase

class TestTimeout(BuildCase):
    def test_timeout_stops_and_cleans(self):
        for stage in ['source','verify','optimise','object','link','run']:
            with self.subTest(stage=stage):
                self.configure(sleep=stage)
                self.rejected(self.build(timeout=1,runner=self.tools[3]),stage)
