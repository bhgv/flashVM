from support import BuildCase


# Group related build scenarios with independent temporary ownership.
class TestOptimisefailure(BuildCase):
    # Verify optimiser rollback without publishing an invalid build.
    def test_optimiser_rollback(self):
        self.configure(fail="optimise")
        result = self.build()
        self.rejected(result, "optimise")
        self.assertEqual(len(result["steps"]), 3)
