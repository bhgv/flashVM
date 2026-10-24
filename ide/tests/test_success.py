from support import BuildCase


# Group related build scenarios with independent temporary ownership.
class TestSuccess(BuildCase):
    # Verify success and no optimise without publishing an invalid build.
    def test_success_and_no_optimise(self):
        for optimise in (True, False):
            with self.subTest(optimise=optimise):
                result = self.build(optimise=optimise)
                self.assertTrue(result["ok"], result)
                self.assertTrue(result["published"])
                self.assertEqual(self.output.read_bytes()[:4], b"FWS\x08")
                self.assertEqual(len(result["sha256"]), 64)
                self.assertEqual(
                    [s["stage"] for s in result["steps"]],
                    ["source", "verify"]
                    + (["optimise", "verify_optimised"] if optimise else [])
                    + ["object", "link"],
                )
                self.assertFalse(list(self.root.glob(".ide-build-*")))
