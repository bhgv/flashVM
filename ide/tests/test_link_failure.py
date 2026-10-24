from support import BuildCase

class TestLinkfailure(BuildCase):
    def test_link_failure(self):
        self.configure(fail='link');result=self.build();self.rejected(result,'link');self.assertEqual(len(result['steps']),6)
