# Copyright (c) 2026 bhgv
# EN: Audit explicitly reviewed tutorial sources; never synthesize teaching prose from syntax.
# RU: Проверять явно отредактированные учебные исходники; не сочинять пояснения по синтаксису.
from pathlib import Path
import sys

STOCK = (  # These former automatic phrases add no operation-specific explanation.
    'Choose a branch using this condition',
    'Continue the surrounding expression with these operands',
    'Invoke main with these explicit arguments',
    'Declare operation-local storage; its initializer specifies the initial value',
    'Ownership/threading: retain the original contract immediately above',
)


# EN: Check the review marker, file size and known boilerplate in borrowed source paths.
# Return None; raise with a file/line diagnostic on failure. This lexical check is
# deliberately not a proof of comment accuracy; a maintainer must review behavior.
# RU: Проверить marker review, размер и известные шаблонные фразы в paths.
# Возврата нет; при ошибке указать файл/строку. Лексическая проверка не доказывает
# точность комментариев; поведение должен проверить сопровождающий.
def audit(paths):
    for path in paths:
        source = path.read_text()
        if 'standalone-reviewed:' not in source:
            raise ValueError(f'{path}: review marker missing; author/review the contracts first')
        lines = source.splitlines()
        if len(lines) > 500:
            raise ValueError(f'{path}: more than 500 source lines')
        for number, line in enumerate(lines, 1):
            if any(phrase in line for phrase in STOCK):
                raise ValueError(f'{path}:{number}: replace stock prose with an operation-specific explanation')
        print('PASS reviewed-source lexical audit:', path)


# EN: Optional CLI paths select manually reviewed files; default to all authored
# tutorials in both packages. Inherited library sources are outside this audit.
# RU: Необязательные CLI paths выбирают отредактированные файлы; по умолчанию все
# примеры обоих пакетов. Унаследованные library sources не входят в этот audit.
def main():
    root = Path(__file__).resolve().parents[1]
    tutorials = [path for package in ('styx', 'libunibus')
                 for path in (root / package / 'examples').rglob('*')
                 if path.suffix in ('.c', '.h', '.py') and '__pycache__' not in path.parts]
    paths = [Path(value) for value in sys.argv[1:]] or sorted(tutorials) + [
        root / 'styx/tests/live_registry.c', root / 'styx/tests/http_contract.py']
    audit(paths)


if __name__ == '__main__':
    main()
