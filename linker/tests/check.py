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
        object('a',b'\x96\x03\0\0a\0'),object('a',b'\xff\0\0\0'),object('a',b'\0',flags=2),object('bad-name',b'\0'),
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
    # The object profile admits AVM1 Divide used by FIR f64.div.
    divide=b'\x96\x05\0\x07'+struct.pack('<i',6)+b'\x96\x05\0\x07'+struct.pack('<i',2)+b'\x0d\x26\0'
    a.write_bytes(object('divide',divide));call('link',out,a)
    if os.environ.get('FIR_FLASHVM_RUNNER'):
        run=subprocess.run([os.environ['FIR_FLASHVM_RUNNER'],str(out)],capture_output=True,check=True)
        assert run.stdout==b'3\n'
print(f'{checks} linker subprocess checks')

# Resource imports are checked independently of the linker bit writer.
class Bits:
    def __init__(self,data,bit=0): self.data,self.bit=data,bit
    def read(self,n):
        result=0
        for _ in range(n):
            result=(result<<1)|((self.data[self.bit//8]>>(7-self.bit%8))&1);self.bit+=1
        return result
    def signed(self,n):
        v=self.read(n);return v-(1<<n) if n and v>>(n-1) else v
    def align(self): self.bit=(self.bit+7)//8*8

def rect(data,offset):
    b=Bits(data,offset*8);n=b.read(5);coords=[b.signed(n) for _ in range(4)];b.align()
    return coords,b.bit//8

with tempfile.TemporaryDirectory() as tmp:
    p=Path(tmp);out=p/'scene.swf';svf=p/'shape.svf';wav=p/'tone.wav'
    root=Path(__file__).resolve().parents[1]/'examples'
    call('link',out,'--svf','panel',root/'panel.svf','--jpeg','tile',root/'tile.jpg',
         '--wav','tone',root/'tone.wav','--place','panel','20','30','1',
         '--place','tile','150','30','2','--play','tone')
    records=tags(out.read_bytes());ids=[]
    assert [tag for tag,_ in records]==[32,21,32,14,56,26,26,15,1,0]
    # Independent edge decoding confirms rectangle dimensions and style indices.
    body=records[0][1];ids.append(struct.unpack_from('<H',body)[0]);bounds,pos=rect(body,2)
    assert bounds==[0,2400,0,1600]
    assert body[pos:pos+8]==b'\x01\0\x20\xa0\x60\xff\0\x10'
    b=Bits(body,(pos+8)*8)
    assert b.read(1)==0 and b.read(5)==5
    n=b.read(5);assert [b.signed(n),b.signed(n)]==[0,0] and b.read(1)==1
    edges=[]
    for _ in range(4):
        assert b.read(1)==1 and b.read(1)==1;n=b.read(4)+2
        assert b.read(1)==0
        vertical=b.read(1);delta=b.signed(n);edges.append((vertical,delta))
    assert edges==[(0,2400),(1,1600),(0,-2400),(1,-1600)] and b.read(6)==0
    # JPEG is preserved byte-for-byte; its generated display shape has another ID.
    assert records[1][1][2:]==(root/'tile.jpg').read_bytes()
    assert struct.unpack_from('<H',records[2][1])[0]!=struct.unpack_from('<H',records[1][1])[0]
    image=records[2][1];bounds,pos=rect(image,2)
    assert bounds==[0,160,0,160] and image[pos:pos+2]==b'\x01\x41'
    assert struct.unpack_from('<H',image,pos+2)[0]==struct.unpack_from('<H',records[1][1])[0]
    matrix=Bits(image,(pos+4)*8);assert matrix.read(1)==1;n=matrix.read(5)
    assert [matrix.signed(n),matrix.signed(n)]==[20<<16,20<<16] and matrix.read(1)==0
    n=matrix.read(5);assert [matrix.signed(n),matrix.signed(n)]==[0,0]
    import wave
    with wave.open(str(root/'tone.wav'),'rb') as f:
        samples=f.readframes(f.getnframes());count=f.getnframes()
    sound=records[3][1]
    assert sound[2]==0x36 and struct.unpack_from('<I',sound,3)[0]==count and sound[7:]==samples
    # ExportAssets includes every named input; placement and StartSound IDs resolve.
    exports=records[4][1];assert struct.unpack_from('<H',exports)[0]==3
    cursor=2;symbols={}
    while cursor<len(exports):
        identifier=struct.unpack_from('<H',exports,cursor)[0];cursor+=2
        end=exports.index(0,cursor);symbols[exports[cursor:end].decode()]=identifier;cursor=end+1
    assert set(symbols)=={'panel','tile','tone'} and len(set(symbols.values()))==3
    for index,name,x,y,depth in [(5,'panel',400,600,1),(6,'tile',3000,600,2)]:
        body=records[index][1];assert body[0]==6
        assert struct.unpack_from('<HH',body,1)==(depth,symbols[name])
        b=Bits(body,40);assert b.read(1)==0 and b.read(1)==0;n=b.read(5)
        assert [b.signed(n),b.signed(n)]==[x,y]
    assert records[7][1]==struct.pack('<HB',symbols['tone'],0)
    if os.environ.get('FIR_FLASHVM_RUNNER'):
        subprocess.run([os.environ['FIR_FLASHVM_RUNNER'],str(out)],check=True,capture_output=True)
    for bad in ['SVF 2 rect 1 1 FFFFFFFF','SVF1 rect 1 1 FFFFFFFF','SVF 1 rect -1 1 FFFFFFFF',
                'SVF 1 rect '+ '9'*150 +' 1 FFFFFFFF','SVF 1 rect 1 1 FFFFFFFF extra',
                'SVF 1 rect 1 1 GGGGGGGG','SVF 1 rect 0 1 FFFFFFFF']:
        svf.write_text(bad);out.write_bytes(b'keep');call('link',out,'--svf','shape',svf,good=False);assert out.read_bytes()==b'keep'
    for kind,source in [('jpeg',root/'tile.jpg'),('wav',root/'tone.wav')]:
        data=source.read_bytes()
        for cut in [0,1,4,12,len(data)//2,len(data)-1]:
            wav.write_bytes(data[:cut]);out.write_bytes(b'keep');call('link',out,'--'+kind,'asset',wav,good=False);assert out.read_bytes()==b'keep'
    for rate_code,rate in enumerate((5512,11025,22050,44100)):
        for channels in (1,2):
            for sample_bytes in (1,2):
                pcm=bytes([128 if sample_bytes==1 else 0])*8*channels*sample_bytes
                with wave.open(str(wav),'wb') as f:
                    f.setparams((channels,sample_bytes,rate,0,'NONE','not compressed'));f.writeframes(pcm)
                call('link',out,'--wav','sound',wav)
                body=next(body for tag,body in tags(out.read_bytes()) if tag==14)
                assert body[2]==0x30|(rate_code<<2)|(2 if sample_bytes==2 else 0)|(channels-1)
                assert struct.unpack_from('<I',body,3)[0]==8 and body[7:]==pcm
    out.write_bytes(b'keep');call('link',out,'--svf','panel',root/'panel.svf','--place','missing','0','0','1',good=False);assert out.read_bytes()==b'keep'
print(f'{checks} total linker/resource subprocess checks')

# Inspection uses the same validating core; no output is published on bad input.
with tempfile.TemporaryDirectory() as tmp:
    p = Path(tmp)
    a, b, lib = p/'a.swo', p/'b.swo', p/'library.swa'
    a.write_bytes(object('alpha', b'\0'))
    b.write_bytes(object('beta', trace('extension'), 1))
    assert call('inspect', a).stdout == b'module alpha code 1 requirements 0\n'
    call('archive', lib, a, b)
    expected = (b'module alpha code 1 requirements 0\n' +
                f'module beta code {len(trace("extension"))} requirements 1\n'.encode())
    assert call('inspect', lib).stdout == expected
    data = lib.read_bytes()
    for cut in range(len(data)):
        lib.write_bytes(data[:cut])
        assert call('inspect', lib, good=False).stdout == b''
    lib.write_bytes(data + b'extra')
    assert call('inspect', lib, good=False).stdout == b''
    lib.write_bytes(data[:-1] + b'\1')
    assert call('inspect', lib, good=False).stdout == b''
    assert call('inspect', p/'missing', good=False).stdout == b''
    for args in [('inspect',), ('inspect', a, b)]:
        result = subprocess.run([EXE, *map(str, args)], capture_output=True, timeout=15)
        assert result.returncode == 2 and result.stdout == b''
print(f'{checks} total checks including inspection')

# Stage options affect real SWF header fields, independent of the bit writer.
with tempfile.TemporaryDirectory() as tmp:
    p = Path(tmp); a = p/'a.swo'; out = p/'scene.swf'
    a.write_bytes(object('stage', b'\0'))
    for width, height, fps in [(1, 1, 1), (3276, 3276, 255), (640, 480, 30)]:
        call('link', out, a, '--size', width, height, '--fps', fps)
        data = out.read_bytes(); bounds, pos = rect(data, 8)
        assert bounds == [0, width*20, 0, height*20]
        assert struct.unpack_from('<HH', data, pos) == (fps << 8, 1)
    for option in [('--size','0','1'),('--size','3277','1'),('--size','1','-2'),
                   ('--size','1'),('--fps','0'),('--fps','256'),('--fps','1x'),
                   ('--fps','9'*200),('--fps',''),('--fps',)]:
        out.write_bytes(b'preserve')
        result = subprocess.run([EXE, 'link', str(out), str(a), *option],
                                capture_output=True, timeout=15)
        assert result.returncode == 2 and out.read_bytes() == b'preserve'
    # A resource operand is data, never permission to emit extension code.
    a.write_bytes(object('channels', b'\0', 1)); out.write_bytes(b'preserve')
    root = Path(__file__).resolve().parents[1]/'examples'
    (p/'--extensions').write_bytes((root/'panel.svf').read_bytes())
    for explicit, expected_status in [(False, 1), (True, 0)]:
        args = [EXE, 'link', str(out), str(a), '--svf', 'panel', '--extensions']
        if explicit:
            args.append('--extensions')
        result = subprocess.run(args, cwd=p, capture_output=True, timeout=15)
        assert result.returncode == expected_status, result.stderr
        if not explicit:
            assert b'enable extensions explicitly' in result.stderr
            assert out.read_bytes() == b'preserve'
        else:
            tags(out.read_bytes())
print(f'{checks} total checks including stage policy')
