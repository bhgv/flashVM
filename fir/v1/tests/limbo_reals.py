"""Check Coco/R homogeneous Limbo real profiles, provenance and rejection rollback."""

import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile


# Wrap a borrowed numeric body with matching tutorial interface/definition result types.
def program(body, interface='real', definition='real'):
    return ('implement RealProbe; RealProbe: module {value: fn(): ' + interface + ';}; '
            'value(): ' + definition + ' {' + body + '}')


# Compile typed real source through independent CLI tools; no bootstrap compiler is needed.
# argv supplies source/FIR CLIs. All files are private and rejected compilation preserves sentinels.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:])
    with tempfile.TemporaryDirectory(prefix='CocoR Limbo real ') as temporary:
        directory = Path(temporary)
        environment = dict(os.environ, TMPDIR=str(directory))
        source, fir, debug = (directory / name for name in ('program.b', 'program.fir', 'debug.json'))

        # Require bounded completion and the expected exit status for every generated-parser call.
        def call(arguments, success=True):
            result = subprocess.run(list(map(str, arguments)), env=environment, capture_output=True,
                                    text=True, timeout=10)
            if (result.returncode == 0) != success:
                raise RuntimeError(result.stdout + result.stderr)
            return result

        for body, expected in [('return -0.0;', -0.0), ('x: real = 2.5; x /= 2.0; return x;', 1.25),
                               ('x := 4.0; {x := 9.0; x += 1.0;} return x * 2.0;', 8.0),
                               ('# Я😀\nx := 1.25e2; {x /= 2.0;} return x - 20.0;', 42.5)]:
            text = program(body)
            source.write_text(text)
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug])
            if 'source_semantics limbo_real\n' not in fir.read_text() or ': i32 =' in fir.read_text():
                raise RuntimeError('real source lost its homogeneous type/provenance')
            for task, output in [('check', None), ('print', 'printed.fir'), ('opt', 'optimized.fir'),
                                 ('object', 'program.swo'), ('dis', 'program.dis')]:
                arguments = [compiler, task, fir]
                if output:
                    arguments.append(directory / output)
                call(arguments)
            if 'source_semantics limbo_real' not in (directory / 'printed.fir').read_text():
                raise RuntimeError('round trip dropped Limbo real provenance')
            if environment.get('FIR_FLASHVM_RUNNER'):
                if not environment.get('FIR_SWL'):
                    raise RuntimeError('real SWF execution requires explicit FIR_SWL')
                for selected in (fir, directory / 'optimized.fir'):
                    obj, movie = directory / 'real.swo', directory / 'real.swf'
                    call([compiler, 'object', selected, obj])
                    call([environment['FIR_SWL'], 'link', movie, obj])
                    result = call([environment['FIR_FLASHVM_RUNNER'], movie])
                    # SWF trace formatting is numeric observation, not an IEEE payload/sign-zero oracle.
                    if float(result.stdout.strip()) != expected:
                        raise RuntimeError('our Limbo real result differs on the actual SWF target')
            record = json.loads(debug.read_text())
            definitions = set(re.findall(r'%v([0-9]+):', fir.read_text()))
            origins = {str(event['value']) for event in record['events']
                       if event['kind'] in ('constant', 'binary')}
            if definitions != origins:
                raise RuntimeError('real SSA values are missing source origins')
        rejected = [program('real := 2.5; return real;'), program('return 2;'), program('return 16rA;'), program('return 0x1e;'),
                    program('return 1e400;'), program('x: int = 2.5; return x;'),
                    program('return 2.5;', 'int', 'real'), program('return 2;', 'real', 'int'),
                    program('if(1.0<2.0){return 1.0;}else{return 2.0;}'),
                    program('return real 2;'), program('return 1.0 + ;'), program('return 1.0;')[:-1],
                    program('x: real = 1; return x;'), program('x: real = 2; return x;', 'int', 'int')]
        for text in rejected:
            source.write_text(text)
            fir.write_bytes(b'old FIR')
            debug.write_bytes(b'old debug')
            result = call([source_compiler, 'limbo', '--input', source, fir, '-g', debug,
                           '--diagnostics-json'], success=False)
            diagnostic = json.loads(result.stderr)
            if diagnostic['kind'] not in ('semantic', 'syntax') or fir.read_bytes() != b'old FIR' or debug.read_bytes() != b'old debug':
                raise RuntimeError('real rejection lost diagnostic/transactional output')
        # Adding a Limbo type keyword must preserve the same spelling as an AS variable name.
        for language in ('as1', 'as2', 'as3'):
            source.write_text('var real = 2.5; trace(real * 2.0);')
            call([source_compiler, language, '--input', source, fir])
            call([compiler, 'dis', fir, directory / 'as-real-name.dis'])
        # The new profile is paired only with Limbo; AS must not acquire it through forged metadata.
        source.write_text(program('return 2.5;'))
        call([source_compiler, 'limbo', '--input', source, fir])
        fir.write_text(fir.read_text().replace('source_language limbo', 'source_language as3'))
        call([compiler, 'check', fir], success=False)
    print('PASS Limbo real grammar, homogeneous bindings/scopes, provenance, origins and rejection rollback')


if __name__ == '__main__':
    main()
