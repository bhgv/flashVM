# Copyright (c) 2026 bhgv
# EN: Insert a host-only metadata hook at the existing compiler output boundary.
# RU: Подключить метаданные в существующей точке вывода hosted-компилятора.
import argparse
from pathlib import Path


# EN: Prepare a build copy of the compiler source; fail on anchor drift.
# RU: Подготовить сборочную копию компилятора и проверить точку вставки.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    header = '#include "limbo.h"\n'
    anchor = '\t\tdispath();\n'
    source = args.source.read_text()
    if source.count(header) != 1 or source.count(anchor) != 1:
        parser.error("compiler output boundary changed")
    source = source.replace(header, header + '#include "styx_export.h"\n')
    source = source.replace(anchor, anchor + "\t\tlimbo_styx_export(impdecl, ldts);\n")
    args.output.write_text(source)


if __name__ == "__main__":
    main()
