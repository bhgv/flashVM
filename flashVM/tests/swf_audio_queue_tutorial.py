"""Run the real C89 queue tutorial: rejected-write retry, copied clock and CLI errors."""

import subprocess
import sys


def run(executable, *arguments):
    return subprocess.run(
        [executable] + list(arguments), stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10
    )


def main():
    executable = sys.argv[1]
    result = run(executable, "--demo")
    if result.returncode or result.stdout != b"accepted=128 queued=64 played=64\n":
        raise RuntimeError((result.returncode, result.stdout, result.stderr))
    result = run(executable, "--finite")
    if result.returncode or result.stdout != b"finite pending=0 submitted=128 played=128\n":
        raise RuntimeError((result.returncode, result.stdout, result.stderr))
    if run(executable).returncode != 0:
        raise RuntimeError("usage must succeed without acquiring resources")
    for arguments in [("--bad",), ("--demo", "extra"), ("--sdl", "extra"), ("--finite", "extra")]:
        if run(executable, *arguments).returncode != 2:
            raise RuntimeError("malformed arguments must fail before opening devices")
    print(
        "queue tutorial finite drain, legacy retry, half-block clock and argument handling passed"
    )


if __name__ == "__main__":
    main()
