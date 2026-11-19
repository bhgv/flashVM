# Copyright (c) 2026 bhgv
# EN: Give native error text the same OS-thread lifetime as errno.
# RU: Привязать native-текст ошибки к потоку ОС, как errno.
import argparse
from pathlib import Path


# EN: Adapt private source text, rejecting drift; keep imported files unchanged.
# RU: Адаптировать личный source, отвергая расхождения; импорт не менять.
def prepare(source):
    before = 'static char errstring[ERRMAX];'
    if source.count(before) != 1:
        raise ValueError('native error storage boundary drift')
    return source.replace(before, 'static _Thread_local char errstring[ERRMAX]; /* Native text follows errno ownership. */')


# EN: Read CLI source and write build-owned output; raise on preparation failure.
# RU: Читать CLI source и записать личный output сборки; при сбое вызвать исключение.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(prepare(args.source.read_text()))


if __name__ == '__main__':
    main()
