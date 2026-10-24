from support import BuildCase

class TestVerifyoptimisedfailure(BuildCase):
    def test_post_optimisation_verify(self):
        self.configure(fail='verify_optimised');result=self.build();self.rejected(result,'verify_optimised');self.assertEqual(len(result['steps']),4)
