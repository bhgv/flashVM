# Copyright (c) 2026 bhgv
# EN: Matched producer and isolated real-VM lifecycle for network owner acceptance.
# RU: Matching producer и isolated real-VM lifecycle для network owner acceptance.
import re
import socket
import subprocess

from resident_evidence import evidence


# EN/RU: Compile production libraries and fixture variants with the exact native producer.
# Собрать production libraries и fixture variants точным native producer.
def compile_fixture(root, work, compiler, include):
    sources = [
        (root.parent / "limbo" / (name + ".b"), name)
        for name in (
            "proxyref",
            "proxyowner",
            "proxyframe",
            "proxystream",
            "proxynetio",
            "proxynet",
            "proxyregistry",
            "proxyserver",
            "proxyroute",
        )
    ]
    sources += [
        (root / "fixtures/proxy-network" / (name + ".b"), name)
        for name in (
            "handler",
            "server",
            "client",
            "failure-client",
            "orphan-io",
            "orphan-server",
            "greeting-client",
            "socket-output",
            "idle-client",
            "scalar-results",
            "gc-client",
            "result-client",
            "routed-client",
            "queued-deadline-client",
            "owner-deadline-client",
            "relay-server",
            "relay-client",
            "relay-churn-client",
            "relay-orphan-client",
            "relay-capacity-client",
            "relay-deadline-client",
            "relay-source-loss-client",
            "relay-caller-loss-client",
            "routed-failure-client",
            "result-quota-client",
            "result-failure-client",
            "result-capacity-client",
            "orphan-result",
            "lease-native",
            "lease-refusal",
        )
    ]
    for source, name in sources:
        compile_one(root, work, compiler, include, source, name)
    old = (root / "fixtures/proxy-network/server.b").read_text()
    alternate = work / "new-incarnation.b"
    alternate.write_text(old.replace("big 67890", "big 67891"))
    compile_one(root, work, compiler, include, alternate, "new-incarnation")
    handler = (root / "fixtures/proxy-network/handler.b").read_text()
    anchor = "    left.value += value;"
    if handler.count(anchor) != 1:
        raise RuntimeError("queued owner handler anchor moved")
    gate = """    if (value == -777) {
        entered := sys->create("owner-entered", Sys->OWRITE, 8r666);
        if (entered == nil) {
            raise "owner deadline entry publication";
        }
        entered = nil;
        attempts := 0;
        released := sys->open("owner-release", Sys->OREAD);
        while (released == nil && attempts++ < 2000) {
            sys->sleep(5);
            released = sys->open("owner-release", Sys->OREAD);
        }
        if (released == nil) {
            raise "owner deadline gate timeout";
        }
        return left.value;
    }
"""
    queued = work / "queued-handler.b"
    queued.write_text(handler.replace(anchor, gate + anchor))
    compile_one(root, work, compiler, include, queued, "queued-handler")
    routed = work / "routed-server.b"
    routed.write_text(old.replace(", 5000, handler", ", 45000, handler"))
    compile_one(root, work, compiler, include, routed, "routed-server")


# EN/RU: Compile one source into work; diagnostics are failures, never semantic controls.
# Собрать source в work; diagnostics — отказ, не semantic control.
def compile_one(root, work, compiler, include, source, name):
    result = subprocess.run(
        [
            str(compiler),
            "-I",
            str(include),
            "-I",
            str(root.parent / "module"),
            "-o",
            str(work / (name + ".dis")),
            str(source),
        ],
        capture_output=True,
        text=True,
    )
    if result.returncode or result.stderr:
        raise RuntimeError("network fixture compile failed\n" + result.stdout + result.stderr)


# EN: Own subprocess and diagnostic log; socket is independently retained by the
# child on stdin/stdout. Join/kill before reading logs or releasing fixture files.
# RU: Владеть subprocess/log; child удерживает socket в stdin/stdout. Join/kill
# предшествует чтению log и удалению fixture files.
class Guest:

    # EN: Start runtime/fixture name on mode/locked polling with borrowed endpoint;
    # work owns files, tag selects an independent diagnostic log. Own process/log.
    # RU: Запустить runtime/name с mode/locked и borrowed endpoint; work владеет
    # файлами, tag выбирает отдельный diagnostic log. Владеть process/log.
    def __init__(self, runtime, work, name, mode, locked, endpoint, tag, output=None):
        self.path = work / (tag + ".log")
        self.log = self.path.open("wb")
        self.process = subprocess.Popen(
            [
                str(runtime),
                "--stats",
                mode,
                *(["--gc-locked-poll"] if locked else []),
                str(work / (name + ".dis")),
            ],
            cwd=work,
            stdin=endpoint,
            stdout=endpoint if output is None else output,
            stderr=self.log,
        )

    # EN/RU: Join real VM, require clean status, resident lifetimes and concurrent GC evidence.
    # Join настоящей VM, проверить status, resident lifetimes и concurrent GC.
    # EN/RU: 90s join budget covers instrumented clone/call/drop resident lifetimes;
    # wire deadlines stay unchanged. 90s budget учитывает instrumented lifetimes,
    # не меняя wire deadlines.
    def finish(self, expected=None):
        try:
            status = self.process.wait(timeout=90)
        except subprocess.TimeoutExpired:
            self.abort()
            raise RuntimeError("network VM hung\n" + self.path.read_text())
        self.log.close()
        output = self.path.read_text()
        if status or "ThreadSanitizer:" in output or "runtime error:" in output:
            raise RuntimeError("network VM failed\n" + output[-18000:])
        if expected and expected not in output:
            raise RuntimeError("missing network result " + expected + "\n" + output[-16000:])
        children = len(re.findall(r"NATIVEGUEST start pid=", output))
        evidence(output, children=children)
        if not re.search(
            r"NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0",
            output,
        ):
            raise RuntimeError("network owner roots were not drained\n" + output[-12000:])
        if "PROXYGC" in output:
            count = re.search(
                r"PROXYGC queues=0 tokens=0 pending=0 enqueued=(\d+) drained=(\d+) discarded=(\d+)",
                output,
            )
            if not count or int(count[1]) != int(count[2]) + int(count[3]):
                raise RuntimeError("native image ownership was not drained\n" + output[-12000:])
        if "PROXYHOST" in output and not re.search(
            r"PROXYHOST opened=([1-9][0-9]*) closed=\1 exports=0 active=0", output
        ):
            raise RuntimeError("network exports or admitted calls leaked\n" + output[-12000:])
        return output

    # EN/RU: Dispose unfinished child and log on every exception path; idempotent.
    # Удалить unfinished child и log на любом exception path; идемпотентно.
    def abort(self):
        if self.process.poll() is None:
            self.process.kill()
        self.process.wait()
        self.log.close()


# EN/RU: Establish actual trusted loopback TCP endpoints; caller owns both sockets.
# Создать настоящие trusted loopback TCP endpoints; caller владеет обоими socket.
def connected_tcp():
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        left = socket.create_connection(listener.getsockname(), timeout=10)
        right, _ = listener.accept()
    for endpoint in (left, right):
        endpoint.settimeout(None)
        endpoint.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    return left, right
