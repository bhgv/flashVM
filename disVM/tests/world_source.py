# Copyright (c) 2026 bhgv
# [GC:] EN: Expand the private world composition for isolated semantic controls.
# RU: Раскрыть закрытую композицию мира для изолированных семантических контролей.
import re


# [GC:] EN: Return owned production world text from borrowed host directory.
# Expand only its declared native_world fragments; do not modify sources.
# Missing files raise their file errors. No runtime operation is substituted.
# RU: Вернуть собственный текст мира из host. Раскрыть лишь объявленные
# модули native_world без изменения исходников. Ошибки файлов передаются;
# операции runtime не подменяются.
def read_world_source(host):
    source = (host / "native_world.c").read_text()
    return re.sub(
        r'^#include "(native_world_[^"\n]+\.inc)"$',
        lambda match: (host / match[1]).read_text(),
        source,
        flags=re.M,
    )
