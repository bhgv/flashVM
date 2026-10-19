#!/usr/bin/env python3
"""Export only the Android/Linux library sources from the explicitly selected Git repository."""
import argparse
import hashlib
import json
import pathlib
import subprocess
import tarfile
import tempfile

REVISION = "2a616b264f0045a3ba00ce77aa1930ca905956ea"
REPOSITORY = "https://github.com/bhgv/Inferno-OS_Android-Clutter3d.git"
PATHS = ("COPYING", "LICENSE", "sources/clutter-android", "sources/cogl-android",
         "sources/glib-android", "fake-android-application")


def prepare(repository, output):
    """Refuse to overwrite user files; preserve upstream paths and licenses, excluding the client."""
    repository = pathlib.Path(repository).resolve()
    output = pathlib.Path(output).resolve()
    subprocess.check_call(["git", "-C", str(repository), "cat-file", "-e", REVISION + "^{commit}"])
    if output.exists() and any(output.iterdir()):
        raise RuntimeError("output must be absent or empty: " + str(output))
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryFile() as archive:
        subprocess.check_call(["git", "-C", str(repository), "archive", REVISION] + list(PATHS),
                              stdout=archive)
        archive.seek(0)
        with tarfile.open(fileobj=archive) as tree:
            # Git-controlled paths still receive tarfile's traversal/symlink protections.
            tree.extractall(output, filter="data")
    patch = pathlib.Path(__file__).resolve().parent / "patches" / "linux-android-config.patch"
    subprocess.check_call(["patch", "--batch", "-p1", "-i", str(patch)], cwd=output)
    manifest = {"repository": REPOSITORY, "revision": REVISION, "included": list(PATHS),
                "excluded": ["inferno-os"],
                "patch_sha256": hashlib.sha256(patch.read_bytes()).hexdigest()}
    (output / "clutter3d-source.json").write_text(json.dumps(manifest, indent=2) + "\n")
    if (output / "inferno-os").exists():
        raise RuntimeError("unexpected excluded client in export")
    return output


def main():
    """The checkout is explicit; source preparation never guesses sibling projects or pushes Git."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", required=True, help="local Git repository containing pinned revision")
    parser.add_argument("--output", required=True, help="new or empty SDK source directory")
    args = parser.parse_args()
    print(prepare(args.repository, args.output))


if __name__ == "__main__":
    main()
