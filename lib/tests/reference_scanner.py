#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
# EN: Protect documentation source/header selection against names appearing only in prose/literals.
# RU: Защитить выбор headers/source в документации от имён только в prose/literals.
from pathlib import Path
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from lib.tools.reference import call_sites, declaration_header
from documentation import reference_anchors


# EN: No arguments. Create isolated raw C fixtures, require real declaration/call
# selection and physical source line preservation; raise on any false prose match.
# RU: Аргументов нет. Создать отдельные raw C fixtures; требовать настоящие
# declaration/call и сохранение физических строк; при false match — исключение.
def main():
    folder = Path(__file__).resolve().parents[2] / 'build/task-tmp'
    folder.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=folder) as directory:
        base = Path(directory)
        decoy = base / 'decoy.h'
        real = base / 'real.h'
        decoy.write_text('/* use send_record(data) in the application */\n#define HELP "send_record(data)"\n')
        real.write_text('int send_record(const void *data);\n')
        if declaration_header([decoy, real], 'send_record') != real:
            raise RuntimeError('documentation chose a comment/string instead of the declaring header')
        if declaration_header([decoy, real], 'absent') is not None:
            raise RuntimeError('nonexistent declaration was invented')
        examples = base / 'examples'
        examples.mkdir()
        (examples / 'sample.c').write_text('void demo(void)\n{\n    puts("ghost(data);");\n    consume(data); /* phantom(data); */\n}\n')
        sites = call_sites(base)
        if 'ghost' in sites or 'phantom' in sites:
            raise RuntimeError('a prose/literal function call became an API example')
        if sites.get('consume') != ('examples/sample.c', 4, 'consume(data);'):
            raise RuntimeError('real call site lost its physical source line or gained comment filler')
        doc = base / 'docs/reference/send_record.md'
        correct = '```c\nconsume(data);\n```\n\n[Complete calling context](../../examples/sample.c#L4)'
        reference_anchors(base, doc, correct)
        stale = correct.replace('#L4', '#L3')
        try:
            reference_anchors(base, doc, stale)
        except ValueError:
            pass
        else:
            raise RuntimeError('in-range stale call-site anchor was accepted')
        implementation = '[Implementation: real.h:1](../../real.h#L1)'
        reference_anchors(base, doc, implementation)

    print('PASS reference scanner: declarations, prose/literal exclusion and real call-site anchors')


if __name__ == '__main__':
    main()
