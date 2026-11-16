# Copyright (c) 2026 bhgv
# EN: Run real guest cwd/module/event operations over a private dynamic provider.
# RU: Настоящие guest cwd/module/event operations над private dynamic provider.
from pathlib import Path
import json
import subprocess
import sys
import tempfile
from test_owner_location import compile_image
from relink_private import relink


# EN/RU: Compile native module fixture and run unchanged Sys in native interpreter/JIT.
# Собрать native module fixture; выполнить Sys в native interpreter/JIT.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    fixture = root/'fixtures/provider-cwd'
    with tempfile.TemporaryDirectory(prefix='provider cwd ') as name:
        work = Path(name)
        (work/'remote').mkdir()
        (work/'plain').mkdir()
        (work/'plain/item').write_text('C')
        compile_image(compiler, include, root, fixture/'module-b.b', work/'plain/provider.dis', (fixture,))
        source = (fixture/'main.b').read_text().replace('"@FIXTURE_BASE@"', json.dumps(str(work)))
        (work/'main.b').write_text(source)
        compile_image(compiler, include, root, work/'main.b', work/'main.dis', (fixture,))
        binary = relink(runtime, work, root/'test_provider_cwd.c', 0,
                        ['dvm_namespace_boot_initialize', 'dvm_namespace_boot_shutdown'])
        for mode in ('--no-jit', '--jit'):
            result = subprocess.run([binary, '--native-spawn', mode, '--stats', work/'main.dis'],
                                    cwd=work, capture_output=True, text=True, timeout=45)
            output = result.stdout+result.stderr
            if result.returncode or 'PASS guest provider cwd identity copy loader and events' not in output or \
                    'PASS guest provider cwd native lifetimes drained' not in output:
                raise RuntimeError(output[-18000:])
            print('PASS provider cwd native guest '+mode, flush=True)


if __name__ == '__main__':
    main()
