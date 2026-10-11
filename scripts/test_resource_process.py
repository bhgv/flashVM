#!/usr/bin/env python3
"""Run TCP resource/file restoration tests across two independent processes."""
import pathlib
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix="flashvm-network-") as directory:
    state = str(pathlib.Path(directory) / "capabilities.bin")
    owner = subprocess.Popen([sys.argv[1], "owner", state], stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, text=True)
    try:
        uri = owner.stdout.readline().strip()
        if not uri.startswith("fvm://"):
            raise RuntimeError("Owner did not publish a resource URI: " + owner.stderr.read())
        client = subprocess.run([sys.argv[1], "client", uri, state], timeout=20,
                                capture_output=True, text=True)
        if client.returncode:
            raise RuntimeError("Client failed: " + client.stderr)
        _, stderr = owner.communicate(timeout=10)
        if owner.returncode:
            raise RuntimeError("Owner failed: " + stderr)
    finally:
        if owner.poll() is None:
            owner.kill()
            owner.wait()
print("Independent owner/client processes passed")
