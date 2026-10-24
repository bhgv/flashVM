"""Compile/run generated C stubs, load Python stubs and verify exact .m metadata."""
import json
from pathlib import Path
import runpy
import subprocess
import sys
import tempfile


# Execute the real Coco/R frontend and inspect products rather than copied examples.
def main():
    generator, cc = sys.argv[1:]
    source = ('# UTF-8: 中\ninclude "not-opened.m";\n'
              'Native:module{ VERSION:con 1; Request:adt{value:int;};\n'
              'add:fn(a,b:int):int;\n'
              '求值:fn(request:ref Request):string; };\n')
    with tempfile.TemporaryDirectory(prefix="limbo-bindings-") as directory:
        root = Path(directory)
        inp = root / "module.m"
        inp.write_text(source, encoding="utf-8")
        for language, suffix in [('json', 'json'), ('c', 'c'), ('python', 'py'), ('as1', 'as1'), ('as2', 'as2'), ('as3', 'as3')]:
            output = root / ('bindings.' + suffix)
            subprocess.run([generator, '--format', language, str(inp), str(output),
                            '--prefix', 'fixture'], check=True, timeout=15)
        for language in ('as1', 'as2', 'as3'):
            text = (root / ('bindings.' + language)).read_text()
            assert 'unimplemented external callback:' in text and 'new Object()' in text
            assert text.count('function fixture_call_') == 2
        record = json.loads((root / 'bindings.json').read_text())
        assert record['format'] == 'LIMBO-INTERFACE-1' and record['source'] == source
        functions = [d for d in record['declarations'] if d['kind'] == 'function']
        assert [d['name'] for d in functions] == ['add', '求值']
        for d in record['declarations']:
            if not d['name']:
                continue
            start, length = d['byte_offset'], d['byte_length']
            assert source.encode()[start:start + length].decode() == d['name']
            prefix = source.encode()[:start]
            assert d['line'] == prefix.count(b'\n') + 1
            assert d['column'] == len(prefix.rsplit(b'\n', 1)[-1]) + 1
        # The generated callbacks must fail explicitly until implemented by a host.
        harness = root / 'check.c'
        harness.write_text('#include "bindings.c"\nint main(void) { size_t i; '
                           'if(fixture_export_count != 2) return 1; '
                           'for(i=0;i<fixture_export_count;i++) '
                           'if(fixture_exports[i].call(NULL,NULL) != -1) return 2; return 0; }\n')
        binary = root / 'check'
        subprocess.run([cc, '-std=c99', '-Wall', '-Wextra', '-Werror', str(harness), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        callbacks = runpy.run_path(str(root / 'bindings.py'))['fixture_exports']
        assert len(callbacks) == 2
        for callback in callbacks.values():
            try:
                callback(None, None)
            except NotImplementedError:
                pass
            else:
                raise AssertionError('scaffold fabricated success')
        for bad in ['M:module{f:fn();f:fn();};', 'implement M; M:module{f:fn();}; f(){}']:
            inp.write_text(bad)
            output = root / 'bindings.c'
            output.write_bytes(b'old output')
            result = subprocess.run([generator, '--format', 'c', str(inp), str(output)], capture_output=True)
            assert result.returncode == 1 and output.read_bytes() == b'old output'
        inp.write_text('M:module{N:con 1;};')
        subprocess.run([generator, '--format', 'c', str(inp), str(root / 'empty.c')], check=True)
        subprocess.run([cc, '-std=c99', '-Wall', '-Wextra', '-Werror', '-c', str(root / 'empty.c'),
                        '-o', str(root / 'empty.o')], check=True)
    print('PASS Coco/R .m to C/Python scaffolds, debug positions and rejection rollback')


if __name__ == '__main__':
    main()
