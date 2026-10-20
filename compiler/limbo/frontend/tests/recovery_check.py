"""Truncated syntax must terminate; acceptance is separate from resource behavior."""
from pathlib import Path
import subprocess,sys,tempfile
seed='implement L; f() { outer: for(;;) { alt { x := <-c => break outer; * => exit; } } }'
cases=[seed[:i] for i in range(len(seed)+1)]
cases += ['@', '/*', 'for(', 'f(){', '}', seed+'@', seed+'(']
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp)/'truncated.src'
 for source in cases:
  p.write_text(source)
  r=subprocess.run([sys.argv[1],str(p)],capture_output=True,timeout=5)
  assert r.returncode in (0,1),(source,r.returncode,r.stderr)
  assert b'ERROR: AddressSanitizer' not in r.stderr and b'runtime error:' not in r.stderr
print(len(cases),'termination cases; not additional syntax-coverage claims')
