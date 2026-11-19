# Copyright (c) 2026 bhgv
# EN: Wire-level fixed/live/HTTP acceptance using separate processes and standard 9P2000 messages.
# RU: Wire-проверки fixed/live/HTTP отдельными процессами и стандартными 9P2000 сообщениями.
# standalone-reviewed: authored fixture contracts; Python AST unchanged.
from pathlib import Path
import json
import subprocess
import sys
import tempfile
import urllib.request
from wire_client import Client, require


# EN: Start borrowed command with owned stdout/stderr pipes, require its first stdout line to be a
# decimal port and return process/port. Reject startup output/errors explicitly. Caller owns
# stop/reap after success; this local acceptance helper is not a general process supervisor.
# RU: Запустить command с owned stdout/stderr pipes; требовать первой строкой decimal port, вернуть
# process/port. Явно отклонить startup error/output. После успеха caller владеет stop/reap; это
# acceptance helper, не process supervisor.
def launch(command):
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    line = process.stdout.readline()
    if not line.strip().isdigit():
        process.kill()
        output, error = process.communicate()
        raise RuntimeError("server startup failed " + line + output + error)
    return process, int(line)


# EN: Terminate and reap borrowed owned process with an 8s deadline; kill/reap on timeout and raise.
# Reject ASan/UBSan diagnostics from captured output. Return None; normal SIGTERM exit is expected
# for these servers.
# RU: Завершить/reap owned process с deadline8с; при timeout kill/reap и exception. Отвергнуть
# ASan/UBSan diagnostics в captured output. Вернуть None; для серверов ожидается обычный SIGTERM
# exit.
def stop(process):
    process.terminate()
    try:
        output, error = process.communicate(timeout=8)
    except subprocess.TimeoutExpired:
        process.kill()
        process.communicate()
        raise RuntimeError("server shutdown did not complete")
    require("ERROR: AddressSanitizer" not in error and "runtime error:" not in error, output + error)


# EN: argv supplies server executable, one catalog model and native client path. Own temporary TSV
# data, optional HTTP fixture, server and 9P client; exercise the selected model, backend mutations
# and retained opened identity. finally closes client and reaps children. Return normally on PASS,
# otherwise raise the exact acceptance failure.
# RU: argv задаёт server executable, catalog model и native client path. Владеть temporary TSV,
# optional HTTP fixture, сервером и 9P client; проверить выбранную модель, backend mutations и
# opened identity. finally закрывает client и reaps children. Нормальный возврат — PASS, иначе exact
# acceptance failure.
def main():
    executable, kind, native_client = sys.argv[1:]
    helper = None
    process = None
    client = None
    with tempfile.TemporaryDirectory(prefix="styx-catalog-") as temporary:
        database = Path(temporary) / "keys.tsv"
        database.write_text("".join(f"key{index}\tvalue{index}\n" for index in range(40)))
        command = [executable, "0"]
        try:
            if kind == "database":
                command.append(str(database))
            if kind == "http":
                helper, source_port = launch([sys.executable, str(Path(__file__).with_name("http_source.py"))])
                command.append(f"http://127.0.0.1:{source_port}/node")
            process, port = launch(command)
            if kind == "http":
                with urllib.request.urlopen(f"http://127.0.0.1:{source_port}/stats") as response:
                    require(json.load(response) == [], "HTTP tree was fetched at startup")
            client = Client(port)
            if kind == "flat":
                require(set(client.directory("/")) == {"status", "temperature"}, "flat root")
                require(client.file("/status") == "ready\n", "flat contents")
                native = subprocess.run([native_client, str(port), "status"], capture_output=True, text=True, timeout=10)
                require(native.returncode == 0 and native.stdout == "ready\n", "real native client: " + native.stderr)
            elif kind == "tree":
                require(set(client.directory("/")) == {"devices", "system"}, "tree root")
                require(client.directory("/devices") == ["sensor"], "second level")
                require(client.file("/devices/sensor/temperature") == "21.5\n", "third level")
            elif kind == "database":
                require(len(client.directory("/")) == 40 and client.file("/key39") == "value39", "paged live database")
                held = client.open("/key0")
                replacement = database.with_suffix(".next")
                replacement.write_text("key0\tchanged\nlate\tnew\n")
                replacement.replace(database)
                require(set(client.directory("/")) == {"key0", "late"}, "fresh DB keys")
                require(client.read(held, 0, 64) == b"changed" and client.file("/late") == "new", "fresh DB values")
                client.clunk(held)
            elif kind == "live":
                require(set(client.directory("/")) == {"ctl", "window-100"}, "initial GUI registry")
                held = client.open("/window-100/title")
                client.write("/ctl", "add")
                require("window-103" in client.directory("/"), "live window addition")
                client.write("/ctl", "remove 100")
                client.write("/ctl", "add")
                require(set(client.directory("/")) == {"ctl", "window-103", "window-106"}, "live window removal and stable IDs")
                require(client.read(held, 0, 64) == b"Live window", "hidden name must not retire the already opened object")
                client.clunk(held)
            elif kind == "http":
                require(set(client.directory("/")) == {"info", "Exampleland", "DemoRepublic"}, "HTTP countries")
                path = "/Exampleland/city-37/district-42/street-5/house-12/room-9/info"
                require(client.file(path) == "Synthetic information for room-9", "deep lazy HTTP walk/read")
                require(len(client.directory("/Exampleland")) == 101, "paged HTTP cities")
                with urllib.request.urlopen(f"http://127.0.0.1:{source_port}/stats") as response:
                    reports = json.load(response)
                require(reports and all(report["limit"] <= 16 for report in reports), "bounded immediate-child HTTP requests")
                require(len(reports) < 1000, "whole huge tree traversal occurred")
            else:
                raise ValueError("unknown catalog example")
            print("PASS standalone standard 9P2000 " + kind, flush=True)
        finally:
            if client:
                client.close()
            if process:
                stop(process)
            if helper:
                stop(helper)


if __name__ == "__main__":
    main()
