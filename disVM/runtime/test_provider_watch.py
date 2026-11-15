# Copyright (c) 2026 bhgv
# EN: Real native/event files, pure Limbo watch library and single-record reads.
# RU: Реальные native/event files, Limbo watch library и single-record reads.
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink
from test_owner_location import compile_image


# EN/RU: Own host trees and private runtime; verify bytes independently from guest.
# Владеть host trees и private runtime; проверить bytes независимо от guest.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="provider watch ") as temporary:
        work = Path(temporary)
        for name in ("view", "watch", "jail"):
            (work / name).mkdir()
        (work / "alias").write_text("HOST")
        image = work / "main.dis"
        compile_image(compiler, include, root, root / "fixtures/provider-watch/main.b", image)
        compile_image(compiler, include, root, root.parent / "limbo/providerwatch.b", work / "providerwatch.dis")
        binary = relink(runtime, work, root / "test_provider_watch.c", 0,
                        ["dvm_namespace_boot_initialize", "dvm_close_worker_stop"])
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                (work / "native").write_text("ABCDE")
                (work / "ordinary").write_text("ordinary bytes\n")
                result = subprocess.run([str(binary), mode, "--stats",
                                         *(["--gc-locked-poll"] if locked else []), str(image)],
                                        cwd=work, text=True, capture_output=True, timeout=60)
                text = result.stdout + result.stderr
                if result.returncode or "PASS provider watch guest" not in text or \
                        "PASS provider watch host" not in text or "ThreadSanitizer:" in text:
                    raise RuntimeError(text[-18000:])
                if (work / "native").read_text() != "ABXDE" or (work / "ordinary").read_text() != "ordinary bytes\n" or \
                        (work / "missing").exists() or (work / "alias").read_text() != "HOST":
                    raise RuntimeError("ORDWR/handshake/native target bytes violated")
                print(f"PASS provider watch {mode} locked={locked}")


if __name__ == "__main__":
    main()
