"""Check built C99 pilot writers against Inferno's independent whole-file golden."""

from pathlib import Path
import json
import sys
import tempfile

from check import call


# Borrow explicit built tools and a hand-written upstream fixture. Own temporary
# files and compare exact bytes, then validate the real semantic CLI/report.
def main():
    inspector, example, fixture = (Path(a).resolve() for a in sys.argv[1:4])
    golden = bytes.fromhex(fixture.read_text())
    with tempfile.TemporaryDirectory(prefix="Dis64 real pilot ") as directory:
        work = Path(directory)
        image, copied = work / "pilot Я.dis64", work / "copy.dis64"
        call([example, image])
        if image.read_bytes() != golden or len(golden) != 180:
            raise RuntimeError("shared C99 pilot differs from independent golden")
        report = json.loads(
            call([inspector, image, "--pilot", "--rewrite", copied]).stdout
        )
        if (
            report["envelope_only"]
            or report["execution_supported"]
            or not report["scalar_pilot"]
        ):
            raise RuntimeError("semantic pilot check claimed VM execution")
        if [
            report[k]
            for k in ("data_bytes", "frame_bytes", "entry_pc", "instruction_count")
        ] != [4, 64, 0, 2]:
            raise RuntimeError("pilot metadata differs")
        if copied.read_bytes() != golden:
            raise RuntimeError("checked pilot rewrite changed bytes")
        initialized = bytes.fromhex((fixture.parent / "pilot-init42.hex").read_text())
        call([example, image, "--initialized"])
        report_init = json.loads(call([inspector, image, "--pilot", "--rewrite", copied]).stdout)
        if image.read_bytes() != initialized or copied.read_bytes() != initialized or report_init["initializer_count"] != 1:
            raise RuntimeError("initialized C99 pilot differs from golden/report")
        if report.get("initializer_count") != 0 or len(initialized) != 192:
            raise RuntimeError("default/initialized payload extents differ")
        # Corrupt metadata name ID while preserving the structural envelope.
        broken = bytearray(golden)
        broken[report["sections"][0]["offset"] + 3] = 1
        image.write_bytes(broken)
        call([inspector, image])
        copied.write_bytes(b"KEEP")
        call([inspector, image, "--pilot", "--rewrite", copied], False)
        if copied.read_bytes() != b"KEEP":
            raise RuntimeError("pilot semantic rejection overwrote destination")
        call([example], False)
    print(
        "PASS built C99 real pilot: exact180/192 bytes, semantic links, checked rewrite, no VM claim"
    )


if __name__ == "__main__":
    main()
