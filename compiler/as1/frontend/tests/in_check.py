from pathlib import Path
import subprocess,sys,tempfile
exe=sys.argv[1]
good=['trace("x" in obj);', 'if (x in obj) f();', 'for(var k in obj) trace(k);',
      'for(k in obj) f();', 'for(var k=("x" in obj);k;k=false) f();',
      'for(var k=f("x" in obj);k;k=false) f();',
      'for(var k=a["x" in obj];k;k=false) f();',
      'for(var k=["x" in obj];k;k=false) f();',
      'for(var k={x:"x" in obj};k;k=false) f();',
      'for(var f=function(){ return "x" in obj; };f;f=null) f();',
      'for(var k in ("x" in obj ? a:b)) if(k in obj) f();']
good += [(Path(__file__).resolve().parents[2]/'examples/in.as').read_text()]
bad=['trace(in obj);','trace(x in);','for(var k in) f();',
     'for(var k = "x" in obj;;) f();','for(k in obj; ; ) f();']
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp)/'input.as'
 for expected,cases in [(0,good),(1,bad)]:
  for source in cases:
   p.write_text(source);r=subprocess.run([exe,str(p)],capture_output=True,timeout=5)
   assert r.returncode==expected,(source,r.returncode,r.stderr)
   assert b'ERROR: AddressSanitizer' not in r.stderr and b'runtime error:' not in r.stderr
print(len(good)+len(bad),'in/NoIn syntax cases')
