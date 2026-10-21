#!/usr/bin/env python3
"""Independent binary transcript oracle for the documented memory and ordinary-file tutorial."""
from pathlib import Path
import subprocess,sys,tempfile,re
from sbindings_oracle import declaration
from srecord_oracle import frame

def main():
    executable=str(Path(sys.argv[1]).resolve())
    expected='tag=3 type=ieee754.f64.be bytes=8\ntag=3 type=boolean bytes=1\n'
    with tempfile.TemporaryDirectory(prefix='srecord-example-') as temporary:
        directory=Path(temporary);path=directory/'transcript.bin'
        for args in [[],[str(path)]]:
            result=subprocess.run([executable,*args],capture_output=True,text=True,timeout=8)
            assert result.returncode==0 and result.stdout==expected,(result.stdout,result.stderr)
        assert path.read_bytes()==declaration([(3,b'ieee754.f64.be',1)])+frame(3,bytes.fromhex('3ff4000000000000'))+declaration([(3,b'boolean',1)])+frame(3,b'\x81')
        for args,status in [([str(directory)],1),([str(path),'extra'],2)]:
            result=subprocess.run([executable,*args],capture_output=True,text=True,timeout=8)
            assert result.returncode==status,(args,result.stdout,result.stderr)
    root=Path(__file__).resolve().parents[2]
    tutorial=root/'examples/embedding/srecords'
    source=(tutorial/'main.c').read_text()
    for number,line in enumerate(source.splitlines(),1):
        assert not line.strip() or '/*' in line or '*/' in line,number
    for lang in ['', '.ru']:
        manual=(tutorial/('README'+lang+'.md')).read_text()
        for api in set(re.findall(r'\b(fvm_\w+)\s*\(',source)):
            assert re.search(r'\|.*'+api+r'.*\|',manual),(lang,api)
    catalog=(root/'examples/README.md').read_text()
    assert catalog.count('example-embedding-srecords')==3
    entries=[line for line in catalog.split('## Глоссарий',1)[1].splitlines() if line.startswith('- [')]
    assert entries==sorted(entries,key=str.casefold)
    print('PASS native memory/file tutorial, opposite schemas, independent transcript bytes and I/O/argument failures')

if __name__=='__main__':main()
