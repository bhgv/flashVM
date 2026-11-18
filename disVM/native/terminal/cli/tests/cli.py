# Copyright (c) 2026 bhgv
# EN: Executable native CLI acceptance over inherited carriers and independent application S-record framing.
# RU: Проверки настоящего CLI через унаследованные carriers и независимое клиентское S-обрамление.
import os
import fcntl
import signal
import select
import socket
import subprocess
import sys
import tempfile
import zlib


# EN/RU: Independent canonical positive S-integer, final marker only in last byte.
# Независимое каноническое положительное S-число, признак конца лишь в последнем байте.
def integer(value):
    groups = []
    while value > 63:
        groups.insert(0, value & 127)
        value >>= 7
    groups.insert(0, value)
    groups[-1] |= 128
    return bytes(groups)


# EN/RU: Independent original S-record CRC/length, not the private ordered-packet prefix.
# Независимые CRC/длина исходной S-записи, не закрытый префикс нумерованного пакета.
def frame(tag, payload):
    body = integer(tag) + payload
    length = integer(len(body))
    mask = (1 << min(32, 6 + 7 * (len(length) - 1))) - 1
    return integer(zlib.crc32(length + body) & mask) + length + body


# EN/RU: Decode one checked positive canonical integer, return value/new offset or fail explicitly.
# Прочитать проверяемое положительное каноническое число, вернуть value/offset либо явную ошибку.
def read_integer(data, at):
    begin = at
    first = data[at]
    if first & 64:
        raise RuntimeError("unexpected negative application integer")
    value = first & 63
    while not data[at] & 128:
        at += 1
        value = (value << 7) | (data[at] & 127)
    at += 1
    if data[begin:at] != integer(value):
        raise RuntimeError("noncanonical output integer")
    return value, at


# EN/RU: Validate complete binary stdout; any leaked transport prefix/root service changes expected records.
# Проверить весь двоичный stdout; утечка transport-префикса/root-службы меняет ожидаемые записи.
def records(data):
    result = []
    at = 0
    while at < len(data):
        begin = at
        checksum, at = read_integer(data, at)
        length, body = read_integer(data, at)
        end = body + length
        tag, payload = read_integer(data, body)
        if end > len(data) or data[begin:end] != frame(tag, data[payload:end]):
            raise RuntimeError(f"invalid application stdout frame, checksum {checksum}")
        result.append((tag, data[payload:end]))
        at = end
    return result


