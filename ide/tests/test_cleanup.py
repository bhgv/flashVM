from support import BuildCase


# Group related build scenarios with independent temporary ownership.
class TestCleanup(BuildCase):
    # Verify launch error and missing parent without publishing an invalid build.
    def test_launch_error_and_missing_parent(self):
        self.tools[0].chmod(0o644)
        self.rejected(self.build(), "source")

    # Verify cancellation preserves destination without publishing an invalid build.
    def test_cancellation_preserves_destination(self):
        from unittest.mock import patch
        from support import driver

        with patch.object(driver, "_run", side_effect=KeyboardInterrupt):
            self.rejected(self.build(), "source")

    # Verify bounded child logs without publishing an invalid build.
    def test_bounded_child_logs(self):
        self.configure(log="source")
        result = self.build()
        self.assertTrue(result["ok"], result)
        self.assertTrue(result["steps"][0]["stdout_truncated"])
        self.assertEqual(len(result["steps"][0]["stdout"]), 65536)

    # Verify bad ambient temp directory without publishing an invalid build.
    def test_bad_ambient_temp_directory(self):
        import os
        from unittest.mock import patch

        with patch.dict(os.environ, {"TMPDIR": str(self.root / "absent")}):
            result = self.build()
            self.assertTrue(result["ok"], result)

    # Verify publish failure is atomic without publishing an invalid build.
    def test_publish_failure_is_atomic(self):
        from unittest.mock import patch
        from support import driver

        with patch.object(
            driver.os, "replace", side_effect=OSError("publication denied")
        ):
            result = self.build()
            self.rejected(result, "publish")
            self.assertNotIn("sha256", result)
