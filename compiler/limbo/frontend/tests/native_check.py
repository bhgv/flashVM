"""Cross-check our Coco/R native C ABI against independent MD5/layout expectations and Inferno."""
from pathlib import Path
import hashlib
import os
import re
import shlex
import struct
import subprocess
import sys
import tempfile


# Execute an explicit generator/CC, returning text only after checking its expected status.
def run(arguments, success=True):
    result = subprocess.run([str(value) for value in arguments], capture_output=True, text=True)
    if (result.returncode == 0) != success:
        raise RuntimeError('native generator status mismatch: '+result.stdout+result.stderr)
    return result.stdout


# Decode only generated C table records for independent metadata comparison, not Limbo source.
def records(text):
    result = {}
    pattern = r'"([A-Za-z_0-9]+)"\s*,\s*0x([0-9a-f]+)(?:UL)?\s*,\s*\w+\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*\{([^}]*)\}'
    for name, signature, size, count, bits in re.findall(pattern, text):
        values = [int(value.strip(), 0) for value in bits.split(',') if value.strip()]
        result[name] = (int(signature, 16), int(size), int(count), values[:int(count)])
    return result


# Verify profile ABI selection, real signatures, pointer bits and failure retention.
# Optional oracle compiler paths are supplied explicitly; no runtime/network lookup is hidden here.
def main():
    generator = Path(sys.argv[1]).resolve()
    classic = sys.argv[2] if len(sys.argv) > 2 else ''
    nc64 = sys.argv[3] if len(sys.argv) > 3 else ''
    with tempfile.TemporaryDirectory(prefix='native-c-interface-') as temporary:
        work = Path(temporary)
        source = work / 'bridge.m'
        definitions = {
            'add': (['big', 'big'], 'big'),
            'check': (['int'], None),
            'greeting': ([], 'string'),
            'mixed': (['byte', 'big', 'real', 'int', 'string'], 'real'),
            'strings': (['string']*16, 'string'),
            'bytes': (['byte']*16, 'byte'),
        }
        tags = {'byte':'b', 'int':'i', 'big':'B', 'real':'r', 'string':'s', None:'n'}
        text = 'Bridge: module {\n'
        for name, (parameters, result) in definitions.items():
            text += name+': fn('+', '.join('arg%d: %s' % (index, kind) for index, kind in enumerate(parameters))+')'
            text += ': '+result if result else ''
            text += ';\n'
        source.write_text(text+'};\n')
        for cell, profile, oracle in ((4, 'native-c-classic32', classic), (8, 'native-c-nc64', nc64)):
            header = work / ('bridge%d.h' % cell)
            run([generator, '--format', profile, source, header])
            actual = records(header.read_text())
            expected = {}
            for name, (parameters, result) in definitions.items():
                signature = 'f('+','.join(tags[kind] for kind in parameters)+')'+tags[result]
                words = struct.unpack('<4I', hashlib.md5(signature.encode()).digest())
                folded = words[0]^words[1]^words[2]^words[3]
                offset = 8*cell
                bits = [0]*16
                count = 0
                for kind in parameters:
                    size = 1 if kind == 'byte' else 8 if kind in ('big','real') else cell
                    alignment = size
                    offset = (offset+alignment-1)//alignment*alignment
                    if kind == 'string':
                        slot = offset//cell
                        bits[slot//8] |= 0x80>>(slot%8)
                        count = slot//8+1
                    offset += size
                expected[name] = (folded, (offset+7)//8*8, count, bits[:count])
            if actual != expected:
                raise RuntimeError('wrong independently computed signatures/layout/maps: '+repr(actual))
            if oracle:
                reference = records(run([oracle, '-a', '-t', 'Bridge', source]))
                if reference != expected:
                    raise RuntimeError('native interface diverges from selected Inferno compiler')
            witness = work / 'witness.c'
            witness.write_text('#include <stdint.h>\ntypedef int64_t WORD; typedef int64_t LONG; typedef double REAL;\n'
                               'typedef unsigned char uchar; typedef uchar BYTE; typedef struct String String;\n'
                               'enum { NREG=5 };\n#include "'+header.name+'"\n')
            command = shlex.split(os.environ.get('CC','cc'))+['-std=c99','-c',str(witness),'-o',str(work/'witness.o')]
            run(command, success=cell == 8)
        unsupported = [
            'Bad: module { value: fn(x: array of byte): int; };',
            'Bad: module { Alias: type int; value: fn(x: Alias): int; };',
            'Bad: module { value: fn(x: int, *): int; };',
            'Bad: module { value: fn(register: int): int; };',
            'Bad: module { value: fn(x: int) raises Problem; };',
        ]
        output = work / 'unchanged.h'
        for text in unsupported:
            source.write_text(text)
            output.write_text('previous valid interface\n')
            run([generator,'--format','native-c-nc64',source,output],success=False)
            if output.read_text() != 'previous valid interface\n':
                raise RuntimeError('unsupported ABI profile overwrote an existing C interface')
    print('PASS Coco/R native C: classic32/NC64 signatures, frame layout, GC maps, host guards and rejection')


if __name__ == '__main__':
    main()
