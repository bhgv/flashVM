# Copyright (c) 2026 bhgv
# [GC:] EN: Derive the real wide-owner fixture without legacy-finalizer scaffolding.
# RU: Получить настоящий тест широких владельцев без обвязки прежнего финализатора.
from pathlib import Path
import re


# [GC:] EN: Expand fixture includes from borrowed root, producing owned text.
# Only test .inc files are expanded; missing fragments raise their file error.
# No production implementation or allocator/collector behavior is replaced.
# RU: Раскрыть тестовые include из root в собственный текст. Только тестовые
# .inc раскрываются; отсутствие файла вызывает ошибку. Реализация runtime,
# выделение памяти и работа сборщиков не подменяются.
def expand(root, source):
    return re.sub(
        r'^#include "(test_[^"\n]+\.inc)"$',
        lambda match: expand(root, (root / match[1]).read_text()),
        source,
        flags=re.M,
    )


# [GC:] EN: Build a private fixture in work from real root sources. Remove only
# allocation/publication/cleanup of the deliberately unknown retained finalizer;
# preserve target construction, root/count/free/epoch/claim checks and fault probes.
# Its case 16 depends on retained assistance and must be excluded by the driver.
# Return owned generated path; source-boundary drift raises before compilation.
# RU: Создать в work личный тест из настоящих исходников root. Удалить лишь
# создание/публикацию/очистку намеренно неизвестного резервного финализатора.
# Сохранить построение target, проверки корней/ссылок/free/эпох/захватов и отказы.
# Case 16 требует резервной помощи и исключается драйвером. Вернуть путь;
# изменение границ вызывает отказ до сборки.
def prepare_fixture(root: Path, work: Path):
    source = expand(root, (root / "test_large_owner_release.c").read_text())
    start = source.index("    Type *unknown_type = dtype(unknown_destructor")
    end = source.index("    uchar map[(Slots + 7) / 8];", start)
    source = source[:start] + source[end:]
    start = source.index("    require(Assist || (!unknown_calls")
    end = source.index("    released = 1;", start)
    source = source[:start] + source[end:]
    start = source.index("    native = dvm_runtime_gc_native_try_leave();")
    end = source.index('    fprintf(stderr,\n            "PASS large owner release', start)
    source = source[:start] + source[end:]
    path = work / "concurrent_large_owner.c"
    path.write_text(source)
    return path
