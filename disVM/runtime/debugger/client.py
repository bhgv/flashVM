# Copyright (c) 2026 bhgv
# EN: Private host debugger pipes; guest stdin/stdout and native controls are separate.
# RU: Private host debugger pipes; guest stdin/stdout и native control разделены.
import json
import os
from pathlib import Path
import select
import subprocess
import time


# EN: Own parent pipe endpoints; child owns explicit copies. Closing a client does
# not resume/terminate the VM; callers explicitly finish or kill their child.
# RU: Владеть parent pipe endpoints; child владеет explicit copies. Close client
# не делает resume/terminate VM; caller явно завершает свой child.
class PipeDebugger:
    # EN/RU: Launch runtime with private descriptors; preserve application I/O policy.
    # Запустить runtime с private descriptors; сохранить caller application I/O policy.
    def __init__(self, runtime, arguments, **process_options):
        child_input, self.input = os.pipe()
        self.output, child_output = os.pipe()
        self.buffer = b""
        self.session = ""
        try:
            self.process = subprocess.Popen(
                [str(runtime), "--debug-fds", str(child_input), str(child_output), *arguments],
                pass_fds=(child_input, child_output),
                **process_options,
            )
        except BaseException:
            self.close()
            raise
        finally:
            os.close(child_input)
            os.close(child_output)
        try:
            self.session = self.command("hello 0")["session"]
        except BaseException:
            self.close()
            self.process.kill()
            self.process.wait()
            raise

    # EN: Send one bounded command and decode one bounded response. Timeout means
    # unknown command outcome; query status before changing control again.
    # RU: Отправить bounded command и прочитать bounded response. Timeout означает
    # неизвестный outcome; запросить status перед следующим изменением control.
    def command(self, text, timeout=5):
        encoded = text.encode("utf-8")
        if not encoded or len(encoded) > 218 or b"\n" in encoded or b"\x00" in encoded:
            raise ValueError("one debug command must be 1..218 bytes without newline/NUL")
        if self.session:
            encoded = b"@" + self.session.encode("ascii") + b" " + encoded
        os.write(self.input, encoded + b"\n")
        deadline = time.monotonic() + timeout
        while b"\n" not in self.buffer:
            remaining = deadline - time.monotonic()
            if remaining <= 0 or not select.select([self.output], [], [], remaining)[0]:
                raise TimeoutError("debug command outcome unknown: " + text)
            block = os.read(self.output, 4096)
            if not block:
                raise RuntimeError("debug supervisor closed the response stream")
            self.buffer += block
            if len(self.buffer) > 4096:
                raise RuntimeError("debug supervisor exceeded response bound")
        response, self.buffer = self.buffer.split(b"\n", 1)
        return json.loads(response)

    # EN/RU: Close only owned pipe endpoints, retaining the VM's debug state.
    # Закрыть только свои pipe endpoints, сохраняя debug state VM.
    def close(self):
        for name in ("input", "output"):
            descriptor = getattr(self, name, -1)
            if descriptor >= 0:
                os.close(descriptor)
                setattr(self, name, -1)


# EN: Attach to the private Unix endpoint. Every command checks the original random
# session identity, so pathname reuse cannot route old PID controls to another VM.
# RU: Подключиться к private Unix endpoint. Каждая command проверяет original random
# session identity: повторное использование пути не направляет old PID control в другую VM.
class SocketDebugger:
    # EN/RU: Pin native session identity; endpoint permissions supply local authority.
    # Зафиксировать native session identity; endpoint permissions дают local authority.
    def __init__(self, path, process=None):
        self.path = str(path)
        self.process = process
        self.session = ""
        deadline = time.monotonic() + 5
        while True:
            try:
                self.session = self.command("hello 0")["session"]
                break
            except (FileNotFoundError, ConnectionRefusedError):
                if time.monotonic() >= deadline:
                    raise
                time.sleep(0.01)

    # EN/RU: One bounded request/connection; closing it never releases a VM continuation.
    # Один bounded request/connection; закрытие не освобождает VM continuation.
    def command(self, text, timeout=5):
        import socket

        encoded = text.encode("utf-8")
        if not encoded or len(encoded) > 218 or b"\n" in encoded or b"\x00" in encoded:
            raise ValueError("one debug command must be 1..218 bytes without newline/NUL")
        if self.session:
            encoded = b"@" + self.session.encode("ascii") + b" " + encoded
        data = b""
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
            connection.settimeout(timeout)
            connection.connect(self.path)
            connection.sendall(encoded + b"\n")
            while not data.endswith(b"\n"):
                block = connection.recv(4096)
                if not block:
                    raise RuntimeError("debugger connection ended before a complete response")
                data += block
                if len(data) > 4096:
                    raise RuntimeError("debugger response exceeded native bound")
        return json.loads(data)

    # EN/RU: Connections are scoped to command; detached VM state remains retained.
    # Connections ограничены command; detached VM state остаётся retained.
    def close(self):
        pass
