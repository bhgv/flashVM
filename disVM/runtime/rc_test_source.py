# Copyright (c) 2026 bhgv
# EN: Expand the private RC composition for isolated semantic mutation controls.
# RU: Развернуть закрытую композицию RC для изолированных семантических контролей.
from pathlib import Path
import re


# EN: Read production RC in declared order from runtime. Return an owned string;
# only its quoted .inc directives are expanded, without modifying any source.
# Missing fragments raise the normal file error. This helper grants no VM access.
# RU: Прочитать RC из runtime в объявленном порядке. Вернуть собственную строку;
# раскрыть только его .inc без изменения исходников. Отсутствующий файл вызывает
# обычную ошибку чтения. Разрешений на доступ к данным VM этот helper не даёт.
def read_rc_source(runtime):
    runtime = Path(runtime)
    source = (runtime / "runtime_rc.c").read_text()
    return re.sub(
        r'^#include "([^"\n]+\.inc)"$',
        lambda match: (runtime / match[1]).read_text(),
        source,
        flags=re.M,
    )
