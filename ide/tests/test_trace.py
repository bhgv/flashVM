from support import BuildCase

class TestTrace(BuildCase):
    def test_exact_trace_gate(self):
        result=self.build(runner=self.tools[3],expect_trace='wrong\n');self.rejected(result,'run')
        result=self.build(runner=self.tools[3],expect_trace='42\n');self.assertTrue(result['ok'],result)
    def test_trace_requires_runner(self):
        self.rejected(self.build(expect_trace='42\n'),'input')
