# Copyright (c) 2026 bhgv
"""Test the real compiled Dis export catalogue. / Проверить каталог экспортов Dis."""

from pathlib import Path
from selectors import DefaultSelector, EVENT_READ
import subprocess
import sys
from tempfile import TemporaryDirectory


# EN: Compile three independent images, then replace the path while the guest holds its anchor.
# RU: Собрать три образа и заменить путь, пока процесс удерживает исходный экземпляр.
def check(runtime, limbo, mode):
    here = Path(__file__).resolve().parent
    dis = here.parent
    sources = here / "tests/loaded_exports"
    with TemporaryDirectory(prefix="disvm-loaded-exports-") as temp:
        work = Path(temp)
        for source, output in (("original", "target"), ("replacement", "next"), ("client", "client")):
            subprocess.run(
                (limbo, "-D", "RG", "-I", dis / "upstream/module", "-I", dis / "module",
                 "-o", work / f"{output}.dis", sources / f"{source}.b"),
                cwd=work, check=True, timeout=15,
            )
        with subprocess.Popen((runtime, "--native-spawn", mode, "client.dis"), cwd=work,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True) as guest:
            try:
                with DefaultSelector() as ready:
                    ready.register(guest.stdout, EVENT_READ)
                    if not ready.select(5):
                        raise RuntimeError("guest did not retain the original module")
                marker = guest.stdout.readline()
                if marker != "READY original module\n":
                    raise RuntimeError(f"guest stopped before replacement: {marker}")
                (work / "next.dis").replace(work / "target.dis")
                (work / "replacement-ready").touch()
                output, _ = guest.communicate(timeout=7)
                if guest.returncode or "PASS exact compiled export catalogue" not in output:
                    raise RuntimeError(output)
                print(output, end="")
            except (RuntimeError, subprocess.TimeoutExpired) as failure:
                if guest.poll() is None:
                    guest.kill()
                output, _ = guest.communicate()
                raise RuntimeError(f"{mode}: {failure}\n{output}") from failure


if __name__ == "__main__":
    check(*(Path(arg).resolve() for arg in sys.argv[1:3]), sys.argv[3])
