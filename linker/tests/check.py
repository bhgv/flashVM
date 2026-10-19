"""Independent format/linker acceptance; no compiler/Coco-R/VM dependency."""
from pathlib import Path
import os, struct, subprocess, sys, tempfile
EXE=sys.argv[1]
checks=0
def call(*args, good=True):
    global checks
    r=subprocess.run([EXE,*map(str,args)],capture_output=True,timeout=15)
    checks+=1
    assert r.returncode==(0 if good else 1), (args,r.returncode,r.stderr)
    assert b'ERROR: AddressSanitizer' not in r.stderr and b'runtime error:' not in r.stderr
    return r

def object(name,code,flags=0):
    body=name.encode()+code;h=2166136261
    for x in body:h=((h^x)*16777619)&0xffffffff
    return b'SWO1'+struct.pack('<IIIII',1,flags,len(name),len(code),h)+body

def trace(text):
    data=b'\0'+text.encode()+b'\0'
    return b'\x96'+struct.pack('<H',len(data))+data+b'\x26\0'

def tags(data):
    assert data[:4]==b'FWS\x08' and struct.unpack_from('<I',data,4)[0]==len(data)
    n=data[8]>>3;pos=8+(5+4*n+7)//8+4;result=[]
    while pos<len(data):
        record=struct.unpack_from('<H',data,pos)[0];pos+=2;n=record&63
        if n==63:n=struct.unpack_from('<I',data,pos)[0];pos+=4
        assert n<=len(data)-pos
        result.append((record>>6,data[pos:pos+n]));pos+=n
    assert result[-2:]==[(1,b''),(0,b'')]
    return result

with tempfile.TemporaryDirectory() as tmp:
    p=Path(tmp);a=p/'a.swo';b=p/'b.swo';out=p/'out.swf';lib=p/'lib.swa'
    a.write_bytes(object('alpha',trace('first')));b.write_bytes(object('beta',trace('second')))
    call('link',out,a,b)
    assert [body for tag,body in tags(out.read_bytes()) if tag==12]==[trace('first'),trace('second')]
    expected=out.read_bytes()
    call('archive',lib,a,b);call('link',out,lib);assert out.read_bytes()==expected
    call('link',out,b,a)
    assert [body for tag,body in tags(out.read_bytes()) if tag==12]==[trace('second'),trace('first')]
    for argv in [('link',out,a,a),('link',out,a,lib),('archive',lib,a,a)]:
        target=Path(argv[1]);target.write_bytes(b'preserve');call(*argv,good=False);assert target.read_bytes()==b'preserve'
    malformed=[b'',b'SWO1',object('a',b'\0\0'),object('a',b'\x99\x02\0\xfd\xff\0'),
        object('a',b'\x96\x04\0\0bad\0'),object('a',b'\x96\xff\xff\0'),
        object('a',b'\xff\0\0\0'),object('a',b'\0',flags=2),object('bad-name',b'\0'),
        object('a',trace('ok'))+b'trailing', b'SWA1'+struct.pack('<II',1,1),
        b'SWA1'+struct.pack('<II',1,0),b'SWA1'+struct.pack('<III',1,1,0xffffffff)]
    valid=object('a',trace('ok'))
    malformed += [valid[:i] for i in range(len(valid))]
    malformed += [valid[:20]+bytes([valid[20]^1])+valid[21:]]
    for data in malformed:
        a.write_bytes(data);out.write_bytes(b'preserve');call('link',out,a,good=False);assert out.read_bytes()==b'preserve'
    a.write_bytes(object('channels',trace('extension'),1));out.write_bytes(b'preserve')
    call('link',out,a,good=False);assert out.read_bytes()==b'preserve'
    call('link',out,'--extensions',a);tags(out.read_bytes())
    if os.environ.get('FIR_FLASHVM_RUNNER'):
        a.write_bytes(object('alpha',trace('first')));b.write_bytes(object('beta',trace('second')))
        call('link',out,a,b)
        run=subprocess.run([os.environ['FIR_FLASHVM_RUNNER'],str(out)],capture_output=True,check=True)
        assert run.stdout==b'first\nsecond\n'
print(f'{checks} linker subprocess checks')
