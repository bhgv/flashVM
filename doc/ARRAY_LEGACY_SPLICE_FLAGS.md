# Loaded native splice flags / Флаги нативного splice — v0.155

Loaded native splice с собственными флагами индексов или READ_ONLY length идёт
через live raw операции общего пути. Flags источника читаются после каждого
удаления: READ_ONLY без DONT_DELETE исчезает при удалении и не блокирует будущую
запись в ту же позицию. Protected tail не удерживает новую length; readonly
length допускает raw slots за её пределами. Быстрый путь обычных массивов
теперь обходит watcher, сохраняя staging и существующие OOM проверки.

array_legacy_splice_flags проверяет SWF6..8 и все 8 масок source flags,
nondelete tail, readonly length с ростом индексов, fast watcher bypass и
prototype getter на hole без вызова. Generic ограничения 65536/INT32_MAX теперь
явно применяются к маршрутизированным flagged массивам; большие flagged arrays
не заявлены поддержанными. Native C accessor data, sort mutation flags,
case/version и все стадии остаются открытыми.

## English

Loaded native splice with own indexed flags or readonly length uses live generic
raw operations. Source deletion removes readonly when nondelete is absent,
permitting later writes to that position. Protected tails do not clamp length;
readonly length permits raw outside slots. Ordinary unflagged arrays keep staging
and existing OOM checks while fast splice now bypasses watchers.
SWF6..8 tests cover all eight source masks, protected tails, readonly length with
index growth, fast watcher bypass and inherited hole getters without calls.
Routed flagged arrays retain explicit generic 65536/INT32_MAX limits; large
flagged arrays are not claimed supported. Native accessor data, sort mutation
flags, case/version rules and all roadmap stages remain open.
