from support import BuildCase

class TestOptimisefailure(BuildCase):
    def test_optimiser_rollback(self):
        self.configure(fail='optimise');result=self.build();self.rejected(result,'optimise');self.assertEqual(len(result['steps']),3)
