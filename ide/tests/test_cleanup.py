from support import BuildCase

class TestCleanup(BuildCase):
    def test_launch_error_and_missing_parent(self):
        self.tools[0].chmod(0o644)
        self.rejected(self.build(),'source')
    def test_cancellation_preserves_destination(self):
        from unittest.mock import patch
        from support import driver
        with patch.object(driver,'_run',side_effect=KeyboardInterrupt):self.rejected(self.build(),'source')

    def test_bounded_child_logs(self):
        self.configure(log='source')
        result=self.build();self.assertTrue(result['ok'],result)
        self.assertTrue(result['steps'][0]['stdout_truncated'])
        self.assertEqual(len(result['steps'][0]['stdout']),65536)
    def test_bad_ambient_temp_directory(self):
        import os
        from unittest.mock import patch
        with patch.dict(os.environ,{'TMPDIR':str(self.root/'absent')}):
            result=self.build();self.assertTrue(result['ok'],result)

    def test_publish_failure_is_atomic(self):
        from unittest.mock import patch
        from support import driver
        with patch.object(driver.os,'replace',side_effect=OSError('publication denied')):
            result=self.build();self.rejected(result,'publish');self.assertNotIn('sha256',result)
