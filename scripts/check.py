#!/usr/bin/env python3
"""Linux offline C89/C++98 build and complete test runner (no CMake needed)."""
import argparse, concurrent.futures, pathlib, subprocess, sys
p=argparse.ArgumentParser(); p.add_argument('--sanitize',action='store_true'); a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1]; out=root/('build-sanitize' if a.sanitize else 'build-check'); out.mkdir(exist_ok=True)
flags=['-g','-O1','-Wall','-Wextra','-pedantic-errors','-Iinclude','-Iports','-Iextern/mini_regex','-pthread']
if a.sanitize: flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
def run(cmd):
 r=subprocess.run(cmd,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 if r.returncode: raise RuntimeError(' '.join(map(str,cmd))+'\n'+r.stdout)
 return r.stdout
sources=list(root.glob('src/**/*.cpp'))+list(root.glob('src/**/*.c'))+list(root.glob('ports/linux/*.c'))+list(root.glob('extern/mini_regex/*.c'))
# SQLite is optional; this standalone runner exercises the dependency-free configuration.
sources=[s for s in sources if s.name!='sqlite_c.cpp']
def compile(s):
 obj=out/(str(s.relative_to(root)).replace('/','_')+'.o')
 run(['g++' if s.suffix=='.cpp' else 'gcc','-std=c++98' if s.suffix=='.cpp' else '-std=c89']+flags+['-c',str(s),'-o',str(obj)])
 return str(obj)
try:
 with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool: objects=list(pool.map(compile,sources))
 archive=str(out/'libflashvm.a'); run(['ar','rcs',archive]+objects)
 tests=sorted(root.glob('tests/*.c'))+sorted(root.glob('tests/*.cpp'))+sorted(root.glob('examples/**/main.c'))+sorted(root.glob('examples/**/main.cpp'))
 tests=[t for t in tests if 'sqlite' not in str(t)]
 count=0
 for t in tests:
  obj=compile(t); exe=out/(str(t.relative_to(root)).replace('/','_')+'.test')
  run(['g++']+flags+[obj,archive,'-o',str(exe)])
  run(['timeout','30',str(exe)]); count+=1; print('PASS',t.relative_to(root),flush=True)
 print('PASS: %d tests and examples; strict C89/C++98%s'%(count,'; ASan/UBSan' if a.sanitize else ''))
except Exception as e: print(e,file=sys.stderr); sys.exit(1)
