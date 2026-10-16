#!/usr/bin/env python3
"""Execute an unregistered C callback on its original heap in a different process."""
import pathlib
import queue
import subprocess
import sys
import threading


def read_uri(stream, lines):
    """Bound startup externally even if the child never emits its first line."""
    lines.put(stream.readline().strip())


def main():
    """Start an authority, invoke four results in a client, then reclaim source state."""
    program = str(pathlib.Path(sys.argv[1]).resolve())
    owner = subprocess.Popen([program, "serve"], stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    lines = queue.Queue()
    reader = threading.Thread(target=read_uri, args=(owner.stdout, lines), daemon=True)
    reader.start()
    try:
        uri = lines.get(timeout=10)
        reader.join(timeout=1)
        if not uri.startswith("fvm://"):
            raise RuntimeError("Authority did not publish a function capability")
        client = subprocess.run([program, "call", uri], timeout=15,
                                capture_output=True, text=True)
        if client.returncode:
            raise RuntimeError("Remote client failed: " + client.stderr)
        if "remote result=42; counted exception; unsupported; error" not in client.stdout:
            raise RuntimeError("Remote client did not finish all four calls")
        stdout, stderr = owner.communicate(input="done\n", timeout=10)
        if owner.returncode or "original calls=4" not in stdout:
            raise RuntimeError("Original authority did not reclaim four calls: " + stderr)
    finally:
        if owner.poll() is None:
            owner.kill()
            owner.communicate()
        reader.join(timeout=1)
    print("PASS codec-free original C callback across separate authority/client processes")


if __name__ == "__main__":
    main()
