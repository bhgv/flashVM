# Copyright (c) 2026 bhgv
# EN: Measure real editor idle CPU, then verify input wakes it and saves a file.
# RU: Измерить CPU редактора в простое, затем проверить пробуждение и сохранение.

from pathlib import Path
import os
import struct
import subprocess
import sys
import tempfile
import time


# EN: Encode a borrowed numeric event and UTF-8 text for the counted replay FIFO.
# Return owned bytes; this test input is not an alternate live Unibus protocol.
# RU: Кодировать числовое событие и UTF-8 text для replay FIFO с длиной записи.
# Вернуть отдельные байты; это ввод теста, а не другой живой протокол Unibus.
def event(kind, code=0, flags=0, text=""):
    payload = b"fvd1:" + "".join(
        f"{value:08x}" for value in (kind, int(kind == 1), code, flags, 0, 0)
    ).encode("ascii") + text.encode("utf-8")
    return struct.pack("<I", len(payload)) + payload


# EN: Read process-wide user+kernel ticks for borrowed pid, including all threads.
# Return an integer; absent/malformed proc data raises instead of hiding failure.
# RU: Прочитать суммарные user+kernel ticks всех потоков процесса pid.
# Вернуть число; отсутствие или повреждение proc-данных считается ошибкой.
def ticks(pid):
    fields = Path(f"/proc/{pid}/stat").read_text().rsplit(")", 1)[1].split()
    return int(fields[11]) + int(fields[12])


# EN: Wait up to 30 seconds for UI main to block on the replay pipe after drawing.
# process/log are borrowed; failure/timeout raises with the captured diagnostics.
# RU: До 30 секунд ждать, пока UI main после отрисовки заблокируется на replay FIFO.
# process/log заимствованы; завершение/таймаут дают ошибку с захваченным выводом.
def wait_idle(process, log):
    deadline = time.monotonic() + 30
    while process.poll() is None and time.monotonic() < deadline:
        wait = Path(f"/proc/{process.pid}/wchan").read_text().strip()
        if "pipe" in wait and "FRAMECOPY " in log.read_text(errors="replace"):
            return
        time.sleep(0.05)
    raise RuntimeError("editor did not reach input wait: " + log.read_text(errors="replace")[-2000:])


# EN: argv supplies editor, desktop bytecode and font. Own a temporary FIFO,
# source and logs; join or kill the child on every exit. Measure two quiet seconds
# before injecting edit/save/quit. Linux proc is a development-only dependency.
# RU: argv задаёт editor, desktop-байткод и шрифт. Тест владеет FIFO, исходником
# и логами; при любом выходе ждёт или завершает дочерний процесс. Измерить две
# тихие секунды до ввода правки/save/quit. Linux proc нужен лишь для проверки.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix="editor idle ") as directory:
        work = Path(directory)
        source = work / "document.txt"
        source.write_text("original", encoding="utf-8")
        fifo = work / "events.fifo"
        os.mkfifo(fifo)
        descriptor = os.open(fifo, os.O_RDWR)
        log = work / "stdout.log"
        error = work / "stderr.log"
        process = None
        try:
            with log.open("w") as output, error.open("w") as diagnostic:
                process = subprocess.Popen(
                    [str(runtime), "--jit", "--stats", "--font", str(font),
                     "--events", str(fifo), "--document", str(source), str(module)],
                    cwd=work, stdout=output, stderr=diagnostic,
                    env=dict(os.environ, TMPDIR=str(work)),
                )
                wait_idle(process, log)
                # EN/RU: Exclude startup and let outstanding render/GC work drain.
                # Исключить старт и дать завершиться отложенным render/GC задачам.
                time.sleep(0.5)
                before = ticks(process.pid)
                started = time.monotonic()
                time.sleep(2)
                cpu = (ticks(process.pid) - before) / os.sysconf("SC_CLK_TCK")
                elapsed = time.monotonic() - started
                percent = 100 * cpu / elapsed
                if percent > 15:
                    raise RuntimeError(f"editor idle busy loop: {percent:.1f}% of one CPU")
                payload = event(5, text="edited-") + event(4, 17, 1)
                payload += event(4, ord("S"), 1) + event(4, ord("S"), 0)
                payload += event(4, 17, 0) + event(1, 65539, 1)
                if os.write(descriptor, payload) != len(payload):
                    raise RuntimeError("short replay FIFO write")
                process.wait(timeout=30)
            if process.returncode or "PASS MovieClip desktop:" not in log.read_text():
                raise RuntimeError(log.read_text()[-2000:] + error.read_text())
            if source.read_text(encoding="utf-8") != "edited-original":
                raise RuntimeError("editor did not wake and save input")
            print(f"PASS editor idle: CPU={percent:.1f}% input/save/quit=ok")
        finally:
            if process is not None and process.poll() is None:
                process.kill()
                process.wait()
            os.close(descriptor)


if __name__ == "__main__":
    main()
