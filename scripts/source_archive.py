#!/usr/bin/env python3
"""Create a reproducible source ZIP with the complete Git ancestry of a commit.

Only committed files and reachable history are exported. Private stashes, local
configuration, worktree links, build output and reflogs never enter the archive.
"""
import argparse
import datetime
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
import zipfile

# ZIP stores DOS dates and two-second timestamp units.
ZIP_MIN_YEAR = 1980
ZIP_MAX_YEAR = 2107
ZIP_TIME_RESOLUTION_SECONDS = 2
# Fixed archive compression and ordinary Git file mode for repository metadata.
ZIP_COMPRESSION_LEVEL = 6
GIT_METADATA_MODE = 0o100644
# Keep pack generation independent of host thread count and existing delta reuse.
PACK_WINDOW = 10
PACK_DEPTH = 50


def git(repo, *args, data=None, stdout=subprocess.PIPE):
    """Run Git with user/system configuration isolated from archive construction."""
    env = dict(os.environ)
    env.update(GIT_CONFIG_NOSYSTEM="1", GIT_CONFIG_GLOBAL=os.devnull)
    return subprocess.run(["git", "-C", str(repo), *args], input=data, stdout=stdout,
                          stderr=subprocess.PIPE, env=env, check=True).stdout


def create(repo, revision, output):
    """Export a commit and its reachable Git objects with deterministic ZIP metadata."""
    repo = repo.resolve()
    output = output.resolve()
    if output.exists():
        raise ValueError("output already exists: " + str(output))
    if git(repo, "rev-parse", "--is-shallow-repository").strip() == b"true":
        raise ValueError("complete history is required; shallow repositories are rejected")
    commit = git(repo, "rev-parse", "--verify", "--end-of-options", revision + "^{commit}").decode().strip()
    version = git(repo, "show", commit + ":VERSION.txt").decode().strip()
    if output.name != "FlashVM-source-v" + version + ".zip":
        raise ValueError("output filename must match committed VERSION.txt")
    epoch = int(git(repo, "show", "-s", "--format=%ct", commit))
    date = datetime.datetime.fromtimestamp(epoch, datetime.timezone.utc)
    if not ZIP_MIN_YEAR <= date.year <= ZIP_MAX_YEAR:
        raise ValueError("commit timestamp is outside ZIP date range")
    stamp = (date.year, date.month, date.day, date.hour, date.minute, date.second // ZIP_TIME_RESOLUTION_SECONDS * ZIP_TIME_RESOLUTION_SECONDS)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="flashvm-export-", dir=output.parent) as temporary:
        stage = Path(temporary) / "FlashVM"
        stage.mkdir()
        object_format = git(repo, "rev-parse", "--show-object-format").decode().strip()
        git(stage, "init", "--quiet", "--template=", "--object-format=" + object_format)
        metadata = stage / ".git"
        (metadata / "config").write_text(
            '[core]\n\trepositoryformatversion = 0\n\tfilemode = true\n\tbare = false\n'
            '\tlogallrefupdates = false\n[remote "origin"]\n'
            '\turl = https://bhgv.pythonanywhere.com/git/FlashVM/\n'
            '\tfetch = +refs/heads/*:refs/remotes/origin/*\n', encoding="utf-8")
        # Retain every ancestor of this commit, without copying unrelated local refs.
        pack = Path(temporary) / "history.pack"
        with pack.open("wb") as stream:
            git(repo, "pack-objects", "--stdout", "--revs", "--no-reuse-delta",
                "--no-reuse-object", "--threads=1", "--window=" + str(PACK_WINDOW), "--depth=" + str(PACK_DEPTH),
                data=(commit + "\n").encode(), stdout=stream)
        with pack.open("rb") as stream:
            env = dict(os.environ, GIT_CONFIG_NOSYSTEM="1", GIT_CONFIG_GLOBAL=os.devnull)
            subprocess.run(["git", "-C", str(stage), "-c", "pack.writeReverseIndex=false",
                            "index-pack", "--stdin"], stdin=stream, stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE, env=env, check=True)
        (metadata / "refs/heads/master").write_text(commit + "\n", encoding="ascii")
        (metadata / "HEAD").write_text("ref: refs/heads/master\n", encoding="ascii")
        git(stage, "checkout", "--force", "--quiet", "master")
        # Rebuild index entries from the tree, removing checkout-time stat metadata.
        git(stage, "read-tree", "HEAD")
        git(stage, "fsck", "--full", "--strict")
        if git(stage, "rev-list", "--count", "HEAD") != git(repo, "rev-list", "--count", commit):
            raise ValueError("history count differs after export")
        tree = git(repo, "ls-tree", "-r", "-z", commit)
        modes = {}
        for entry in tree.split(b"\0"):
            if entry:
                metadata_entry, name = entry.split(b"\t", 1)
                modes[os.fsdecode(name)] = int(metadata_entry.split()[0], 8)
        candidate = Path(temporary) / output.name
        with zipfile.ZipFile(candidate, "w", zipfile.ZIP_DEFLATED, compresslevel=ZIP_COMPRESSION_LEVEL) as archive:
            paths = sorted(p for p in stage.rglob("*") if p.is_file() or p.is_symlink())
            for path in paths:
                relative = path.relative_to(stage).as_posix()
                info = zipfile.ZipInfo("FlashVM/" + relative, stamp)
                info.create_system = 3
                mode = modes.get(relative, GIT_METADATA_MODE)
                info.external_attr = mode << 16
                info.compress_type = zipfile.ZIP_DEFLATED
                content = os.fsencode(os.readlink(path)) if path.is_symlink() else path.read_bytes()
                archive.writestr(info, content)
        with zipfile.ZipFile(candidate) as archive:
            if archive.testzip() is not None:
                raise ValueError("archive CRC verification failed")
        candidate.replace(output)
    return commit, hashlib.sha256(output.read_bytes()).hexdigest()


def main():
    """Parse explicit repository, committed revision and versioned output path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--revision", default="HEAD")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    commit, digest = create(args.repo, args.revision, args.output)
    print(commit, digest, args.output)


if __name__ == "__main__":
    main()
