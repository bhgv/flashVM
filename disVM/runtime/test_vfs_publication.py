# Copyright (c) 2026 bhgv
# EN: Real C/JIT publication guests with independent native byte and close-drain oracles.
# RU: C/JIT publication guests с независимыми native byte и close-drain проверками.
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from test_owner_location import compile_image
from relink_private import relink


# EN/RU: Own source/private/mount fixtures through every guest run and refusal variant.
# Владеть source/private/mount fixtures на всех guest runs и refusal variants.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="guest publication ") as temporary:
        work = Path(temporary)
        (work / "data").mkdir()
        (work / "out").mkdir()
        for name in ("notice", "file"):
            (work / name).write_text("")
        original = {"data/a":"A", "data/b":"B", "data/secret":"S", "private":"PRIVATE"}
        for name, value in original.items():
            (work / name).write_text(value)
        (work / "data/link").symlink_to("../private")
        compile_image(compiler, include, root, root / "fixtures/vfs-publication/main.b", work / "main.dis")
        compile_image(compiler, include, root, root.parent / "limbo/vfspublish.b", work / "vfspublish.dis")
        binaries = [runtime] if "--base-only" in sys.argv[4:] else []
        for control in (() if "--base-only" in sys.argv[4:] else (0, 1, 2)):
            directory = work / str(control)
            directory.mkdir()
            symbols = ["dvm_close_worker_stop"]
            if control:
                symbols.append("dvm_runtime_movp_copy" if control == 1 else "ptradd")
            binaries.append(relink(runtime, directory, root / "test_vfs_publication_refusal.c", control, symbols))
        for index, binary in enumerate(binaries):
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
                    result = subprocess.run([str(binary), mode, "--stats", *(["--gc-locked-poll"] if locked else []),
                                             str(work / "main.dis")], cwd=work, text=True, capture_output=True, timeout=70)
                    output = result.stdout + result.stderr
                    close = re.search(r"NATIVECLOSE submitted=(\d+) completed=(\d+) pending=(\d+).*joined=1", output)
                    if result.returncode or "PASS guest publication capabilities" not in output or not close or \
                       close[1] != close[2] or close[3] != "0" or "runtime error:" in output or "ThreadSanitizer:" in output:
                        raise RuntimeError(output[-16000:])
                    if "--base-only" not in sys.argv[4:] and "PASS VFS capability native lifetimes drained" not in output:
                        raise RuntimeError("native capabilities did not retire\n" + output[-16000:])
                    if index and "PASS VFS capability constructor refusal" not in output:
                        raise RuntimeError("private refusal did not execute\n" + output[-16000:])
                    for name, value in original.items():
                        if (work / name).read_text() != value:
                            raise RuntimeError("publication changed private/source bytes: " + name)
                    print(f"PASS guest publication {mode} locked={locked} refusal={index}", flush=True)


if __name__ == "__main__":
    main()
