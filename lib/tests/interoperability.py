# Copyright (c) 2026 bhgv
# EN: Join separately built external libraries only at their wire boundary; no shared interpreter or linked SDK.
# RU: Соединить отдельно собранные внешние библиотеки только по wire; без общего интерпретатора и линковки SDK.
from pathlib import Path
import subprocess
import sys
import tempfile

LIB = Path(__file__).resolve().parents[1]  # Workspace test location, never a library build dependency.
sys.path.insert(0, str(LIB / "libunibus/examples/bridge"))
sys.path.insert(0, str(LIB / "styx/examples"))
from check import launch, stop
from wire_client import Client, require
from wire import Peer, bindings, record, unpack


# EN/RU: Adapt one client-owned virtual socket to a nested Unibus lane while retaining its own unread reply bytes.
# Адаптировать виртуальный socket одного клиента к вложенному Unibus-каналу, удерживая его непрочитанные байты.
class Lane:
    # EN/RU: Borrow the shared peer/router through both client lifetimes; tag identifies only the outer client.
    # Заимствовать общий peer/router до завершения клиентов; tag задаёт только внешнего клиента.
    def __init__(self, router, tag):
        self.router = router
        self.tag = tag
        self.pending = b""

    # EN/RU: Send an opaque standard 9P message inside application tag1 and this client's outer tag.
    # Передать непрозрачное стандартное 9P-сообщение внутри application tag1 и внешнего tag клиента.
    def sendall(self, data):
        self.router.peer.send(record(self.tag, record(1, data)))

    # EN/RU: Receive only this lane, saving other clients' checked replies in their own queues.
    # Получить только свой канал, сохраняя проверенные ответы других клиентов в их очередях.
    def recv(self, size):
        while not self.pending:
            if self.router.queues[self.tag]:
                self.pending = self.router.queues[self.tag].pop(0)
                break
            tag, body = self.router.peer.receive()
            inner, data = unpack(body)
            require(tag in self.router.queues and inner == 1, "gateway reply namespace")
            self.router.queues[tag].append(data)
        result, self.pending = self.pending[:size], self.pending[size:]
        return result

    # EN/RU: A lane does not own the shared process/pipe; its router closes them after all clients retire.
    # Канал не владеет общим процессом/трубой; router закрывает их после завершения всех клиентов.
    def close(self):
        pass


# EN/RU: Reuse only the independent standard-9P oracle, with transport replaced by a client-isolated lane.
# Использовать лишь независимый стандартный 9P oracle, заменив транспорт изолированным каналом клиента.
class NestedClient(Client):
    # EN/RU: Start separate fid/tag spaces even when two clients use identical inner IDs.
    # Создать отдельные fid/tag-пространства даже при одинаковых внутренних ID клиентов.
    def __init__(self, router, tag):
        self.socket = Lane(router, tag)
        self.tag = 0
        self.fid = 1
        import struct
        from wire_client import text
        self.rpc(100, struct.pack("<I", 8192) + text("9P2000"))
        self.rpc(104, struct.pack("<II", 1, 0xffffffff) + text("demo") + text(""))


# EN/RU: Own the wire-only gateway with two independently queued clients.
# Владеть wire-only шлюзом с двумя независимо поставленными в очереди клиентами.
class Router:
    # EN/RU: Start gateway to the supplied external server port and require its exact stock binding declaration.
    # Запустить шлюз к заданному внешнему порту и требовать точное штатное объявление binding.
    def __init__(self, binary, port):
        self.process = subprocess.Popen([str(binary), str(port)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.peer = Peer(self.process)
        self.queues = {7: [], 8: []}
        require(self.peer.receive() == (0, bindings()), "stock tag0 binding declaration")
        self.peer.send(record(0, bindings()))

    # EN/RU: Drain/join all gateway workers before backend server shutdown; reject sanitizer or protocol failures.
    # Освободить/join всех workers шлюза до завершения backend; отклонить ошибки санитайзера или протокола.
    def close(self):
        self.process.stdin.close()
        self.process.stdin = None
        try:
            _, error = self.process.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            self.process.kill()
            self.process.communicate()
            raise RuntimeError("gateway workers did not join")
        require(self.process.returncode == 0 and b"ERROR: AddressSanitizer" not in error and b"runtime error:" not in error, error.decode())


# EN/RU: Run every file-server model through real original Styx plus a separately linked Unibus-only gateway.
# Запустить все модели файлового сервера через настоящий исходный Styx и отдельно связанный Unibus-only шлюз.
def main():
    styx_build, unibus_build = (Path(value).resolve() for value in sys.argv[1:])
    for kind in ("flat", "tree", "database", "live", "http"):
        helper = None
        server = None
        router = None
        with tempfile.TemporaryDirectory(prefix="link-interop-") as temporary:
            source = Path(temporary) / "keys.tsv"
            source.write_text("alpha\tfirst\nbeta\tsecond\n")
            command = [str(styx_build / "examples" / ("styx-" + kind)), "0"]
            try:
                if kind == "database":
                    command.append(str(source))
                if kind == "http":
                    helper, port = launch([sys.executable, str(LIB / "styx/examples/http_source.py")])
                    command.append(f"http://127.0.0.1:{port}/node")
                server, port = launch(command)
                router = Router(unibus_build / "examples/unibus-9p-bridge", port)
                a = NestedClient(router, 7)
                b = NestedClient(router, 8)
                if kind == "flat":
                    require(a.file("status") == "ready\n" and b.file("temperature") == "21.5\n", "flat bridge")
                elif kind == "tree":
                    require(a.file("devices/sensor/temperature") == "21.5\n" and b.directory("system") == ["info"], "tree bridge")
                elif kind == "database":
                    require(a.file("alpha") == "first", "database bridge")
                    source.write_text("gamma\tthird\n")
                    require(b.directory("/") == ["gamma"] and a.file("gamma") == "third", "dynamic database bridge")
                elif kind == "live":
                    a.write("ctl", "add")
                    require("window-103" in b.directory("/"), "live registry bridge")
                    b.write("ctl", "remove 100")
                    require("window-100" not in a.directory("/"), "live registry removal bridge")
                else:
                    path = "Exampleland/city-2/district-3/street-4/house-5/room-6/info"
                    require(a.file(path) == "Synthetic information for room-6" and "DemoRepublic" in b.directory("/"), "lazy HTTP bridge")
                print("PASS independently linked Unibus/Styx " + kind, flush=True)
            finally:
                if router:
                    router.close()
                if server:
                    stop(server)
                if helper:
                    stop(helper)


if __name__ == "__main__":
    main()
