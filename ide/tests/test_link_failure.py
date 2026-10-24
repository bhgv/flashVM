from support import BuildCase


# Group related build scenarios with independent temporary ownership.
class TestLinkfailure(BuildCase):
    # Verify link failure without publishing an invalid build.
    def test_link_failure(self):
        self.configure(fail="link")
        result = self.build()
        self.rejected(result, "link")
        self.assertEqual(len(result["steps"]), 6)
