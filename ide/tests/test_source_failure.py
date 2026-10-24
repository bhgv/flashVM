from support import BuildCase

class TestSourcefailure(BuildCase):
    def test_syntax_and_semantic_locations(self):
        for error,line,column in [('source:3:7: syntax E2 (byte 9)',3,7),('source line 4: unknown name',4,None),('opaque error',None,None)]:
            self.configure(fail='source',error=error)
            result=self.build();self.rejected(result,'source')
            item=result['diagnostics'][0]
            self.assertEqual(item.get('line'),line)
            self.assertEqual(item.get('column'),column)
            if line:self.assertEqual(item['file'],str(self.source))
            self.assertEqual(len(result['steps']),1)
