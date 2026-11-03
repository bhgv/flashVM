"""Relink a private diagnostic wrapper against unchanged CMake runtime objects."""
from pathlib import Path
import shlex
import subprocess


# runtime identifies the matching native console build, work owns temporary
# outputs, source is its C wrapper, variant copied define and symbols are exact
# linker interceptions. Return private binary. No production object is changed.
# Makefiles/GNU-style native linker only; compile/link failures never count as
# semantic detection. Optional executor replaces only private xec.c object.
# Matching private headers/flags remain mandatory.
def relink(runtime, work, source, variant, symbols, executor=None):
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
    if executor is not None:
        replacement = work / 'executor.o'
        command = [link[0], *flags['C_DEFINES'], *flags['C_INCLUDES'], *flags['C_FLAGS'],
                   '-c', str(executor), '-o', str(replacement)]
        built = subprocess.run(command, capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError('private executor compile failed: '+built.stdout+built.stderr)
        targets = [index for index, value in enumerate(link) if value.endswith('/native/xec.c.o')]
        if len(targets) != 1:
            raise RuntimeError('private executor object boundary drift')
        link[targets[0]] = str(replacement)
    binary = work / ('runtime-'+str(variant))
    link[link.index('-o')+1] = str(binary)
    link += [str(obj), *['-Wl,--wrap='+symbol for symbol in symbols]]
    built = subprocess.run(link, cwd=runtime.parent, capture_output=True, text=True)
    if built.returncode:
        raise RuntimeError('private runtime relink failed: '+built.stdout+built.stderr)
    return binary
