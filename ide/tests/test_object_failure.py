from support import BuildCase


# Group related build scenarios with independent temporary ownership.
class TestObjectfailure(BuildCase):
    # Verify object failure without publishing an invalid build.
    def test_object_failure(self):
        self.configure(fail="object")
        result = self.build()
        self.rejected(result, "object")
        self.assertEqual(len(result["steps"]), 5)
