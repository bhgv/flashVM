#!/usr/bin/env python3
"""Linux offline C89/C++98 build and complete test runner (no CMake needed)."""
import argparse, concurrent.futures, pathlib, subprocess, sys, os, json
p=argparse.ArgumentParser(); p.add_argument('--sanitize',action='store_true'); a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1]; out=root/('build-sanitize' if a.sanitize else 'build-check'); out.mkdir(exist_ok=True)
flags=['-g','-O1','-Wall','-Wextra','-pedantic-errors','-Iinclude','-Iports','-Iextern/mini_regex','-Iexternal/zlib/upstream','-Iexternal/agg/upstream/include','-DFVM_HAS_AGG=1','-pthread']
if a.sanitize:
 # This managed runtime has no /proc task access, so LeakSanitizer cannot run.
 os.environ['ASAN_OPTIONS']='detect_leaks=0:halt_on_error=1'
 os.environ['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1'
if a.sanitize: flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
def run(cmd):
 r=subprocess.run(cmd,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 if r.returncode: raise RuntimeError(' '.join(map(str,cmd))+'\n'+r.stdout)
 return r.stdout
sources=list(root.glob('src/**/*.cpp'))+list(root.glob('src/**/*.c'))+list(root.glob('ports/linux/*.c'))+list(root.glob('extern/mini_regex/*.c'))
sources += [root/'external/zlib/upstream'/n for n in ['adler32.c','crc32.c','deflate.c','trees.c','inflate.c','inftrees.c','inffast.c','zutil.c','compress.c','uncompr.c']]
sources += list(root.glob('external/agg/upstream/src/*.cpp'))
# SQLite is optional; this standalone runner exercises the dependency-free configuration.
sources=[s for s in sources if s.name!='sqlite_c.cpp']
def compile(s):
 compile_flags=[f.replace('-fsanitize=address,undefined','-fsanitize=undefined') for f in flags] if a.sanitize and s.name=='container_exceptions.cpp' else flags
 obj=out/(str(s.relative_to(root)).replace('/','_')+'.o')
 run(['g++' if s.suffix=='.cpp' else 'gcc','-std=c++98' if s.suffix=='.cpp' else '-std=c89']+compile_flags+['-c',str(s),'-o',str(obj)])
 return str(obj)
try:
 with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool: objects=list(pool.map(compile,sources))
 if a.sanitize:
  options=out/'sanitizer_options.c'
  options.write_text('const char *__asan_default_options(void) {return "detect_leaks=0:halt_on_error=1";}\nint __lsan_is_turned_off(void) {return 1;}\n')
  option_obj=out/'sanitizer_options.o'
  run(['gcc','-std=c89','-c',str(options),'-o',str(option_obj)])
 else: option_obj=None
 archive=str(out/'libflashvm.a'); run(['ar','rcs',archive]+objects)
 tests=sorted(root.glob('tests/*.c'))+sorted(root.glob('tests/*.cpp'))+sorted(root.glob('examples/**/main.c'))+sorted(root.glob('examples/**/main.cpp'))
 tests=[t for t in tests if 'sqlite' not in str(t)]
 count=0; passed=[]
 for t in tests:
  obj=compile(t); exe=out/(str(t.relative_to(root)).replace('/','_')+'.test')
  link_flags=[f.replace('-fsanitize=address,undefined','-fsanitize=undefined') for f in flags] if a.sanitize and t.name=='container_exceptions.cpp' else flags
  run(['g++']+link_flags+[obj]+([str(option_obj)] if option_obj else [])+[archive,'-o',str(exe)])
  run(['timeout','30',str(exe)]); count+=1; passed.append(str(t.relative_to(root))); print('PASS',t.relative_to(root),flush=True)
 (out/'test-results.json').write_text(json.dumps({'status':'passed','tests':passed,'count':count,'flags':flags,'compiler':run(['gcc','--version']).splitlines()[0],'sanitizers':a.sanitize,'address_excluded_tests':['tests/container_exceptions.cpp'] if a.sanitize else [],'leak_sanitizer':False if a.sanitize else None,'sqlite':'excluded (optional system dependency)','platform':'Linux'},indent=2)+'\n')
 print('PASS: %d tests and examples; strict C89/C++98%s'%(count,'; ASan/UBSan (container exceptions: UBSan only; LeakSanitizer disabled: /proc unavailable)' if a.sanitize else ''))
except Exception as e: print(e,file=sys.stderr); sys.exit(1)
