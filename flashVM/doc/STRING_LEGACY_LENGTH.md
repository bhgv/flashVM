# Boxed String.length / Собственная длина boxed String — v0.187

String, созданный в загруженном контексте, получает own data length по количеству
UTF16 units и flags DONT_ENUM | DONT_DELETE. Свойство изменяемо, в том числе
на Object; READ_ONLY защищает data. Stored text и строковые методы не меняются
при изменении length. Незагруженные String сохраняют прежний native getter.
Graph уже поддерживает обычные свойства native String: format/ABI не меняются.

string_legacy_length проверяет SWF4..8/host, install-before-load, initial flags,
числовую запись, readonly, charAt независимо от length, Object/cycle length с
NUL marker, traced GC/channel/destination GC, сохранение flags/text/aliasing,
nondelete guard и удаление own свойства после снятия DONT_DELETE.
Длина прототипа, load-after-host-instance поведение, boxed overrides, память
при произвольном native callback GC и все стадии остаются открытыми.

## English

Strings constructed in loaded contexts get own UTF16-count length data with
DONT_ENUM | DONT_DELETE. The property is writable, including Object values;
READ_ONLY protects data. Changing public length does not change stored text or
String methods. Unloaded constructions retain the native getter. Existing graph
properties support the new layout without format/ABI changes.

Tests cover SWF4..8/host, installation before loading, flags, numeric/readonly
writes, charAt independent of public length, Object/cycle length with a counted
NUL marker, traced GC/channel/destination GC, flags/text/alias preservation,
nondelete protection and own deletion after clearing DONT_DELETE. Prototype length,
loading after host-instance creation, boxed overrides, arbitrary native callback
GC and all stages remain open. Reference: pinned String constructor define_value.
