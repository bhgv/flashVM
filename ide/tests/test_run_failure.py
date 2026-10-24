from support import BuildCase

class TestRunfailure(BuildCase):
    def test_runner_failure(self):
        self.configure(fail='run');result=self.build(runner=self.tools[3]);self.rejected(result,'run');self.assertEqual(len(result['steps']),7)
