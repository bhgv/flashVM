#!/usr/bin/env python3
"""Extend the pristine upstream channel fixture with the actual UI adapter and FlashVM library."""
import argparse
from pathlib import Path
import os
import subprocess
import tempfile


# Reuse upstream extraction verbatim, replacing only its test entry point and linker inputs.
# Arguments name explicit FlashVM headers/libraries; returns by subprocess exit or diagnostic.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--flashvm-root", required=True, type=Path)
    parser.add_argument("--adapter", required=True)
    parser.add_argument("--host", required=True)
    parser.add_argument("--libraries", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    upstream = root / "upstream/libinterp/tests/channel-transfers.py"
    driver = upstream.read_text()
    anchor = "unit = (root/'libinterp/tests/channel-transfers.c').read_text()"
    if driver.count(anchor) != 1:
        raise RuntimeError("upstream fixture changed; review UI adapter extraction")
    fixture = (upstream.parent / "channel-transfers.c").read_text()
    signature = "int\nmain(void)"
    if fixture.count(signature) != 1:
        raise RuntimeError("upstream fixture entry changed; review before adapting")
    fixture = fixture.replace(signature, "int\nupstream_fixture_main(void)")
    fixture += "\n" + (root / "tests/dis_ui_channel_tail.c").read_text()
    driver = driver.replace(anchor, "unit = " + repr(fixture))
    # WORD32 remains an upstream fixture; the bridge's linked archive uses the native WORD ABI.
    driver = driver.replace("for bits in (64,32):", "for bits in (64,):")
    link_anchor = "subprocess.run(cmd+includes+[str(source),'-o',str(binary)],check=True)"
    if driver.count(link_anchor) != 1:
        raise RuntimeError("upstream link command changed; review adapter fixture")
    extra = ["-I" + str(root / "include"), "-I" + str(args.flashvm_root / "include")]
    libraries = [args.adapter, args.host] + args.libraries.split(";")
    libraries = [value if value.startswith(("/", "-")) else "-l" + value for value in libraries]
    libraries += ["-lstdc++"]
    driver = driver.replace(link_anchor,
        "subprocess.run(cmd+includes+" + repr(extra) + "+[str(source)]+" + repr(libraries)
        + "+['-o',str(binary)],check=True)")
    # Preserve the upstream __file__ anchor; execute only locally assembled committed fixture text.
    with tempfile.TemporaryDirectory(prefix="dis-ui-", dir=os.environ.get("TMPDIR")) as directory:
        environment = dict(os.environ, TMPDIR=directory)
        subprocess.run([os.environ.get("PYTHON", "python3"), "-c",
                        "__file__=" + repr(str(upstream)) + "\n" + driver],
                       env=environment, check=True)


if __name__ == "__main__":
    main()
