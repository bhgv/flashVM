# Copyright (c) 2026 bhgv
# EN: Hold selected original replies on the reverse 9P lane of the stock paired runtimes.
# RU: Удерживать выбранные исходные ответы на встречном канале 9P штатной пары.
from pathlib import Path
import socket
import struct
import subprocess
import threading
import time

from run import stream_pair, verify_exchange

LANE = 1  # DisVM serves the shared namespace on the reverse lane.
DATA = 0  # P9MX data frame type.
PAYLOAD = 1024  # P9MX maximum data payload.
STOP_BUDGET = 3.0  # Less than the paired client's ordinary five-second deadline.


# EN: Own the test's path/tag observations under one lock shared by two relay threads.
# RU: Хранить наблюдения путей и тегов под одним замком двух потоков ретрансляции.
class Gate:
    stages = ((114, "created-by-cancel"), (116, "hold-dir"), (122, "remove-by-cancel"),
              (124, "attr-cancel"), (112, "open-cancel"))  # Expected ordinary 9P request sequence.

    # EN: Borrow the fixture directory until relay threads have joined.
    # RU: Заимствовать каталог теста до завершения потоков ретрансляции.
    def __init__(self, directory):
        self.directory = Path(directory)
        self.lock = threading.Lock()
        self.paths = {}
        self.selected = []
        self.pending = {}
        self.held = None
        self.held_since = 0.0
        self.flush_tags = {}
        self.flushed = []
        self.errors = []
        self.directory_fid = None
        self.recovered = False

    # EN: Decode names needed by the fixture and record each exact selected tag.
    # RU: Декодировать нужные тесту имена и запомнить точный тег выбранного запроса.
    def request(self, kind, tag, frame):
        if kind == 108:
            original, = struct.unpack_from("<H", frame, 7)
            if self.held != original:
                raise RuntimeError(f"unexpected Tflush for tag {original}")
            if time.monotonic() - self.held_since >= STOP_BUDGET:
                raise RuntimeError(f"task let tag {original} reach the session deadline")
            self.flush_tags[tag] = self.pending.pop(original)
            self.held = None
            return frame
        if kind == 110:
            _, newfid, count = struct.unpack_from("<IIH", frame, 7)
            at = 17
            for _ in range(count):
                size, = struct.unpack_from("<H", frame, at)
                self.paths[newfid] = frame[at + 2 : at + 2 + size].decode("utf-8")
                at += 2 + size
            return frame
        return self.select_request(kind, tag, frame)

    # EN: Record the selected mutation or read and recognize recovery on a new fid.
    # RU: Учесть выбранное изменение или чтение и распознать восстановление на новом fid.
    def select_request(self, kind, tag, frame):
        if kind not in (112, 114, 116, 122, 124):
            return frame
        fid, = struct.unpack_from("<I", frame, 7)
        if kind == 114:
            size, = struct.unpack_from("<H", frame, 11)
            name = frame[13 : 13 + size].decode("utf-8")
        else:
            name = self.paths.get(fid)
        pair = kind, name
        if pair == self.stages[1]:
            if self.directory_fid is not None and self.directory_fid != fid and pair in self.flushed:
                self.recovered = True
                return frame
            self.directory_fid = fid
        if pair in self.stages:
            if len(self.selected) >= len(self.stages) or pair != self.stages[len(self.selected)]:
                raise RuntimeError(f"operation out of order or replayed: {pair}")
            self.selected.append(pair)
            self.pending[tag] = pair
        return frame

    # EN: Discard only the selected original acknowledgement; pass genuine Rflush.
    # Signal the movie after the provider has completed the original operation.
    # RU: Удержать только выбранное подтверждение и пропустить настоящий Rflush.
    # Сигнализировать фильму после завершения исходной операции провайдером.
    def response(self, kind, tag, frame):
        if kind == 109 and tag in self.flush_tags:
            self.flushed.append(self.flush_tags.pop(tag))
        if tag not in self.pending:
            return frame
        expected, name = self.pending[tag]
        if kind != expected + 1 or self.held is not None:
            raise RuntimeError(f"wrong reply for {name}: {kind}")
        self.held = tag
        self.held_since = time.monotonic()
        with (self.directory / "control-gate").open("r+b", buffering=0) as ready:
            ready.write(bytes([ord("0") + len(self.selected)]))
        return None

    # EN: Forward ordinary frames untouched except for the explicit reply gate.
    # RU: Передать обычные кадры без изменений, кроме выбранного подтверждения.
    def select(self, frame, request):
        kind, tag = struct.unpack_from("<BH", frame, 4)
        with self.lock:
            return self.request(kind, tag, frame) if request else self.response(kind, tag, frame)


