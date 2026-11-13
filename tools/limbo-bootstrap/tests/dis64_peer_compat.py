# Copyright (c) 2026 bhgv
# EN: Compare committed-parser and independent peer compiler Dis/Dis64 products.
# RU: Сравнить Dis/Dis64 продукты committed-parser и независимого peer-компилятора.
"""Compare real ordinary Dis and Dis64 products for a reviewed peer source catalog."""

from pathlib import Path
import argparse
import runpy
import subprocess
import tempfile


# Execute one native compiler against the same borrowed source path. compiler is
# explicit, target is owned by this run, scalar selects -64. Raise on diagnostics
# or missing products; no fallback compiler or substitute encoder is used.
# EN: Borrow compiler/source, own target output. Return exact image bytes; reject
# errors or missing output. scalar selects current DIS64, not a reference VM mode.
# RU: Заимствовать compiler/source, владеть target. Вернуть байты образа; ошибки
# и отсутствие результата дают отказ. scalar выбирает DIS64, не reference VM.
def compile_image(compiler, source, target, scalar):
    command = [str(compiler)]
    if scalar:
        command.append("-64")
    command += ["-o", str(target), str(source)]
    if target.exists():
        target.unlink()
    result = subprocess.run(command, text=True, capture_output=True, timeout=30)
    if result.returncode or not target.is_file():
        raise RuntimeError(result.stdout + result.stderr)
    return target.read_bytes()


# The reviewed catalog is executable repository test code selected explicitly by
# --catalog. Both versions receive identical source paths/contents and flags.
# Outputs and temporary source files belong only to this run and are discarded.
# EN: CLI supplies two explicit compilers and a trusted source catalog. Compare
# ordinary/Dis64 bytes for identical source paths; own temporary files through exit.
# RU: CLI задаёт два компилятора и доверенный каталог. Сравнить обычные/Dis64 байты
# при одинаковых путях исходника; владеть временными файлами до завершения.
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
        source = root / "probe.b"
        for entry in catalog["programs"]():
            if len(entry) == 3:
                name, text, unused = entry
            elif len(entry) == 4:
                name, globals_, body, unused = entry
                text = catalog["PREFIX"] + globals_ + "\n" + body + "\n"
            else:
                raise ValueError("unsupported peer catalog entry shape")
            source.write_text(text)
            for scalar in (False, True):
                suffix = ".dis64" if scalar else ".dis"
                reference = compile_image(
                    args.reference.resolve(), source, root / ("reference" + suffix), scalar
                )
                actual = compile_image(
                    args.compiler.resolve(), source, root / ("actual" + suffix), scalar
                )
                if actual != reference:
                    raise RuntimeError("peer byte identity changed: " + name + suffix)
            count += 1
    print("PASS peer compiler byte identity:", count, "sources; ordinary Dis and Dis64 products")


if __name__ == "__main__":
    main()
