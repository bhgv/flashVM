"""Relink a private diagnostic wrapper against unchanged CMake runtime objects."""
from pathlib import Path
import shlex
import subprocess


# runtime identifies the matching native console build, work owns temporary
# outputs, source is its C wrapper, variant copied define and symbols are exact
# linker interceptions. Return private binary. No production object is changed.
# Makefiles/GNU-style native linker only; compile/link failures never count as
# semantic detection. Optional executor/compiler_source replace only private xec.c/AMD64 compiler
# objects; originals and source headers remain unchanged.
# Optional exception/loader/collector/managed-owner replacements compile private views.
# Optional shared_arithmetic_source replaces only that guarded transaction body;
# benchmark comparisons keep computation and other production objects unchanged.
# Optional bytes_fast_source changes only the private C dispatcher for exact
# decline/false-completion/duplicate-execution controls.
# Matching private headers/flags remain mandatory.
def relink(runtime, work, source, variant, symbols, executor=None,
           compiler_source=None, exception_source=None, loader_source=None,
           stack_source=None, runtime_rc_source=None, gc_source=None,
           managed_owner_source=None, shared_pointer_source=None, shared_bytes_source=None,
           shared_arithmetic_source=None, shared_real_arithmetic_source=None, bytes_fast_source=None, shared_real_conversion_source=None, heap_hold_source=None, list_hold_source=None):
    metadata = runtime.parent / 'CMakeFiles/disvm.dir'
    flags = {}
    for line in (metadata / 'flags.make').read_text().splitlines():
        if ' = ' in line:
            key, value = line.split(' = ', 1)
            flags[key] = shlex.split(value)
    link = [value for value in shlex.split((metadata / 'link.txt').read_text())
            if not value.startswith('-Wl,--dependency-file=')]
    obj = work / 'wrapper.o'
    includes = '-I'+str(Path(__file__).resolve().parent)
    command = [link[0], *flags['C_DEFINES'], *flags['C_INCLUDES'], *flags['C_FLAGS'],
               includes, '-DCONTROL='+str(variant), '-c', str(source), '-o', str(obj)]
    built = subprocess.run(command, capture_output=True, text=True)
    if built.returncode:
        raise RuntimeError('private wrapper compile failed: '+built.stdout+built.stderr)
    for candidate, filename in ((executor, 'xec.c'), (compiler_source, 'comp-amd64.c'),
                                (exception_source, 'exception.c'), (loader_source, 'loader.c'),
                                (stack_source, 'validstk.c'), (runtime_rc_source, 'runtime_rc.c'),
                                (gc_source, 'gc.c'), (managed_owner_source, 'managed_owner.c'),
                                (shared_pointer_source, 'shared_pointer.c'), (shared_bytes_source, 'shared_bytes.c'),
                                (shared_arithmetic_source, 'shared_arithmetic.c'),
                                (shared_real_arithmetic_source, 'shared_real_arithmetic.c'),
                                (bytes_fast_source, 'bytes_fast.c'),
                                (shared_real_conversion_source, 'shared_real_conversion.c'),
                                (heap_hold_source, 'heap_hold.c'),
                                (list_hold_source, 'list_hold.c')):
        if candidate is None:
            continue
        replacement = work / (filename+'.o')
        command = [link[0], *flags['C_DEFINES'], *flags['C_INCLUDES'], *flags['C_FLAGS'],
                   '-c', str(candidate), '-o', str(replacement)]
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError('private replacement compile failed: '+built.stdout+built.stderr)
        suffix = '/'+filename+'.o' if filename in ('runtime_rc.c', 'managed_owner.c', 'shared_pointer.c', 'shared_bytes.c', 'shared_arithmetic.c', 'shared_real_arithmetic.c', 'bytes_fast.c', 'shared_real_conversion.c', 'heap_hold.c', 'list_hold.c') else '/native/'+filename+'.o'
        targets = [index for index, value in enumerate(link) if value.endswith(suffix)]
        if len(targets) != 1:
            raise RuntimeError('private replacement object boundary drift: '+filename)
        link[targets[0]] = str(replacement)
    binary = work / ('runtime-'+str(variant))
    link[link.index('-o')+1] = str(binary)
    link += [str(obj), *['-Wl,--wrap='+symbol for symbol in symbols]]
    built = subprocess.run(link, cwd=runtime.parent, capture_output=True, text=True)
    if built.returncode:
        raise RuntimeError('private runtime relink failed: '+built.stdout+built.stderr)
    return binary