# EN: Receive exactly size bytes, allowing clean EOF only before a new unit.
# RU: Принять ровно size байт, разрешая штатный EOF только перед новым блоком.
def read_exact(stream, size):
    data = bytearray()
    while len(data) < size:
        chunk = stream.recv(size - len(data))
        if not chunk:
            if data:
                raise RuntimeError("partial carrier frame")
            return None
        data.extend(chunk)
    return bytes(data)


# EN: Reassemble only the inspected lane's original frames. Keep other lanes,
# credits and closes unchanged. Segmentation into P9MX payloads may differ.
# RU: Разобрать исходные кадры только проверяемого канала. Передать другие
# каналы, кредиты и закрытия без изменений. Разбиение на кадры P9MX может меняться.
def relay(source, destination, gate, request):
    pending = bytearray()
    try:
        while header := read_exact(source, 12):
            magic, version, kind, lane, reserved, size = struct.unpack("<4sBBBBI", header)
            if magic != b"P9MX" or version != 1 or reserved or size > PAYLOAD:
                raise RuntimeError("unexpected P9MX header")
            payload = read_exact(source, size)
            if payload is None:
                raise RuntimeError("truncated P9MX payload")
            if lane != LANE or kind != DATA:
                destination.sendall(header + payload)
                continue
            pending.extend(payload)
            while len(pending) >= 4:
                length, = struct.unpack_from("<I", pending)
                if not 7 <= length <= 65536:
                    raise RuntimeError("unexpected 9P frame length")
                if length > len(pending):
                    break
                frame = gate.select(bytes(pending[:length]), request)
                del pending[:length]
                if frame is not None:
                    for at in range(0, len(frame), PAYLOAD):
                        chunk = frame[at : at + PAYLOAD]
                        destination.sendall(header[:8] + struct.pack("<I", len(chunk)) + chunk)
        if pending:
            raise RuntimeError("partial 9P frame at EOF")
    except (BrokenPipeError, ConnectionResetError):
        pass
    except (OSError, RuntimeError) as error:
        gate.errors.append(str(error))
    finally:
        try:
            destination.shutdown(socket.SHUT_WR)
        except OSError:
            pass


# EN: Connect two unmodified VM processes to the deterministic gate and check
# their real application results plus one exact original tag per stage from gate_type.
# RU: Подключить штатные процессы VM к барьеру и проверить результаты приложений
# и ровно один исходный тег на этап, выбранный gate_type.
def gated_exchange(directory, player, desktop, expected, *, tcp=False, timeout=16, gate_type=Gate):
    player_fd, back = stream_pair(tcp)
    desktop_fd, front = stream_pair(tcp)
    gate = gate_type(directory)
    with player_fd, back, desktop_fd, front:
        workers = [threading.Thread(target=relay, args=(back, front, gate, True)),
                   threading.Thread(target=relay, args=(front, back, gate, False))]
        with subprocess.Popen(player(["--fd", str(player_fd.fileno())]), cwd=directory,
                              pass_fds=(player_fd.fileno(),), stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True) as provider:
            player_fd.close()
            for worker in workers:
                worker.start()
            try:
                result = subprocess.run(desktop(["--styx-mount-fd", str(desktop_fd.fileno())]), cwd=directory,
                                        pass_fds=(desktop_fd.fileno(),), stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                        text=True, timeout=timeout)
                desktop_fd.close()
                verify_exchange(provider, result, expected, ())
            finally:
                if provider.poll() is None:
                    provider.kill()
                for endpoint in (front, back):
                    try:
                        endpoint.shutdown(socket.SHUT_RDWR)
                    except OSError:
                        pass
                for worker in workers:
                    worker.join(5)
            if any(worker.is_alive() for worker in workers) or gate.errors or tuple(gate.flushed) != gate.stages or not gate.recovered:
                raise RuntimeError(f"reverse 9P cancellation gate: {gate.errors}, flushed={gate.flushed}")
