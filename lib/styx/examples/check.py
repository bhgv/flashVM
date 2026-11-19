# Copyright (c) 2026 bhgv
# EN: Wire-level fixed/live/HTTP acceptance using separate processes and standard 9P2000 messages.
# RU: Wire-проверки fixed/live/HTTP отдельными процессами и стандартными 9P2000 сообщениями.
from pathlib import Path  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import json  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import subprocess  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import sys  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import tempfile  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import urllib.request  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
from wire_client import Client, require  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.


# EN/RU: Start one server and read its real bound port; retain process ownership until stop().
# Запустить сервер и прочитать фактический порт; владеть процессом до stop().
def launch(command):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)  # EN: Update process for the current operation. RU: Обновить process для текущей операции.
    line = process.stdout.readline()  # EN: Update line for the current operation. RU: Обновить line для текущей операции.
    if not line.strip().isdigit():  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
        process.kill()  # EN: Invoke process.kill with these explicit arguments. RU: Вызвать process.kill с этими явными аргументами.
        output, error = process.communicate()  # EN: Invoke process.communicate with these explicit arguments. RU: Вызвать process.communicate с этими явными аргументами.
        raise RuntimeError("server startup failed " + line + output + error)  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
    return process, int(line)  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.


# EN/RU: Terminate/join owned server, rejecting sanitizer diagnostics; never leave fixture children behind.
# Завершить свой сервер и выполнить join, отклонить сообщения санитайзеров; не оставлять дочерние процессы.
def stop(process):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    process.terminate()  # EN: Invoke process.terminate with these explicit arguments. RU: Вызвать process.terminate с этими явными аргументами.
    try:  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
        output, error = process.communicate(timeout=8)  # EN: Invoke process.communicate with these explicit arguments. RU: Вызвать process.communicate с этими явными аргументами.
    except subprocess.TimeoutExpired:  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
        process.kill()  # EN: Invoke process.kill with these explicit arguments. RU: Вызвать process.kill с этими явными аргументами.
        process.communicate()  # EN: Invoke process.communicate with these explicit arguments. RU: Вызвать process.communicate с этими явными аргументами.
        raise RuntimeError("server shutdown did not complete")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
    require("ERROR: AddressSanitizer" not in error and "runtime error:" not in error, output + error)  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.


