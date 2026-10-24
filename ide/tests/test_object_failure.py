from support import BuildCase

class TestObjectfailure(BuildCase):
    def test_object_failure(self):
        self.configure(fail='object');result=self.build();self.rejected(result,'object');self.assertEqual(len(result['steps']),5)
