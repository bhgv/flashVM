#!/usr/bin/env python3
"""Verify or refresh a selected upstream snapshot from an explicit local Git checkout.

No network, checkout modification, commit or push occurs here. Upstream changes
are read with git show at one resolved commit, never from dirty working files.
"""
import argparse
import fnmatch
import hashlib
import json
from pathlib import Path, PurePosixPath
import subprocess
import tempfile


# Never interpret an upstream filename as a local absolute/traversing path.
def safe_path(name):
    """Return a relative POSIX path with no traversal or ambiguous components."""
    path = PurePosixPath(name)
    if path.is_absolute() or ".." in path.parts or str(path) != name or not name:
        raise ValueError("unsafe snapshot path: " + name)
    return path


# Parent symlinks must not redirect a trusted snapshot path outside its root.
def target_path(root, relative):
    """Reject symlinks at every existing component before reading or writing."""
    path = root
    for component in safe_path(relative).parts:
        path = path / component
        if path.is_symlink():
            raise ValueError("snapshot path contains a symlink: " + relative)
    return path


# Preserve author-modified sources rather than overwriting an adaptation silently.
def verify(root, manifest):
    """Return integrity errors for every recorded source, including missing files."""
    errors = []
    for name, expected in manifest.get("files", {}).items():
        path = target_path(root, name)
        if path.is_symlink() or not path.is_file():
            errors.append("missing/non-regular: " + name)
        elif hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            errors.append("locally changed: " + name)
    destination = target_path(root, manifest["destination"])
    if destination.is_dir():
        for path in destination.rglob("*"):
            relative = str(path.relative_to(root))
            if path.is_symlink() or (path.is_file() and relative not in manifest.get("files", {})):
                errors.append("unrecorded snapshot file: " + relative)
    return errors


# Read exact committed blobs; executable paths and Git refs are argv data.
def git(source, *args):
    """Run a read-only Git operation in the explicitly selected checkout."""
    return subprocess.check_output(["git", "-C", str(source), *args])


# Stage every new blob before replacing files; interrupted multi-file publication
# is detected by --check, but is not claimed to be an atomic directory transaction.
def refresh(root, manifest, source, ref):
    """Update the whitelist from a pinned commit after checking local integrity."""
    errors = verify(root, manifest)
    if errors:
        raise ValueError("\n".join(errors) + "\nExport local changes as an upstream patch first.")
    commit = (
        git(source, "rev-parse", "--verify", "--end-of-options", ref + "^{commit}").decode().strip()
    )
    listing = git(source, "ls-tree", "-rz", "--full-tree", commit)
    selected = []
    for entry in listing.split(b"\0"):
        if not entry:
            continue
        meta, encoded = entry.split(b"\t", 1)
        name = encoded.decode("utf-8")
        if not any(fnmatch.fnmatchcase(name, rule) for rule in manifest["include"]):
            continue
        if any(fnmatch.fnmatchcase(name, rule) for rule in manifest.get("exclude", [])):
            continue
        if meta.split()[0] not in (b"100644", b"100755"):
            raise ValueError("snapshot accepts regular blobs only: " + name)
        safe_path(name)
        selected.append(name)
    if not selected:
        raise ValueError("snapshot whitelist selected no sources")
    new_files = {}
    with tempfile.TemporaryDirectory(prefix=".vendor-stage-", dir=root) as directory:
        stage = Path(directory)
        for name in selected:
            data = git(source, "show", commit + ":" + name)
            relative = str(PurePosixPath(manifest["destination"]) / safe_path(name))
            safe_path(relative)
            live = target_path(root, relative)
            if relative not in manifest.get("files", {}) and live.exists():
                raise ValueError("unrecorded local file blocks import: " + relative)
            target = stage / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            new_files[relative] = hashlib.sha256(data).hexdigest()
        # Only manifest-owned obsolete files may be removed, after integrity checks.
        for relative in set(manifest.get("files", {})) - set(new_files):
            target_path(root, relative).unlink()
        for relative in new_files:
            target = target_path(root, relative)
            target.parent.mkdir(parents=True, exist_ok=True)
            (stage / relative).replace(target)
        manifest.update(revision=commit, files=dict(sorted(new_files.items())))
        pending = root / "UPSTREAM.json.new"
        pending.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
        pending.replace(root / "UPSTREAM.json")
    return commit, len(new_files)


# Use --update explicitly; the default is a read-only integrity check.
def main():
    """Validate one manifest or refresh it from a trusted local repository."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshot", type=Path)
    parser.add_argument("--update", action="store_true")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--ref", default="HEAD")
    args = parser.parse_args()
    root = args.snapshot.resolve()
    try:
        manifest = json.loads((root / "UPSTREAM.json").read_text())
        safe_path(manifest["destination"])
        if args.update:
            if args.source is None:
                parser.error("--update requires --source")
            commit, count = refresh(root, manifest, args.source.resolve(), args.ref)
            print("Updated {}: {} files at {}".format(root.name, count, commit))
        else:
            errors = verify(root, manifest)
            if errors:
                raise ValueError("\n".join(errors))
            print(
                "Verified {}: {} files at {}".format(
                    root.name, len(manifest["files"]), manifest["revision"]
                )
            )
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        parser.exit(1, str(error) + "\n")


if __name__ == "__main__":
    main()
