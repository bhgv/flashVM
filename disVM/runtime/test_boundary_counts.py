"""Execute actual Loader construction and emergency exception ownership paths."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink
from prepare_boundary_counts import adapt

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "root_registry.h"
#include <string.h>
void __real_dvm_runtime_root_registry_probe(struct DVM_GCWorkers *workers);
String *__real_c2string(char *text, int length);
void dvm_boundary_emergency_test(void);


/* Refuse exactly the diagnostic message allocation; real newestring must
 * recover through its own waserror frame and retain the permanent buffer. */
String *__wrap_c2string(char *text, int length)
{
    if (length == 17 && !memcmp(text, "boundary-fallback", 17))
    {
        error("boundary allocation refusal");
    }
    return __real_c2string(text, length);
}


/* Run after the actual managed root test while controller still owns admission.
 * workers stays borrowed, then normal owner tests and guest execution continue. */
void __wrap_dvm_runtime_root_registry_probe(struct DVM_GCWorkers *workers)
{
    __real_dvm_runtime_root_registry_probe(workers);
    dvm_boundary_emergency_test();
}
'''

EMERGENCY = r'''
#include "disvm/refcount.h"
#include <stdlib.h>


/* [GC:] Private test of the actual static newestring error branch. Permanent
 * exstr is independently rooted/owned; no GC admission is released here.
 * On failed ownership, stop before destroy can consume the permanent owner. */
void dvm_boundary_emergency_test(void)
{
    ulong before = dvm_refcount_load_u(&D2H(exstr)->ref);
    String *message = newestring("boundary-fallback");
    if (message != exstr || dvm_refcount_load_u(&D2H(exstr)->ref) != before+1)
    {
        fprint(2, "FAIL boundary counts: emergency retain\n");
        exit(10);
    }
    destroy(message);
    fprint(1, "PASS actual emergency buffer retains owner\n");
}
'''

GUEST = '''implement Boundary;
include "sys.m";
include "loader.m";
Boundary: module { init: fn(); };
# Construct an actual native module with one instruction and independent data.
# We do not execute its bytecode or enable any Dis64 file opcode.
init() {
    sys := load Sys Sys->PATH;
    loader := load Loader Loader->PATH;
    data := loader->dnew(16, nil);
    if (data == nil) {raise "fail: Loader.dnew: "+sys->sprint("%r");}
    code := array[1] of Loader->Inst;
    code[0] = Loader->Inst(byte 0, byte 0, 0, 0, 0);
    instance := loader->newmod("boundary-counts", 128, 0, code, data);
    if (instance == nil) {raise "fail: Loader.newmod: "+sys->sprint("%r");}
    sys->print("PASS actual Loader module ownership\\n");
}
'''


def replace_once(source, before, after):
    """Replace a private reviewed source anchor; reject test/source drift."""
    if source.count(before) != 1:
        raise RuntimeError('boundary control source drift: '+before)
    return source.replace(before, after)


def main():
    """Use matching runtime/compiler/includes; exact semantic10 controls only.

    All generated source/objects stay private. Production objects are not edited.
    Execute C/JIT and both poll backends; TSan mode excludes deliberate controls.
    """
    arguments = [value for value in sys.argv[1:] if value != '--positive-only']
    runtime, compiler, include = (Path(value).resolve() for value in arguments)
    positive_only = '--positive-only' in sys.argv
    view = runtime.parent / 'native'
    # Production converted source must reject repeated conversion; pinned hash
    # rejection also refuses arbitrary source drift before emitting a new view.
    for unit in ('loader', 'exception'):
        converted = (view / (unit+'.c')).read_text()
        for candidate in (converted, converted+'\n/* drift */\n'):
            try:
                adapt(candidate, unit)
            except ValueError:
                continue
            raise RuntimeError('boundary adapter accepted converted/drifted text')
    exception = (view / 'exception.c').read_text()
    loader = (view / 'loader.c').read_text()
    loader = replace_once(loader, '\tint n;\n\n\tc=arg;',
                          '\tint n;\n\tulong heap_before;\n\tint type_before;\n\n\tc=arg;')
    loader = replace_once(loader, '\th=D2H(f->data);',
                          '\th=D2H(f->data);\n\theap_before=dvm_refcount_load_u(&h->ref);'
                          '\n\ttype_before=dvm_refcount_load_i(&h->t->ref);')
    loader = replace_once(loader, '\tm->nprog=f->inst->len;', '''
    if(dvm_refcount_load_u(&h->ref) != heap_before+1) {
        fprint(2, "FAIL boundary counts: loader Heap retain\\n");
        exit(10);
    }
    if(dvm_refcount_load_i(&h->t->ref) != type_before+1) {
        fprint(2, "FAIL boundary counts: loader Type retain\\n");
        exit(10);
    }
\tm->nprog=f->inst->len;''')
    loader = '#include "disvm/refcount.h"\n#include <stdlib.h>\n'+loader
    with tempfile.TemporaryDirectory(prefix='boundary counts ') as directory:
        work = Path(directory)
        source = work / 'main.b'
        image = work / 'main.dis'
        source.write_text(GUEST)
        built = subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(source)],
                               capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError('boundary guest compile: '+built.stdout+built.stderr)
        wrapper = work / 'wrapper.c'
        wrapper.write_text(WRAPPER)
        controls = (0,) if positive_only else (0, 1, 2, 3)
        for variant in controls:
            current_exception = exception+EMERGENCY
            current_loader = loader
            if variant == 1:
                current_exception = replace_once(current_exception,
                    'dvm_runtime_heap_retain(&D2H(exstr)->ref);', '(void)exstr;')
            elif variant == 2:
                current_loader = replace_once(current_loader,
                    'dvm_runtime_heap_retain(&h->ref);', '(void)h;')
            elif variant == 3:
                current_loader = replace_once(current_loader,
                    'dvm_runtime_type_retain(&h->t->ref);', '(void)h;')
            private_exception = work / 'exception.c'
            private_loader = work / 'loader.c'
            private_exception.write_text(current_exception)
            private_loader.write_text(current_loader)
            binary = relink(runtime, work, wrapper, variant,
                ['c2string', 'dvm_runtime_root_registry_probe'],
                exception_source=private_exception, loader_source=private_loader)
            for jit in ('--jit', '--no-jit'):
                for locked in (False, True):
                    command = [str(binary), jit, '--gc-worker', '--managed-owner-probe']
                    if locked:
                        command += ['--gc-locked-poll']
                    run = subprocess.run(command+[str(image)], capture_output=True, text=True,
                                         timeout=45, env=dict(os.environ, TMPDIR=str(work)))
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    marker = {1: 'emergency retain', 2: 'loader Heap retain', 3: 'loader Type retain'}
                    if variant:
                        if run.returncode != 10 or 'FAIL boundary counts: '+marker[variant] not in output:
                            raise RuntimeError('boundary control missed: '+str(variant)+output)
                    elif run.returncode or 'PASS actual Loader module ownership' not in output or \
                            'PASS actual emergency buffer retains owner' not in output:
                        raise RuntimeError('boundary positive failed: '+output)
    print('PASS actual Loader/emergency ownership: C/JIT, both polls'+
          ('; positives only' if positive_only else '; three semantic controls'))


if __name__ == '__main__':
    main()
