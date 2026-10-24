from support import BuildCase


# Group related build scenarios with independent temporary ownership.
class TestVerifyoptimisedfailure(BuildCase):
    # Verify post optimisation verify without publishing an invalid build.
    def test_post_optimisation_verify(self):
        self.configure(fail="verify_optimised")
        result = self.build()
        self.rejected(result, "verify_optimised")
        self.assertEqual(len(result["steps"]), 4)
