from support import BuildCase

class TestInvalidartifacts(BuildCase):
    def test_signature_and_declared_length(self):
        for stage in ['object','link']:
            self.configure(malformed=stage);self.rejected(self.build(),stage)

    def test_declared_length_mismatch(self):
        self.configure(bad_length=True);self.rejected(self.build(),'link')
