from support import BuildCase


# Group related build scenarios with independent temporary ownership.
class TestRunfailure(BuildCase):
    # Verify runner failure without publishing an invalid build.
    def test_runner_failure(self):
        self.configure(fail="run")
        result = self.build(runner=self.tools[3])
        self.rejected(result, "run")
        self.assertEqual(len(result["steps"]), 7)
