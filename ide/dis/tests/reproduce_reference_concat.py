"""Opt-in old-reference VM probe; this is not a production acceptance test.

Reproduce self-concatenation after the bulk-render allocation workload. Always
record both emulator modes, including failures. No retries, expected-failure
conversion, source edits, upstream updates or production output replacement.
"""

import argparse
from pathlib import Path
import subprocess


# Build isolated probe modules with a caller-selected matching compiler/header/VM trio.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--limbo", required=True, type=Path)
    parser.add_argument("--module-dir", required=True, type=Path)
    parser.add_argument("--emu", required=True, type=Path)
    parser.add_argument("--build", required=True, type=Path)
    args = parser.parse_args()
    root = args.build.resolve()
    if root.exists() and any(root.iterdir()):
        parser.error("--build must be a new or empty directory")
    root.mkdir(parents=True, exist_ok=True)
    source_root = Path(__file__).resolve().parent.parent
    source = (source_root / "tests/replaceall_test.b").read_text(encoding="utf-8")
    first = source.index("    # Reject expansion beyond the document limit")
    prefix = source[:first]
    # Keep the exact ordinary workload, then stop BEFORE sending the huge string
    # to the owner: a crash here is outside ReplaceAll admission/rendering.
    probe = (
        prefix
        + """    huge := "x";
    while(len huge < core->MaxCharacters)
        huge += huge;
    sys->print("CONSTRUCTED %d characters\\n", len huge);
    halt();
}
"""
    )
    probe_path = root / "concat_probe.b"
    probe_path.write_text(probe, encoding="utf-8")
    for name in ("dev", "chan", "prog", "fd", "net", "net.alt", "env"):
        (root / name).mkdir()
    for source_path, output in [
        (source_root / "idecore.b", "idecore.dis"),
        (probe_path, "concat_probe.dis"),
    ]:
        subprocess.run(
            [
                str(args.limbo.resolve()),
                "-I",
                str(source_root),
                "-I",
                str(args.module_dir.resolve()),
                "-o",
                str(root / output),
                str(source_path),
            ],
            check=True,
        )
    for mode in (0, 1):
        result = subprocess.run(
            [
                str(args.emu.resolve()),
                "-c" + str(mode),
                "-r" + str(root),
                "-d",
                "/concat_probe.dis",
            ],
            capture_output=True,
            timeout=30,
            cwd=root,
        )
        stdout = result.stdout.decode("utf-8", errors="replace")
        stderr = result.stderr.decode("utf-8", errors="replace")
        (root / ("mode-%d.stdout" % mode)).write_text(stdout, encoding="utf-8")
        (root / ("mode-%d.stderr" % mode)).write_text(stderr, encoding="utf-8")
        print("mode=%d exit=%d\n%s%s" % (mode, result.returncode, stdout, stderr), flush=True)


if __name__ == "__main__":
    main()
