# Copyright (c) 2026 bhgv
# EN: Black-box gateway edge cases: independent endpoints, failures and exact shutdown behavior.
# RU: Black-box edge cases шлюза: разные endpoints, ошибки и точное поведение shutdown.
import subprocess
import socket
import sys
from gateway_harness import Backend, Gateway, frame, require, record


# EN: binary is borrowed executable path; require usage errors without any backend connection/output bytes.
# RU: binary — путь executable; требовать usage errors без backend connections и байтов stdout.
def arguments(binary):
    for argv in [[], ['0'], ['65536'], ['+1'], [' 1'], ['1x'], ['9' * 80], ['1', '0'], ['1', '2', '3']]:
        result = subprocess.run([binary, *argv], stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, timeout=5)
        require(result.returncode == 1 and not result.stdout and b'usage:' in result.stderr,
                f'invalid CLI accepted: {argv!r}, {result.returncode}, {result.stderr!r}')


# EN: Start client 7, then refuse client 8 on a bound non-listening port. The already
# started worker must retire; no successful service handshake may leak to stdout.
# RU: Запустить клиента 7, отказать клиенту 8 на bound non-listening порту. Уже
# запущенный worker должен завершиться; успешный handshake не должен попасть в stdout.
def partial_start(binary):
    first = Backend(b'first')
    try:
        with socket.socket() as refused:
            refused.bind(('127.0.0.1', 0))
            result = subprocess.run([binary, str(first.port), str(refused.getsockname()[1])],
                                    stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=6)
            require(result.returncode == 2 and not result.stdout and b'backend connect' in result.stderr,
                    f'partial startup did not retire cleanly: {result.returncode}, {result.stderr!r}')
    finally:
        first.close()


# EN: Use distinct real ports and identical inner IDs; verify replies came from the correct backend.
# binary is borrowed; all processes/threads/sockets belong to Gateway's context.
# RU: Разные реальные порты, одинаковые inner IDs; проверить правильный backend.
# binary заимствован; context Gateway владеет процессами/потоками/socket.
def endpoints(binary):
    with Gateway(binary) as gateway:
        gateway.send(7, b'one')
        gateway.send(8, b'two')
        replies = {reply[0]: reply for reply in (gateway.receive(), gateway.receive())}
        require(replies[7][1:] == (1, frame(b'left:one', kind=101)), 'wrong left backend')
        require(replies[8][1:] == (1, frame(b'right:two', kind=101)), 'wrong right backend')
        gateway.finish()


# EN: Keep client 7's RPC actually pending until client 8's reply arrives, then drain after RX EOF.
# binary is borrowed. Event gates prove ordering without scheduler sleeps or guessed wait admission.
# RU: Удерживать RPC клиента 7 до ответа клиенту 8, затем drain после EOF RX.
# binary заимствован. Event gates доказывают порядок без sleep/догадок о scheduler.
def pending_and_drain(binary):
    with Gateway(binary) as gateway:
        gateway.send(7, b'stall')
        require(gateway.backends[0].gate.wait(5), 'client 7 never reached its backend')
        gateway.send(8, b'progress')
        require(gateway.receive() == (8, 1, frame(b'right:progress', kind=101)), 'pending client blocked another')
        gateway.end_input()
        gateway.backends[0].release.set()
        require(gateway.receive() == (7, 1, frame(b'left:stall', kind=101)), 'EOF cancelled accepted work')
        gateway.finish()


# EN: Invalid inner payloads on known client 7 must not reach a backend or poison client 8/the carrier.
# binary is borrowed; require per-client FIFO order, including error markers and the later valid request.
# RU: Неверные inner payloads клиента 7 не должны попасть в backend/сломать клиента 8/носитель.
# binary заимствован; требовать FIFO клиента, включая ошибки и последующий правильный запрос.
def malformed_client(binary):
    with Gateway(binary) as gateway:
        for payload in [b'\x80', record(9, frame(b'wrong-inner')), record(1, b'\x08\x00\x00\x00\x64\x01\x00')]:
            gateway.peer.send(record(7, payload))
        gateway.send(7, b'valid')
        gateway.send(8, b'valid')
        clients = {7: [], 8: []}
        for _ in range(5):
            client, inner, data = gateway.receive()
            clients[client].append((inner, data))
        require([entry[0] for entry in clients[7]] == [2, 2, 2, 1], 'client error/reply FIFO changed')
        require(clients[8] == [(1, frame(b'right:valid', kind=101))], 'malformed peer poisoned client 8')
        require(gateway.backends[0].requests == [b'valid'], 'invalid client frame reached backend')
        gateway.finish()


