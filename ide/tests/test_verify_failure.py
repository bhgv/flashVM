from support import BuildCase


# Group related build scenarios with independent temporary ownership.
class TestVerifyfailure(BuildCase):
    # Verify verify rollback without publishing an invalid build.
    def test_verify_rollback(self):
        self.configure(fail="verify")
        result = self.build()
        self.rejected(result, "verify")
        self.assertEqual(len(result["steps"]), 2)
