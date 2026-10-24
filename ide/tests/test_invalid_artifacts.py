from support import BuildCase


# Group related build scenarios with independent temporary ownership.
class TestInvalidartifacts(BuildCase):
    # Verify signature and declared length without publishing an invalid build.
    def test_signature_and_declared_length(self):
        for stage in ["object", "link"]:
            self.configure(malformed=stage)
            self.rejected(self.build(), stage)

    # Verify declared length mismatch without publishing an invalid build.
    def test_declared_length_mismatch(self):
        self.configure(bad_length=True)
        self.rejected(self.build(), "link")
