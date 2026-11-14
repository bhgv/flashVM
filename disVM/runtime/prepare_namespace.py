# Copyright (c) 2026 bhgv
# EN: Remove only generated namespace stubs implemented by the hosted VFS.
# RU: Удалить только generated namespace stubs, реализованные в hosted VFS.
import argparse
from pathlib import Path

ENTRIES = (
    "Sys_pctl",
    "Sys_chdir",
    "Sys_bind",
    "Sys_unmount",
)  # Native Sys entries implemented together.


# EN: Adapt owned generated text, refusing partial or changed stub bodies.
# Reapplying to an already adapted view is harmless; vendor input is untouched.
# RU: Изменить generated text, отклоняя частичные/изменённые stub bodies.
# Повтор безопасен; vendor input не изменяется. Вернуть новый текст либо raise.
def adapt(source):
    present = [name for name in ENTRIES if f"void {name}(" in source]
    if not present:
        return source
    if len(present) != len(ENTRIES):
        raise ValueError("partial namespace stub generation")
    for name in ENTRIES:
        stub = (
            "\n/* Reject an unavailable host service without reporting success. */\n"
            f"void {name}(void *frame)\n{{\n    USED(frame);\n"
            f'    error("unsupported native host service: {name}");\n}}\n'
        )
        if source.count(stub) != 1:
            raise ValueError("namespace stub drift: " + name)
        source = source.replace(stub, "", 1)
    return source


# EN/RU: Adapt only --output/unsupported.c; no compiler or runtime execution.
# Изменить только --output/unsupported.c; без запуска compiler/runtime.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    arguments = parser.parse_args()
    path = arguments.output / "unsupported.c"
    path.write_text(adapt(path.read_text()))


if __name__ == "__main__":
    main()