# EN: A normal 9P Rerror is opaque application traffic, not a broken backend stream.
# binary is borrowed; prove the same connection remains usable for a later request.
# RU: Обычный 9P Rerror — непрозрачные данные приложения, не разрыв backend stream.
# binary заимствован; доказать работоспособность того же connection следующим запросом.
def application_error(binary):
    with Gateway(binary) as gateway:
        gateway.send(7, b'rerror')
        require(gateway.receive() == (7, 1, frame(b'\x06\x00denied', kind=107)), 'Rerror was reclassified as carrier failure')
        gateway.send(7, b'after-error')
        require(gateway.receive() == (7, 1, frame(b'left:after-error', kind=101)), 'Rerror poisoned a healthy connection')
        require(gateway.backends[0].requests == [b'rerror', b'after-error'], 'application error changed backend session')
        gateway.finish()


# EN: Consume one backend request then lose/truncate/reject its reply; future requests on that client
# must receive private errors without another effect. binary/cause are borrowed test inputs.
# RU: Backend принимает запрос, затем теряется/усекается/неверен ответ; последующие запросы клиента
# получают собственные ошибки без нового effect. binary/cause — входы теста.
def backend_failure(binary, cause):
    with Gateway(binary) as gateway:
        gateway.send(7, cause)
        client, inner, _ = gateway.receive()
        require((client, inner) == (7, 2), 'backend failure was not client-local')
        gateway.send(7, b'never-replay-1')
        gateway.send(7, b'never-replay-2')
        gateway.send(8, b'survives')
        replies = [gateway.receive() for _ in range(3)]
        require(sum(client == 7 and inner == 2 for client, inner, _ in replies) == 2, 'sticky failure missing')
        require((8, 1, frame(b'right:survives', kind=101)) in replies, 'other backend lost progress')
        require(gateway.backends[0].requests == [cause], 'failed/uncertain operation was replayed')
        gateway.finish()


# EN: Invalid outer framing/unknown service routing is carrier-level failure, unlike malformed client data.
# binary/payload are borrowed. No client operation should have reached either backend.
# RU: Неверный outer frame/маршрут — carrier-level failure, в отличие от неверных данных клиента.
# binary/payload заимствованы. В backend не должна попасть ни одна операция.
def carrier_failure(binary, payload):
    with Gateway(binary) as gateway:
        gateway.peer.send(payload)
        gateway.finish(2)
        require(all(not backend.requests for backend in gateway.backends), 'bad carrier caused backend effects')


# EN: Close stdout while keeping stdin open; a worker's EPIPE must wake idle RX and exit with failure.
# binary is borrowed; pending also holds another backend RPC. RX-only waiting or missing socket shutdown would hang.
# RU: Закрыть stdout, оставив stdin открытым; EPIPE worker должен разбудить idle RX и дать отказ.
# binary заимствован; pending удерживает другой RPC. RX-only wait/пропущенный socket shutdown дали бы зависание.
def output_loss(binary, pending=False):
    with Gateway(binary) as gateway:
        if pending:
            gateway.send(7, b'stall')
            require(gateway.backends[0].gate.wait(5), 'pending backend did not start before output failure')
        gateway.process.stdout.close()
        gateway.process.stdout = None
        gateway.send(8 if pending else 7, b'reply-to-closed-output')
        require(gateway.process.wait(timeout=5) == 2, 'output loss did not wake RX/interrupt pending backend I/O')
        gateway.backends[0].release.set()
        gateway.finish(2)


# EN: CLI binary selects the real built gateway. Run independent bounded fixtures; return via process status.
# RU: CLI binary выбирает собранный шлюз. Выполнить независимые bounded fixtures; результат — process status.
def main():
    binary = sys.argv[1]
    arguments(binary)
    partial_start(binary)
    endpoints(binary)
    pending_and_drain(binary)
    malformed_client(binary)
    application_error(binary)
    for cause in (b'fail', b'partial', b'oversize'):
        backend_failure(binary, cause)
    for payload in (b'\x80', b'\x80\x80', record(9, record(1, frame(b'unknown-client')))):
        carrier_failure(binary, payload)
    output_loss(binary)
    output_loss(binary, True)
    print('PASS gateway edges: CLI, two ports, pending RPC, EOF drain, private errors, sticky failure and output wake')


if __name__ == '__main__':
    main()