# EN/RU: Test the requested example against real RPCs, dynamic backend updates and held fid identity.
# Проверить выбранный пример настоящими RPC, обновлениями backend и удерживаемой идентичностью fid.
def main():  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    executable, kind, native_client = sys.argv[1:]  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
    helper = None  # EN: Update helper for the current operation. RU: Обновить helper для текущей операции.
    process = None  # EN: Update process for the current operation. RU: Обновить process для текущей операции.
    client = None  # EN: Update client for the current operation. RU: Обновить client для текущей операции.
    with tempfile.TemporaryDirectory(prefix="styx-catalog-") as temporary:  # EN: Invoke tempfile.TemporaryDirectory with these explicit arguments. RU: Вызвать tempfile.TemporaryDirectory с этими явными аргументами.
        database = Path(temporary) / "keys.tsv"  # EN: Update database for the current operation. RU: Обновить database для текущей операции.
        database.write_text("".join(f"key{index}\tvalue{index}\n" for index in range(40)))  # EN: Invoke database.write_text with these explicit arguments. RU: Вызвать database.write_text с этими явными аргументами.
        command = [executable, "0"]  # EN: Update command for the current operation. RU: Обновить command для текущей операции.
        try:  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
            if kind == "database":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                command.append(str(database))  # EN: Invoke command.append with these explicit arguments. RU: Вызвать command.append с этими явными аргументами.
            if kind == "http":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                helper, source_port = launch([sys.executable, str(Path(__file__).with_name("http_source.py"))])  # EN: Invoke launch with these explicit arguments. RU: Вызвать launch с этими явными аргументами.
                command.append(f"http://127.0.0.1:{source_port}/node")  # EN: Invoke command.append with these explicit arguments. RU: Вызвать command.append с этими явными аргументами.
            process, port = launch(command)  # EN: Invoke launch with these explicit arguments. RU: Вызвать launch с этими явными аргументами.
            if kind == "http":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                with urllib.request.urlopen(f"http://127.0.0.1:{source_port}/stats") as response:  # EN: Invoke urllib.request.urlopen with these explicit arguments. RU: Вызвать urllib.request.urlopen с этими явными аргументами.
                    require(json.load(response) == [], "HTTP tree was fetched at startup")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
            client = Client(port)  # EN: Update client for the current operation. RU: Обновить client для текущей операции.
            if kind == "flat":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                require(set(client.directory("/")) == {"status", "temperature"}, "flat root")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                require(client.file("/status") == "ready\n", "flat contents")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                native = subprocess.run([native_client, str(port), "status"], capture_output=True, text=True, timeout=10)  # EN: Update native for the current operation. RU: Обновить native для текущей операции.
                require(native.returncode == 0 and native.stdout == "ready\n", "real native client: " + native.stderr)  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
            elif kind == "tree":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                require(set(client.directory("/")) == {"devices", "system"}, "tree root")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                require(client.directory("/devices") == ["sensor"], "second level")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                require(client.file("/devices/sensor/temperature") == "21.5\n", "third level")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
            elif kind == "database":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                require(len(client.directory("/")) == 40 and client.file("/key39") == "value39", "paged live database")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                held = client.open("/key0")  # EN: Update held for the current operation. RU: Обновить held для текущей операции.
                replacement = database.with_suffix(".next")  # EN: Update replacement for the current operation. RU: Обновить replacement для текущей операции.
                replacement.write_text("key0\tchanged\nlate\tnew\n")  # EN: Invoke replacement.write_text with these explicit arguments. RU: Вызвать replacement.write_text с этими явными аргументами.
                replacement.replace(database)  # EN: Invoke replacement.replace with these explicit arguments. RU: Вызвать replacement.replace с этими явными аргументами.
                require(set(client.directory("/")) == {"key0", "late"}, "fresh DB keys")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                require(client.read(held, 0, 64) == b"changed" and client.file("/late") == "new", "fresh DB values")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                client.clunk(held)  # EN: Invoke client.clunk with these explicit arguments. RU: Вызвать client.clunk с этими явными аргументами.
            elif kind == "live":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                require(set(client.directory("/")) == {"ctl", "window-100"}, "initial GUI registry")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                held = client.open("/window-100/title")  # EN: Update held for the current operation. RU: Обновить held для текущей операции.
                client.write("/ctl", "add")  # EN: Invoke client.write with these explicit arguments. RU: Вызвать client.write с этими явными аргументами.
                require("window-103" in client.directory("/"), "live window addition")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                client.write("/ctl", "remove 100")  # EN: Invoke client.write with these explicit arguments. RU: Вызвать client.write с этими явными аргументами.
                client.write("/ctl", "add")  # EN: Invoke client.write with these explicit arguments. RU: Вызвать client.write с этими явными аргументами.
                require(set(client.directory("/")) == {"ctl", "window-103", "window-106"}, "live window removal and stable IDs")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                require(client.read(held, 0, 64) == b"Live window", "hidden name must not retire the already opened object")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                client.clunk(held)  # EN: Invoke client.clunk with these explicit arguments. RU: Вызвать client.clunk с этими явными аргументами.
            elif kind == "http":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                require(set(client.directory("/")) == {"info", "Exampleland", "DemoRepublic"}, "HTTP countries")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                path = "/Exampleland/city-37/district-42/street-5/house-12/room-9/info"  # EN: Update path for the current operation. RU: Обновить path для текущей операции.
                require(client.file(path) == "Synthetic information for room-9", "deep lazy HTTP walk/read")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                require(len(client.directory("/Exampleland")) == 101, "paged HTTP cities")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                with urllib.request.urlopen(f"http://127.0.0.1:{source_port}/stats") as response:  # EN: Invoke urllib.request.urlopen with these explicit arguments. RU: Вызвать urllib.request.urlopen с этими явными аргументами.
                    reports = json.load(response)  # EN: Update reports for the current operation. RU: Обновить reports для текущей операции.
                require(reports and all(report["limit"] <= 16 for report in reports), "bounded immediate-child HTTP requests")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                require(len(reports) < 1000, "whole huge tree traversal occurred")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
            else:  # EN: Handle the alternative to the preceding condition. RU: Обработать альтернативу предыдущего условия.
                raise ValueError("unknown catalog example")  # EN: Invoke ValueError with these explicit arguments. RU: Вызвать ValueError с этими явными аргументами.
            print("PASS standalone standard 9P2000 " + kind, flush=True)  # EN: Invoke print with these explicit arguments. RU: Вызвать print с этими явными аргументами.
        finally:  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
            if client:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                client.close()  # EN: Release this owned native descriptor. RU: Освободить этот native descriptor.
            if process:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                stop(process)  # EN: Invoke stop with these explicit arguments. RU: Вызвать stop с этими явными аргументами.
            if helper:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                stop(helper)  # EN: Invoke stop with these explicit arguments. RU: Вызвать stop с этими явными аргументами.


if __name__ == "__main__":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
    main()  # EN: Invoke main with these explicit arguments. RU: Вызвать main с этими явными аргументами.
