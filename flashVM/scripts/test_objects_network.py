#!/usr/bin/env python3
"""Transfer all supported standard graph kinds over TCP between fresh processes."""
import pathlib
import subprocess
import sys
import tempfile

program = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix="flashvm-objects-network-") as directory:
    receiver = subprocess.Popen(
        [program, "network-receiver"],
        cwd=directory,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    try:
        # The native receiver has a bounded channel wait; CTest bounds this driver too.
        uri = receiver.stdout.readline().strip()
        if not uri.startswith("fvm://127.0.0.1:"):
            raise RuntimeError("receiver did not publish a loopback capability")
        subprocess.run([program, "write", uri], cwd=directory, check=True, timeout=20)
        stdout, stderr = receiver.communicate(timeout=20)
        if receiver.returncode:
            raise RuntimeError("receiver failed: " + stderr)
    finally:
        if receiver.poll() is None:
            receiver.kill()
            receiver.communicate()
print("PASS independent TCP standard object writer/reader processes")
