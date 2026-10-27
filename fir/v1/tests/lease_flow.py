"""Exercise retained-owner dataflow, alias identities and publication failures."""
from pathlib import Path
import sys
import re
import tempfile

from table_mutation import call


# Build complete executable-profile FIR around a borrowed block body. Table SSA
# names denote distinct owners unless copied; this fixture introduces no runtime.
def module(body):
    return 'FIR 1.0\nmodule @flow\nfunc @main() -> void {\n' + body + '\n}\n'


# CLI receives one compiler. Check valid CFGs before/after optimization and reject
# invalid ownership before print/opt/Dis/SWO can replace an existing output file.
def main():
    compiler = Path(sys.argv[1]).resolve()
    runtime = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else None
    mode = '--jit' if len(sys.argv) > 3 and sys.argv[3] == 'jit' else '--no-jit'
    prelude = '^entry:\n%t: table.big = table.new\n%u: table.big = table.new\n%c: bool = const.bool true\n'
    valid = [
        prelude + 'table.lock %t\n%a: table.big = copy %t\ntable.lock %a\n'
        'table.unlock %t\ntable.unlock %a\nreturn',
        prelude + 'table.lock %t\nif %c, ^left, ^right\n'
        '^left:\ntable.unlock %t\njump ^join\n'
        '^right:\ntable.unlock %t\njump ^join\n^join:\nreturn',
        prelude + 'table.lock %t\nif %c, ^left, ^right\n'
        '^left:\njump ^join\n^right:\ntable.lock %t\ntable.unlock %t\njump ^join\n'
        '^join:\ntable.unlock %t\nreturn',
        prelude + 'table.lock %t\njump ^loop\n'
        '^loop:\nif %c, ^body, ^exit\n^body:\ntable.lock %t\ntable.unlock %t\njump ^loop\n'
        '^exit:\ntable.unlock %t\nreturn',
        prelude + 'table.lock %t\ntable.lock %u\ntable.unlock %t\ntable.unlock %u\nreturn',
    ]
    invalid = [
        (prelude + 'table.unlock %t\nreturn', 'unlock without matching lock'),
        (prelude + 'table.lock %t\nreturn', 'return with retained table ownership'),
        (prelude + 'table.lock %t\ntable.unlock %t\ntable.unlock %t\nreturn', 'unlock without matching lock'),
        (prelude + 'table.lock %t\ntable.unlock %u\nreturn', 'unlock without matching lock'),
        (prelude + 'if %c, ^left, ^right\n^left:\ntable.lock %t\njump ^join\n'
         '^right:\njump ^join\n^join:\nreturn', 'table ownership'),
        (prelude + 'jump ^loop\n^loop:\ntable.lock %t\nif %c, ^loop, ^exit\n'
         '^exit:\ntable.unlock %t\nreturn', 'join/backedge'),
        (prelude + 'table.lock %t\njump ^loop\n^loop:\ntable.unlock %t\nif %c, ^loop, ^exit\n'
         '^exit:\nreturn', 'join/backedge'),
        (prelude + '%a: table.big = copy %t\n%b: table.big = copy %a\n'
         'table.lock %b\ntable.unlock %u\nreturn', 'unlock without matching lock'),
    ]
    with tempfile.TemporaryDirectory(prefix='lease flow ') as temporary:
        work = Path(temporary)
        source, output = work/'input.fir', work/'output'
        for body in valid:
            source.write_text(module(body))
            call([compiler, 'check', source], work)
            call([compiler, 'opt', source, output], work)
            call([compiler, 'check', output], work)
        # Execute a documented alias/join example in both optimization states.
        tutorial = Path(__file__).resolve().parents[1] / 'examples/tables/ownership-flow.fir'
        call([compiler, 'opt', tutorial, output], work)
        for selected in (tutorial, output):
            image = work/'flow.dis'
            call([compiler, 'dis', selected, image], work)
            if runtime:
                for quantum in (1, 2048):
                    result = call([runtime, mode, '--quantum', quantum, '--stats',
                                   '--inspect-words', 0, 2, image], work)
                    values = re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)
                    if values != ['42', '1'] or 'tables_live=0 keys_live=0' not in result.stdout + result.stderr:
                        raise RuntimeError('alias/join example failed: ' + result.stdout + result.stderr)
        for body, diagnostic in invalid:
            source.write_text(module(body))
            result = call([compiler, 'check', source], work, False)
            if diagnostic not in result.stderr:
                raise RuntimeError('wrong flow diagnostic: ' + result.stderr)
            for command in ('print', 'opt', 'dis', 'object'):
                output.write_bytes(b'KEEP')
                call([compiler, command, source, output], work, False)
                if output.read_bytes() != b'KEEP':
                    raise RuntimeError(command + ' published invalid ownership flow')
    print('PASS lease flow: recursive aliases, equal joins, balanced cycles and rejected partial outputs')


if __name__ == '__main__':
    main()
