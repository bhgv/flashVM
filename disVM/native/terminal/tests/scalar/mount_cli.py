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
        (work / "alias.dis").touch()
        for source, marker, options, peer_options in (
            ("styx-cli.b", "PASS bidirectional Styx CLI mount", [], []),
            ("styx-run.b", "PASS mounted Styx launch submission",
             ["--launch", "implementation.dis", ".unibus", "17"], ["launch"]),
        ):
            image = work / "caller.dis"
            subprocess.run([compiler, "-I", disvm / "upstream/module", "-I", disvm / "module",
                            "-I", example, "-o", image, example / source], cwd=work, check=True)
            for mode in ("--no-jit", "--jit"):
                caller, owner = socket.socketpair()
                with caller, owner, subprocess.Popen(
                    [provider, str(owner.fileno()), *peer_options], pass_fds=(owner.fileno(),), cwd=work
                ) as peer:
                    owner.close()
                    try:
                        result = subprocess.run(
                            [runtime, "--styx-mount-fd", str(caller.fileno()), *options, "--", mode, image],
                            pass_fds=(caller.fileno(),), cwd=work, capture_output=True, text=True, timeout=20)
                        caller.close()
                        if result.returncode or marker not in result.stdout or peer.wait(timeout=10):
                            raise RuntimeError(result.stdout + result.stderr)
                        print(marker, mode, flush=True)
                    finally:
                        if peer.poll() is None:
                            peer.kill()

        owner_image = work / "owner.dis"
        subprocess.run([compiler, "-I", disvm / "upstream/module", "-I", disvm / "module",
                        "-o", owner_image, example / "styx-owner.b"], cwd=work, check=True)
        for mode in ("--no-jit", "--jit"):
            caller, owner = socket.socketpair()
            with caller, owner, subprocess.Popen(
                [runtime, "--styx-serve-fd", str(owner.fileno()), "--", mode, owner_image],
                pass_fds=(owner.fileno(),), cwd=work, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
            ) as peer:
                owner.close()
                try:
                    result = subprocess.run(
                        [runtime, "--styx-mount-fd", str(caller.fileno()),
                         "--launch", "implementation.dis", ".unibus", "17", "--", mode, image],
                        pass_fds=(caller.fileno(),), cwd=work, capture_output=True, text=True, timeout=20)
                    caller.close()
                    out, err = peer.communicate(timeout=10)
                    marker = "PASS mounted Styx owner Dis init"
                    if result.returncode or peer.returncode or marker not in out or                             "PASS mounted Styx launch submission" not in result.stdout:
                        raise RuntimeError(result.stdout + result.stderr + out + err)
                    print(marker, mode, flush=True)
                finally:
                    if peer.poll() is None:
                        peer.kill()


if __name__ == "__main__":
    main()
