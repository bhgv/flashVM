#!/usr/bin/env python3
"""Exercise local calendar construction/setters under independent libc TZ rules."""
import os
import pathlib
import subprocess
import sys

program = str(pathlib.Path(sys.argv[1]).resolve())
profiles = [("UTC0", 0, 0), ("JST-9", -540, -540), ("EST5EDT,M3.2.0/2,M11.1.0/2", 300, 240)]
for zone, winter, summer in profiles:
    environment = dict(os.environ, TZ=zone)
    subprocess.run([program, str(winter), str(summer)], env=environment, check=True, timeout=15)
print("PASS UTC, fixed east offset, and winter/summer DST calendar profiles")
