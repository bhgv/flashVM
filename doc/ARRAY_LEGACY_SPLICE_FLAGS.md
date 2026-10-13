# Loaded native splice flags / Флаги нативного splice — v0.155

Loaded native splice с собственными флагами индексов или READ_ONLY length идёт
через live raw операции общего пути. Перенос сохраняет source index и его
flags; READ_ONLY блокирует последующую запись в эту позицию. Native length
отдельно удаляет хвост. Protected tail не удерживает новую length; readonly
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
raw operations. Copying retains source properties and their flags, so readonly
continues to block later writes. Native length deletes tails separately.
Protected tails do not clamp length;
readonly length permits raw outside slots. Ordinary unflagged arrays keep staging
and existing OOM checks while fast splice now bypasses watchers.
SWF6..8 tests cover all eight source masks, protected tails, readonly length with
index growth, fast watcher bypass and inherited hole getters without calls.
Routed flagged arrays retain explicit generic 65536/INT32_MAX limits; large
flagged arrays are not claimed supported. Native accessor data, sort mutation
flags, case/version rules and all roadmap stages remain open.

## v0.161 correction / Исправление переноса

Полный splice_internal в pinned Ruffle 2372a4e копирует собственные данные,
но не удаляет source index. Удаление хвоста выполняет только native set_length.
Исправлено прежнее описание v0.154/v0.155: READ_ONLY источника не исчезает при
переносе и блокирует последующую запись; все flags сохраняются на месте.
Borrowed receiver сохраняет собственный хвост за новой length. Отсутствующий
источник по-прежнему удаляет destination с учётом DONT_DELETE.
Проверены все 8 масок, native shrinking и borrowed tail. Ограничения размера,
callback ordering и все стадии остаются открытыми.

The complete pinned splice_internal copies own data without deleting the source.
Only native set_length deletes trailing elements. This corrects v0.154/v0.155:
source READ_ONLY survives copying and blocks later writes; indexed flags remain
in place. Borrowed receivers retain own tails beyond their new length. Absent
sources still delete destinations subject to DONT_DELETE. Tests cover all eight
masks, native shrinking and borrowed tails. Size limits, callback ordering and
all roadmap stages remain open.
