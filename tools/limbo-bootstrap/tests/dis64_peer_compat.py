"""Compare real ordinary Dis and Dis64 products for a reviewed peer source catalog."""
from pathlib import Path
import argparse
import runpy
import subprocess
import tempfile


# Execute one native compiler against the same borrowed source path. compiler is
# explicit, target is owned by this run, scalar selects -6. Raise on diagnostics
# or missing products; no fallback compiler or substitute encoder is used.
def compile_image(compiler, source, target, scalar):
    command = [str(compiler)]
    if scalar:
        command.append("-6")
    command += ["-o", str(target), str(source)]
    if target.exists():
        target.unlink()
    result = subprocess.run(command, text=True, capture_output=True, timeout=30)
    if result.returncode or not target.is_file():
        raise RuntimeError(result.stdout+result.stderr)
    return target.read_bytes()


# The reviewed catalog is executable repository test code selected explicitly by
# --catalog. Both versions receive identical source paths/contents and flags.
# Outputs and temporary source files belong only to this run and are discarded.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--catalog", type=Path, required=True)
    args = parser.parse_args()
    catalog = runpy.run_path(str(args.catalog.resolve()))
    count = 0
    with tempfile.TemporaryDirectory(prefix="Dis64 peer compatibility ") as directory:
        root = Path(directory)
        source = root/"probe.b"
        for name, globals_, body, unused in catalog["programs"]():
            source.write_text(catalog["PREFIX"]+globals_+"\n"+body+"\n")
            for scalar in (False, True):
                suffix = ".dis64" if scalar else ".dis"
                reference = compile_image(args.reference.resolve(),source,root/("reference"+suffix),scalar)
                actual = compile_image(args.compiler.resolve(),source,root/("actual"+suffix),scalar)
                if actual != reference:
                    raise RuntimeError("peer byte identity changed: "+name+suffix)
            count += 1
    print("PASS peer compiler byte identity:", count, "sources; ordinary Dis and Dis64 products")


if __name__ == "__main__":
    main()
