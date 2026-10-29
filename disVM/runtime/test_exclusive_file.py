"""Build the real POSIX file adapter under sanitizers and race exclusive creators."""

from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile


# ports is an explicit selected FlashVM port SDK/source directory. No repository
# discovery or VM is needed; the host fixture owns all temporary files/processes.
def main():
    ports = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="exclusive-port-") as directory:
        work = Path(directory)
        binary = work / "exclusive"
        compiler = shlex.split(os.environ.get("CC", "cc"))
        flags = shlex.split(
            os.environ.get(
                "CFLAGS", "-O1 -g -DNDEBUG -fsanitize=address,undefined -fno-sanitize-recover=all"
            )
        )
        subprocess.run(
            compiler
            + flags
            + [
                "-std=c99",
                "-D_POSIX_C_SOURCE=200809L",
                "-I" + str(ports),
                "-I" + str(ports.parent / "include"),
                str(Path(__file__).with_suffix(".c")),
                str(ports / "file_port.c"),
                str(ports / "linux/filesystem.c"),
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], cwd=work, check=True, timeout=30)


if __name__ == "__main__":
    main()