# EN/RU: Start independent original server and actual CLI with one delegated socketpair end, always retire both children.
# Запустить независимый исходный server и настоящий CLI с socketpair-carrier, всегда завершить оба процесса.
def relay(cli, server, mode="normal", channel=0, payload=None, extra=(), quota=7, expected=0, limit=256, tags=(0, 7, 9)):
    local, remote = socket.socketpair()
    provider = subprocess.Popen([server, str(remote.fileno()), str(channel), mode], pass_fds=[remote.fileno()],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    remote.close()
    command = [cli, "--profile", "ordered", "--phy-fd", str(local.fileno()), "--channel", str(channel),
               "--limit", str(limit), "--window", "8", "--lane", "71",
               "--receive-records", str(quota), *extra]
    for tag in tags:
        command.extend(["--tag", str(tag)])
    try:
        result = subprocess.run(command, input=payload if payload is not None else frame(7, b"echo") + frame(0, b"hello"),
                                pass_fds=[local.fileno()], capture_output=True, timeout=12)
        local.close()
        out, errors = provider.communicate(timeout=8)
        if result.returncode != expected or provider.returncode or out:
            raise RuntimeError(f"CLI={result.returncode}, peer={provider.returncode}, stderr={result.stderr!r}/{errors!r}")
        if expected == 0 and result.stderr:
            raise RuntimeError(f"unexpected CLI diagnostic {result.stderr!r}")
        return result
    finally:
        local.close()
        if provider.poll() is None:
            provider.kill()
            provider.communicate()


# EN/RU: Check application order independently per tag, including restored sequence after reset and binary/local0 privacy.
# Проверить порядок по каждому тегу, включая восстановление после сброса и двоичные/local0 данные.
def ordered(cli, server, channel=0, extra=()):
    result = relay(cli, server, channel=channel, extra=extra)
    grouped = {}
    for tag, payload in records(result.stdout):
        grouped.setdefault(tag, []).append(payload)
    if grouped != {7: [b"zero", b"one", b"two", b"echo"], 9: [b"\0\xff"], 0: [b"client-zero", b"hello"]}:
        raise RuntimeError(f"application order/privacy mismatch: {grouped}")


# EN/RU: Invalid configuration cannot emit carrier bytes; none/help do not instantiate native connection.
# Неверная конфигурация не выдаёт байты carrier; none/help не создают native-соединение.
def preflight(cli):
    for args in [[], ["--profile", "none"], ["--help"]]:
        result = subprocess.run([cli, *args], input=b"unconsumed", capture_output=True, timeout=3)
        if result.returncode or result.stderr or ("--help" not in args and result.stdout):
            raise RuntimeError("none/help changed stdio or failed")
    left, right = socket.socketpair()
    base = ["--profile", "ordered", "--phy-fd", str(left.fileno()), "--lane", "71", "--tag", "7"]
    invalid = [base + ["--limit", "1", "--tag", "4294967295"], base + ["--lane", "72"], base + ["--tag", "7"], base + ["--tag", "-1"],
               base + ["--limit", "0"], base + ["--window", "9223372036854775808"],
               base + ["--window", "01"], base + ["--rx", "../hidden"], base + ["--channel", "2"],
               ["--profile", "none", "--phy-fd", str(left.fileno())], ["--phy-fd", str(left.fileno())],
               base + ["--unknown", "x"], base + ["--tx"]]
    try:
        for args in invalid:
            result = subprocess.run([cli, *args], input=b"", capture_output=True, timeout=3, pass_fds=[left.fileno()])
            if result.returncode != 2 or result.stdout:
                raise RuntimeError(f"invalid arguments accepted: {args}")
        right.setblocking(False)
        try:
            leaked = right.recv(1)
        except BlockingIOError:
            leaked = None
        if leaked is not None:
            raise RuntimeError("invalid CLI touched delegated carrier")
    finally:
        left.close()
        right.close()
    with tempfile.TemporaryFile() as file:
        file.write(b"private-file-marker")
        file.flush()
        result = subprocess.run([cli, "--profile", "ordered", "--phy-fd", str(file.fileno()), "--lane", "71", "--tag", "0"],
                                input=b"", capture_output=True, pass_fds=[file.fileno()], timeout=3)
        file.seek(0)
        if result.returncode != 2 or file.read() != b"private-file-marker" or result.stdout:
            raise RuntimeError("ordinary file treated as carrier")


# EN/RU: Reject carrier/stdout alias before wire I/O, and retire real native owners on a broken output pipe.
# Отвергнуть alias carrier/stdout до wire I/O; при сломанной output-pipe завершить настоящие native-owners.
def stdio_failures(cli, server):
    local, remote = socket.socketpair()
    command = [cli, "--profile", "ordered", "--phy-fd", str(local.fileno()), "--lane", "71", "--tag", "7"]
    try:
        result = subprocess.run(command, input=b"", stdout=local, stderr=subprocess.PIPE,
                                pass_fds=[local.fileno()], timeout=3)
        if result.returncode != 2:
            raise RuntimeError("carrier/stdout alias accepted")
        remote.setblocking(False)
        try:
            leaked = remote.recv(1)
        except BlockingIOError:
            leaked = None
        if leaked is not None:
            raise RuntimeError("aliased stdio leaked carrier bytes")
    finally:
        local.close()
        remote.close()
    local, remote = socket.socketpair()
    read_fd, write_fd = os.pipe()
    os.close(read_fd)
    before = fcntl.fcntl(write_fd, fcntl.F_GETFL)
    provider = subprocess.Popen([server, str(remote.fileno()), "0", "quiet"], pass_fds=[remote.fileno()],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    remote.close()
    command = [cli, "--profile", "ordered", "--phy-fd", str(local.fileno()), "--lane", "71",
               "--tag", "0", "--tag", "7", "--tag", "9", "--limit", "256", "--window", "8"]
    try:
        result = subprocess.run(command, input=b"", stdout=write_fd, stderr=subprocess.PIPE,
                                pass_fds=[local.fileno()], timeout=8)
        local.close()
        out, errors = provider.communicate(timeout=8)
        if result.returncode != 2 or provider.returncode or out or errors or fcntl.fcntl(write_fd, fcntl.F_GETFL) != before:
            raise RuntimeError("broken stdout cleanup or flag restoration failed")
    finally:
        local.close()
        os.close(write_fd)
        if provider.poll() is None:
            provider.kill()
            provider.communicate()


# EN/RU: Keep stdout full, then interrupt: native cancellation must join workers and restore shared output flags.
# Удерживать stdout полным и прервать: native-отмена должна завершить workers и восстановить output-flags.
def interrupted(cli, server, control_signal):
    local, remote = socket.socketpair()
    read_fd, write_fd = os.pipe()
    fcntl.fcntl(write_fd, fcntl.F_SETPIPE_SZ, 4096)
    before = fcntl.fcntl(write_fd, fcntl.F_GETFL)
    provider = subprocess.Popen([server, str(remote.fileno()), "0", "flood"], pass_fds=[remote.fileno()],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    remote.close()
    command = [cli, "--profile", "ordered", "--phy-fd", str(local.fileno()), "--lane", "71", "--tag", "7",
               "--limit", "256", "--window", "8"]
    child = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=write_fd, stderr=subprocess.PIPE, pass_fds=[local.fileno()])
    try:
        if not select.select([provider.stderr], [], [], 10)[0] or provider.stderr.readline() != b"delivered-flood\n":
            raise RuntimeError("broker stalled behind stdout backpressure")
        child.send_signal(control_signal)
        status = child.wait(timeout=8)  # Deliberately do not drain stdout before native shutdown completes.
        if status != 128 + control_signal or fcntl.fcntl(write_fd, fcntl.F_GETFL) != before:
            raise RuntimeError(f"interruption cleanup failed: {status}, {child.stderr.read()!r}")
        local.close()
        out, errors = provider.communicate(timeout=8)
        if provider.returncode or out or errors:
            raise RuntimeError(f"peer interruption teardown failed: {errors!r}")
    finally:
        if child.poll() is None:
            child.kill()
        child.communicate()
        if provider.poll() is None:
            provider.kill()
            provider.communicate()
        local.close()
        os.close(write_fd)
        os.close(read_fd)


# EN/RU: Actual executable suite, no shell, real original Styx plus native workers; bounded child lifetimes.
# Настоящий executable-набор без shell: исходный Styx и native-workers, ограниченная жизнь дочерних процессов.
def main():
    cli, server = sys.argv[1:3]
    if len(sys.argv) > 3 and sys.argv[3] == "eof-probe":
        result = relay(cli, server, mode="eof-single", payload=b"", quota=0)
        if records(result.stdout) != [(7, b"last")]:
            raise RuntimeError("clean EOF overtook the final accepted application record")
        print("PASS deterministic accepted-record drain and startup observation after clean EOF")
        return
    if len(sys.argv) > 3 and sys.argv[3] == "rollback":
        for scenario in ["allocation", "watch", "1", "2", "3", "4", "5"]:
            os.environ["UNIBUS_CLI_TEST_FAULT"] = scenario
            relay(cli, server, mode="quiet", payload=b"", quota=100, expected=2)
        print("PASS executable CLI allocation/watch/worker rollback")
        return
    preflight(cli)
    stdio_failures(cli, server)
    ordered(cli, server)
    ordered(cli, server, channel=1)
    ordered(cli, server, extra=["--root", "records", "--rx", ".", "--tx", "."])
    for payload in [frame(8, b"forbidden"), frame(7, b"echo")[:-1], bytes([1, 2, 3])]:
        relay(cli, server, mode="quiet", payload=payload, quota=100, expected=2)
    relay(cli, server, mode="badack", payload=b"", quota=5, expected=2)
    tiny = relay(cli, server, mode="tiny", payload=b"", quota=1, limit=1, tags=(0,))
    if records(tiny.stdout) != [(0, b"")]:
        raise RuntimeError("tiny client limit incorrectly constrained service capacity")
    quiet = relay(cli, server, mode="quiet", payload=b"", quota=5)
    if len(records(quiet.stdout)) != 5:
        raise RuntimeError("stdin EOF stopped independent RX")
    eof = relay(cli, server, mode="eof", payload=b"", quota=0)
    if len(records(eof.stdout)) != 5:
        raise RuntimeError("clean peer EOF dropped accepted data")
    interrupted(cli, server, signal.SIGINT)
    interrupted(cli, server, signal.SIGTERM)
    print("PASS executable CLI profiles, delegated carriers, client privacy/order, renewal, EOF and invalid input")


if __name__ == "__main__":
    main()
