# Copyright (c) 2026 bhgv
# EN: Run stock DisVM and a separate provider over a delegated carrier.
# RU: Запустить штатный DisVM и отдельного провайдера по выданному транспорту.
from pathlib import Path
import socket
import subprocess
import sys
import tempfile


# EN: Verify the command in both VM modes using a fresh carrier for each.
# RU: Проверить команду в обоих режимах VM, каждый раз с новым транспортом.
def main():
    runtime, compiler, provider = map(lambda name: Path(name).resolve(), sys.argv[1:])
    disvm = Path(__file__).resolve().parents[4]
    example = disvm / "examples/remote-scalar"
    with tempfile.TemporaryDirectory(prefix="styx-cli-") as directory:
        work = Path(directory)
        image = work / "caller.dis"
        subprocess.run([compiler, "-I", disvm / "upstream/module", "-I", disvm / "module",
                        "-I", example, "-o", image, example / "styx-cli.b"], cwd=work, check=True)
        for mode in ("--no-jit", "--jit"):
            caller, owner = socket.socketpair()
            with caller, owner, subprocess.Popen(
                [provider, str(owner.fileno())], pass_fds=(owner.fileno(),), cwd=work
            ) as peer:
                owner.close()
                try:
                    result = subprocess.run(
                        [runtime, "--styx-mount-fd", str(caller.fileno()), "--", mode, image],
                        pass_fds=(caller.fileno(),), cwd=work, capture_output=True, text=True, timeout=20)
                    caller.close()
                    marker = "PASS bidirectional Styx CLI mount"
                    if result.returncode or marker not in result.stdout or peer.wait(timeout=10):
                        raise RuntimeError(result.stdout + result.stderr)
                    print(marker, mode, flush=True)
                finally:
                    if peer.poll() is None:
                        peer.kill()


if __name__ == "__main__":
    main()
