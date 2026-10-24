from support import BuildCase

class TestMissingartifacts(BuildCase):
    def test_missing_and_empty_outputs(self):
        for kind in ['missing','empty']:
            for stage in ['source','optimise','object','link']:
                with self.subTest(kind=kind,stage=stage):
                    self.configure(**{kind:stage});self.rejected(self.build(),stage)
