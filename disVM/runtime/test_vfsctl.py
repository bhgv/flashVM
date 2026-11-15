# Copyright (c) 2026 bhgv
# EN: Execute real Vfsctl with three native directory providers and independent byte oracles.
# RU: Выполнить real Vfsctl с тремя native directories и независимой проверкой bytes.
from pathlib import Path
import subprocess
import sys
import tempfile
from test_owner_location import compile_image
from relink_private import relink


# EN/RU: Build matched guest/library and own filesystem trees until C/JIT checks complete.
# Собрать matched guest/library; владеть filesystem trees до завершения проверок C/JIT.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="layer control ") as temporary:
        work = Path(temporary)
        for name in ("a", "b", "c", "view"):
            (work / name).mkdir()
        for name in ("a", "b", "c"):
            (work / name / "f").write_text(name.upper())
            (work / name / "g").write_text(name)
        image = work / "main.dis"
        compile_image(compiler, include, root, root / "fixtures/vfsctl/main.b", image)
        compile_image(compiler, include, root, root.parent / "limbo/vfsctl.b", work / "vfsctl.dis")
        binary = relink(runtime, work, root / "test_vfsctl_provider.c", 0,
                        ["dvm_namespace_boot_initialize", "dvm_close_worker_stop"])
        for mode in ("--no-jit", "--jit"):
            for locked in (False, True):
                result = subprocess.run([str(binary), mode, "--stats", *(["--gc-locked-poll"] if locked else []), str(image)],
                                        cwd=work, text=True, capture_output=True, timeout=60)
                text = result.stdout + result.stderr
                if result.returncode or "PASS Vfsctl all layers" not in text or "PASS Vfsctl provider lifetime" not in text or "ThreadSanitizer:" in text:
                    raise RuntimeError(text[-16000:])
                for name in ("a", "b", "c"):
                    if (work / name / "f").read_text() != name.upper() or (work / name / "g").read_text() != name:
                        raise RuntimeError("layer edits changed source file data")
                print(f"PASS Vfsctl {mode} locked={locked}")


if __name__ == "__main__":
    main()
