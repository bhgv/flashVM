# Copyright (c) 2026 bhgv
# EN: Read original mkfile common/import/POSIX lists without compiling foreign OS alternatives.
# RU: Читать original mkfile common/import/POSIX lists, не компилируя foreign OS alternatives.
from pathlib import Path
import sys


# EN/RU: Read one continued mk assignment, preserving original source order.
# Читать continued mk assignment, сохраняя original source order.
def assignment(path, name):
    active = False
    result = []
    for line in path.read_text().splitlines():
        if not active:
            if not line.startswith(name + "="):
                continue
            line = line[len(name) + 1:]
            active = True
        if line.lstrip().startswith("#"):
            break
        result.extend(line.replace("\\", "").split())
        if not line.endswith("\\"):
            break
    if not result:
        raise ValueError("missing original mkfile assignment " + name)
    return result


# EN/RU: Print semicolon list for CMake; original target model selects actual native64 FP/PC code.
# Выдать semicolon list для CMake; target model выбирает actual native64 FP/PC code.
def main():
    root = Path(sys.argv[1])
    arch = sys.argv[2]
    values = assignment(root / "mkfile", "COMMONFILES") + assignment(root / "mkfile", "IMPORTFILES")
    values += [name for name in assignment(root / "mkfile-Posix", "TARGFILES") if name != "$IMPORTFILES"]
    result = []
    for name in values:
        name = name.replace("$SYSTARG", "Linux").replace("$OBJTYPE", arch).replace(".$O", ".c")
        if not (root / name).is_file() or name in result:
            raise ValueError("invalid or duplicate original library source " + name)
        result.append(name)
    print(";".join(result))


if __name__ == "__main__":
    main()
