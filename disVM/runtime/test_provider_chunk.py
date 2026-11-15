# Copyright (c) 2026 bhgv
# EN: Exercise precise chunk callbacks through production Sys admission/publication.
# RU: Проверять precise chunk callbacks через production Sys admission/publication.
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink
from test_owner_location import compile_image


# EN/RU: Own private relinked runtime and real guest until all mode runs exit.
# Require actual bytes/counts plus host callback/probe/lifetime evidence.
# Владеть private runtime и guest до конца modes; проверить bytes/counts и host evidence.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="provider chunks ") as temporary:
        work = Path(temporary)
        (work / "view").mkdir()
        image = work / "main.dis"
        compile_image(compiler, include, root, root / "fixtures/provider-chunk/main.b", image)
        binary = relink(runtime, work, root / "test_provider_chunk.c", 0,
                        ["dvm_namespace_boot_initialize", "dvm_close_worker_stop"])
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                result = subprocess.run([str(binary), mode, "--stats",
                                         *(["--gc-locked-poll"] if locked else []), str(image)],
                                        cwd=work, text=True, capture_output=True, timeout=45)
                text = result.stdout + result.stderr
                if result.returncode or "PASS precise chunk guest" not in text or \
                        "PASS precise provider effects once" not in text or "ThreadSanitizer:" in text:
                    raise RuntimeError(text[-18000:])
                print(f"PASS precise provider chunk {mode} locked={locked}")


if __name__ == "__main__":
    main()
