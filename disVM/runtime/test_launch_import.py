# Copyright (c) 2026 bhgv
# EN: Real startup adoption with pair/duplex/absent terminal and native rollback fault injection.
# RU: Настоящее startup adoption с pair/duplex/absent terminal и native rollback fault injection.
from pathlib import Path
import subprocess
import sys
import tempfile
from test_owner_location import compile_image
from relink_private import relink


# EN/RU: Build matched guest/library, install only native admission fixture, require all cleanup markers.
# Собрать guest/library, заменить только native admission fixture, проверить cleanup markers.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='launch import ') as name:
        work = Path(name)
        for source, target in [(root/'fixtures/terminal-init/import.b', 'main'),
                               (root/'fixtures/terminal-init/app.b', 'app'),
                               (root.parent/'limbo/terminal.b', 'terminal')]:
            compile_image(compiler, include, root, source, work/(target+'.dis'))
        symbols = ['dvm_runtime_launch_module', 'dvm_namespace_boot_shutdown', 'malloc', 'free',
                   'newstack', 'dvm_sysevents_create', 'dvm_messages_register']
        for variant in range(3):
            binary = relink(runtime, work, root/'test_launch_import.c', variant, symbols)
            for mode in ('--no-jit', '--jit'):
                result = subprocess.run([binary, '--native-spawn', mode, '--stats', work/'main.dis'],
                                        cwd=work, capture_output=True, text=True, timeout=45)
                output = result.stdout+result.stderr
                marker = 'PASS launch import without terminal' if variant == 2 else \
                    'PASS native launch import before typed application init'
                if result.returncode or marker not in output or \
                        'PASS launch import constructor rollback and native lifetimes' not in output:
                    raise RuntimeError(str(result.returncode)+': '+output[-16000:])
                print(f'PASS launch import variant{variant} {mode}', flush=True)


if __name__ == '__main__':
    main()
