"""Actual POSIX jump boundary: four owners, nested frames, masks and link symbols."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

SOURCE = r'''
#include "native_jump.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>


/* One main-owned record per worker. id is immutable; completed is worker-local
 * until pthread_join publishes it to main. No record outlives the joined owner. */
typedef struct Owner
{
    int id; /* Immutable owner-specific jump status. */
    unsigned long completed; /* Written by this worker; read only after join. */
} Owner;


/* valid/message are a required predicate and borrowed diagnostic. Fail with
 * semantic exit10, independent of compile/link or sanitizer errors. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL native jump: %s\n", message);
        exit(10);
    }
}


/* Borrow owner; two nested live continuations belong only to this OS thread.
 * Repeated transfers restore SIGUSR1 and preserve independent statuses. Return
 * NULL after every frame is retired; no VM heap or cross-thread jump is used. */
static void *worker(void *argument)
{
    Owner *owner = argument;
    sigset_t blocked;
    sigset_t original;
    volatile unsigned int iteration;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGUSR1);
    check(pthread_sigmask(SIG_BLOCK, &blocked, &original) == 0, "initial mask");
    for (iteration = 0; iteration < 12000; iteration++)
    {
        sigjmp_buf outer;
        int status = sigsetjmp(outer, 1);
        if (!status)
        {
            sigjmp_buf inner;
            status = sigsetjmp(inner, 1);
            if (!status)
            {
                check(pthread_sigmask(SIG_UNBLOCK, &blocked, NULL) == 0, "change inner mask");
                dvm_native_error_jump(inner, owner->id);
                check(0, "jump returned");
            }
            check(status == owner->id, "inner owner status");
            {
                sigset_t current;
                check(pthread_sigmask(SIG_SETMASK, NULL, &current) == 0 &&
                    sigismember(&current, SIGUSR1) == 1, "inner saved signal mask");
            }
            check(pthread_sigmask(SIG_UNBLOCK, &blocked, NULL) == 0, "change outer mask");
            dvm_native_error_jump(outer, owner->id+16);
            check(0, "outer jump returned");
        }
        check(status == owner->id+16, "outer owner status");
        {
            sigset_t current;
            check(pthread_sigmask(SIG_SETMASK, NULL, &current) == 0 &&
                sigismember(&current, SIGUSR1) == 1, "outer saved signal mask");
        }
        owner->completed++;
    }
    check(pthread_sigmask(SIG_SETMASK, &original, NULL) == 0, "final mask");
    return NULL;
}


/* No arguments. Own four worker records until all joins, then require exactly
 * 96000 actual transfers. Creation/join failures are explicit semantic errors. */
int main(void)
{
    Owner owners[4] = {{1, 0}, {2, 0}, {3, 0}, {4, 0}};
    pthread_t threads[4];
    unsigned int index;
    unsigned long total = 0;
    for (index = 0; index < 4; index++)
    {
        check(pthread_create(&threads[index], NULL, worker, &owners[index]) == 0, "create owner");
    }
    for (index = 0; index < 4; index++)
    {
        check(pthread_join(threads[index], NULL) == 0, "join owner");
        check(owners[index].completed == 12000, "complete owner");
        total += owners[index].completed*2;
    }
    check(total == 96000, "complete transfers");
    puts("PASS native jump: four owners,96000 nested transfers, masks and statuses");
    return 0;
}
'''  # Native continuations only; actual managed cleanup has separate VM fixtures.


def main():
    """Compile matching boundary, require live controls and glibc symbol selection.

    Optional --tsan instruments production and fixture C; never count a sanitizer
    crash or compiler failure as the expected semantic exit10. Temporary files
    own all artifacts; processes have a bounded timeout and all workers join.
    """
    root = Path(__file__).resolve().parent
    compiler = shlex.split(os.environ.get('CC', 'cc'))
    tsan = '--tsan' in sys.argv
    flags = ['-std=c99', '-O2', '-g', '-D_POSIX_C_SOURCE=200809L', '-D_FORTIFY_SOURCE=2',
             '-pthread', '-I'+str(root)]
    if tsan:
        flags += ['-fsanitize=thread', '-fno-omit-frame-pointer']
    with tempfile.TemporaryDirectory(prefix='native jump ') as directory:
        work = Path(directory)
        source = work/'main.c'
        source.write_text(SOURCE)
        boundary = (root/'native_jump.c').read_text()
        for variant in (0, 1, 2):
            actual = work/'boundary.c'
            actual.write_text(boundary.replace('siglongjmp(buffer, value);', 'siglongjmp(buffer, value+1);') if variant == 1 else boundary)
            source.write_text(SOURCE.replace('sigsetjmp(inner, 1)', 'sigsetjmp(inner, 0)') if variant == 2 else SOURCE)
            image = work/'probe'
            subprocess.run(compiler+flags+[str(source), str(actual), '-o', str(image)], check=True)
            run = subprocess.run([str(image)], capture_output=True, text=True, timeout=60)
            output = run.stdout+run.stderr
            if 'WARNING: ThreadSanitizer:' in output or 'ThreadSanitizer:DEADLYSIGNAL' in output:
                raise RuntimeError(output)
            if variant:
                expected = 'inner owner status' if variant == 1 else 'inner saved signal mask'
                if run.returncode != 10 or 'FAIL native jump: '+expected not in output:
                    raise RuntimeError('semantic control missed: '+output)
            elif run.returncode or 'PASS native jump:' not in output:
                raise RuntimeError('boundary failed: '+output)
        # These symbol checks are glibc-specific; other libcs retain functional evidence.
        macros = subprocess.check_output(compiler+['-dM', '-E', '-include', 'features.h', '-'], input='', text=True)
        if '__GLIBC__' in macros:
            for sanitized, symbol in ((False, '__longjmp_chk'), (True, 'siglongjmp')):
                obj = work/'boundary.o'
                selection = ['-D__SANITIZE_THREAD__=1'] if sanitized else []
                subprocess.run(compiler+flags+selection+['-c', str(root/'native_jump.c'), '-o', str(obj)], check=True)
                symbols = subprocess.check_output(['nm', '-u', str(obj)], text=True)
                # Under real --tsan both probes must select the intercepted symbol.
                expected = 'siglongjmp' if tsan else symbol
                if expected not in symbols or (expected == 'siglongjmp' and '__longjmp_chk' in symbols):
                    raise RuntimeError('jump symbol selection: '+symbols)
    print('PASS native jump boundary, two semantic controls and glibc symbol policy')


if __name__ == '__main__':
    main()
