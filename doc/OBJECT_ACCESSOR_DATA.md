# Хранимые данные accessor / Retained accessor data — v0.136

addProperty сохраняет прежнее собственное data-значение отдельно от getter/setter
в приватном объекте дескриптора. Замена уже виртуального свойства также сохраняет
это значение. Обычное чтение вызывает getter, а исторические Array-методы
читают retained data без getter. Это касается и virtual length. Невызываемый
getter-Object возвращает retained data; для нового свойства оно равно undefined.
Это уточняет описание v0.130: undefined не является безусловным результатом
невызваемого getter при наличии прежних хранимых данных.

Используется обычное свойство скрытого дескриптора: существующие GC/graph корни
переносят значение, строки и циклы без изменения C89 ABI или структуры wire.
Старые дескрипторы без data читаются как undefined. Данные нативных C accessor
без script-дескриптора не выдумываются.

object_accessor_stored_data проверяет data-строку с NUL, ссылку владельца на себя,
virtual length, замену descriptor, обычный script getter и plain getter fallback.
После traced-string GC и queue transfer с GC в принимающем загруженном SWF
контексте slice/concat/join читают сохранённые данные; getter продолжает давать 42.
Правило сверено с pinned Property::data/set_virtual и ScriptObject::add_property_with_case.

Обычное присваивание ещё не обновляет latent data дескриптора; исторический
set_data/move, флаги замены, методы как исходные data, версии и все стадии открыты.

## English

addProperty retains prior own stored data in its hidden descriptor, separately
from executable getter/setter. Replacing a virtual property retains that data.
Ordinary get invokes a function getter; loaded Array methods read retained data
without invoking it, including virtual length. Non-function getters fall back
to retained data, undefined for a new property. This refines v0.130's initial
undefined description for plain getters.

An ordinary hidden descriptor property uses existing GC and graph roots for
strings, objects and cycles without an ABI/wire-structure change. Old descriptors
without data yield undefined; native C accessors without script descriptors do
not acquire fabricated stored values.

object_accessor_stored_data checks counted NUL strings, owner cycles, virtual
length, descriptor replacement, script getter and plain fallback. Traced-string
GC, queue transfer and receiving loaded-context GC preserve values used by
slice/concat/join while public getter still returns 42. Rules follow pinned
Property and ScriptObject implementations. Ordinary assignment latent-data
updates, historical set_data/movement, replacement flags, method data, versions
and every roadmap stage remain open.
