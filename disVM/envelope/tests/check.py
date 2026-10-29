"""Exercise the built inspector and shared C99 example with independent byte checks."""

from pathlib import Path
import json
import subprocess
import sys
import tempfile


# Run explicit executables and literal paths. Capture diagnostics and preserve
# failure status; a file fixture never authorizes execution of its payloads.
def call(arguments, good=True):
    result = subprocess.run(
        [str(a) for a in arguments], capture_output=True, text=True, timeout=20
    )
    if (result.returncode == 0) != good:
        raise RuntimeError(str(arguments) + "\n" + result.stdout + result.stderr)
    return result


# Inspect the actual built tool; canonical rewrites must preserve every byte.
# Own all files, including an existing output sentinel tested on invalid input.
def main():
    inspector, example = (Path(a).resolve() for a in sys.argv[1:3])
    golden_path = Path(sys.argv[3]) if len(sys.argv) > 3 else Path(__file__).resolve().parents[1] / "examples/sample.dis64.hex"
    golden = bytes.fromhex(golden_path.read_text())
    with tempfile.TemporaryDirectory(prefix="Dis64 envelope CLI ") as directory:
        work = Path(directory)
        image, rewritten = work / "sample Я.dis64", work / "rewritten.dis64"
        call([example, image])
        original = image.read_bytes()
        if original != golden:
            raise RuntimeError("C99 example differs from the packaged byte fixture")
        if original[:8] != bytes.fromhex("44697336340d0a1a"):
            raise RuntimeError("example did not emit accepted magic")
        report = json.loads(call([inspector, image]).stdout)
        if not report["envelope_only"] or report["execution_supported"]:
            raise RuntimeError("inspector claimed executable support")
        if report["abi_id"] != 1 or [s["kind"] for s in report["sections"]] != [
            1,
            2,
            3,
            7,
        ]:
            raise RuntimeError("inspector directory differs")
        call([inspector, image, "--rewrite", rewritten])
        if rewritten.read_bytes() != original:
            raise RuntimeError("canonical rewrite changed opaque bytes")
        # Invalid version, truncated content and trailing bytes fail before
        # opening a destination. The same applies to classic/NC64 magic.
        mutated = bytearray(original)
        mutated[11] = 1
        for invalid in (bytes(mutated), original[:-1], original + b"!", b"NC64"):
            image.write_bytes(invalid)
            rewritten.write_bytes(b"KEEP")
            call([inspector, image, "--rewrite", rewritten], False)
            if rewritten.read_bytes() != b"KEEP":
                raise RuntimeError("rejected inspection changed existing output")
        call([inspector], False)
    print(
        "PASS built Dis64 inspector/C99 example: report, opaque rewrite, rejection, no execution claim"
    )


if __name__ == "__main__":
    main()
