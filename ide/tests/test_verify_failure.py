from support import BuildCase

class TestVerifyfailure(BuildCase):
    def test_verify_rollback(self):
        self.configure(fail='verify');result=self.build();self.rejected(result,'verify');self.assertEqual(len(result['steps']),2)
