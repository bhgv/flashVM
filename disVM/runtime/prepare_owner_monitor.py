# Copyright (c) 2026 bhgv
# [GC:] EN: Bind hosted heap observers to their native execution threads.
# RU: Привязать hosted-наблюдателей кучи к их нативным потокам исполнения.
import argparse
from pathlib import Path
import re


# [GC:] EN: Convert one borrowed, freshly generated heap source to a TLS hook.
# Return owned text; reject missing, duplicate or already adapted declarations.
# Callback code pointers are not GC roots. No runtime execution or lock is added.
# RU: Преобразовать один свежий заимствованный исходник кучи в TLS-hook.
# Вернуть личный текст; отсутствие, повтор или готовую адаптацию отвергнуть.
# Указатель кода не является GC-корнем; исполнение и блокировки не добавляются.
def adapt(source):
    pattern = r"^void[ \t]+\(\*heapmonitor\)\(int, void\*, ulong\);$"
    replacement = (
        "__thread void (*heapmonitor)(int, void *, ulong); "
        "/* [GC:] Owner-local hook; new resident/collector threads start with NULL. */"
    )
    result, count = re.subn(pattern, replacement, source, flags=re.MULTILINE)
    if count != 1:
        raise ValueError("owner-local heap monitor declaration changed")
    return result


# EN: CLI output selects the completed source view. Own only heap.c's rewrite;
# fail before writing on declaration drift. Header storage is set by CMake.
# RU: CLI output задаёт готовый source-view. Владеть лишь перезаписью heap.c;
# при дрейфе декларации отказать до записи. Storage header задаёт CMake.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    heap = args.output / "heap.c"
    heap.write_text(adapt(heap.read_text()))


if __name__ == "__main__":
    main()
