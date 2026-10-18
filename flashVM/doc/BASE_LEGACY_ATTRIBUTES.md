# Object/Array/String attributes / Атрибуты Object, Array и String — v0.205

У загруженных Object/Array/String prototype methods flags 3: DONT_ENUM и
DONT_DELETE, без READ_ONLY. Object.registerClass защищён flags 7, String.fromCharCode
— flags 3 и остаётся writable. Host defaults сохраняются. Это атрибуты уже
установленных методов; availability по версии этим изменением не исправлена.

Установка после загрузки создаёт flags сразу. При обратном порядке копии
attributes готовятся до изменения прототипов. Канонические native callbacks
с default flags 1 получают новые flags; изменённые flags, descriptors и значения
сохраняются. Object commit не меняет native layout. Array и String commits
сохраняют переход в ordinary prototype без length. OOM оставляет прежнее
состояние всех primitive/base прототипов до retry.

## English

Loaded Object/Array/String prototype methods are nonenumerable/nondelete (3),
remaining writable. Object.registerClass is readonly/nondelete (7);
String.fromCharCode is nondelete but writable (3). Host defaults are unchanged.
This changes attributes of installed methods, not their version availability.

Installation after load supplies flags directly. Loading after installation
prepares attribute snapshots before any mutation; only canonical native slots
with default flags are upgraded. Custom flags/descriptors/data remain. Object
commit only swaps attributes; Array/String retain ordinary layout activation
and removal of native length. All commits are allocation/callback-free.

Tests cover SWF4–8/install order, host/custom flags, method writes/delete guards,
static method protection/writability, GC/channel and expanded OOM checks for old
Object/Array/String flags before retry. Reference: pinned Ruffle object.rs,
array.rs/string.rs declarations, commit 2372a4e148a67d68efd796f4477b3cddd4f880cd.
Raw __proto__ semantics, visibility by version, other metadata, customized native
layouts and all roadmap stages remain open. Graph/API formats are unchanged.
