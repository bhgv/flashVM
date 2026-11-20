# Copyright (c) 2026 bhgv
# EN: Generate from .m, compile real Dis and call across two isolated VM processes.
# RU: Генерировать из .m, собрать настоящий Dis, вызвать между двумя VM processes.
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from network_proxy_fixture import Guest, connected_tcp
from test_owner_location import compile_image


# EN: Borrow tool paths; own temporary server/client directories and both VMs.
# Only the server receives implementation.dis and its private file. Test C/JIT
# and both polling paths; every child is joined/aborted before directory cleanup.
# RU: Заимствовать tool paths, владеть server/client directories и обеими VM.
# Только сервер получает implementation.dis/private file. Проверить C/JIT и оба
# polling paths, join/abort children до удаления temporary directories.
def main():
    generator, runtime, compiler, include = [Path(value).resolve() for value in sys.argv[1:]]
    root = Path(__file__).resolve().parent
    fixture = root.parent / 'examples/remote-scalar'
    with tempfile.TemporaryDirectory(prefix='generated-remote-') as temporary:
        work = Path(temporary)
        server = work / 'provider'
        client = work / 'caller'
        server.mkdir()
        client.mkdir()
        shutil.copyfile(fixture / 'compute.m', server / 'compute.m')
        generated = server / 'handler.b'
        subprocess.run([str(generator), str(server / 'compute.m'), 'compute.m',
                        'implementation.dis', str(generated)], check=True)
        client_interface = client / 'compute-remote.m'
        client_source = client / 'compute-remote.b'
        subprocess.run([str(generator), '--client-interface', str(server / 'compute.m'),
                        str(client_interface)], check=True)
        subprocess.run([str(generator), '--client', str(server / 'compute.m'),
                        client_interface.name, str(client_source)], check=True)
        for name in ('proxyref', 'proxyowner', 'proxyframe', 'proxystream', 'proxynetio',
                     'proxynet', 'proxyregistry', 'proxyserver', 'proxyroute'):
            compile_image(compiler, include, root, root.parent / 'limbo' / (name + '.b'),
                          server / (name + '.dis'))
            shutil.copyfile(server / (name + '.dis'), client / (name + '.dis'))
        compile_image(compiler, include, root, generated, server / 'handler.dis', (server,))
        compile_image(compiler, include, root, fixture / 'privatevalue.b',
                      server / 'privatevalue.dis', (fixture,))
        compile_image(compiler, include, root, fixture / 'implementation.b',
                      server / 'implementation.dis', (fixture,))
        compile_image(compiler, include, root, root / 'fixtures/proxy-network/server.b', server / 'server.dis')
        compile_image(compiler, include, root, client_source, client / 'compute-remote.dis', (client,))
        compile_image(compiler, include, root, fixture / 'client.b', client / 'client.dis', (client,))
        (server / 'provider-secret').write_text('41')
        for mode in ('--no-jit', '--jit'):
            for locked in (False, True):
                left, right = connected_tcp()
                guests = []
                try:
                    tag = mode[2:] + ('-locked' if locked else '-fast')
                    guests.append(Guest(runtime, server, 'server', mode, locked, left, tag))
                    guests.append(Guest(runtime, client, 'client', mode, locked, right, tag))
                    left.close()
                    right.close()
                    guests[1].finish('PASS generated scalar owner: provider code resource state error recovery')
                    guests[0].finish('result=0 value=10')
                    print('PASS generated remote .m owner:', tag, flush=True)
                finally:
                    left.close()
                    right.close()
                    for guest in guests:
                        guest.abort()


if __name__ == '__main__':
    main()
