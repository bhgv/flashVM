# Math namespace attributes / Атрибуты Math

Loaded SWF 4–8 Math constants and methods use DONT_ENUM|DONT_DELETE|READ_ONLY.
Ordinary assignment is ignored, deletion returns false, and own enumeration
omits the fields. The existing host namespace retains writable/deletable defaults:
constant flags 0, method flags DONT_ENUM. This change preserves the existing
host-derived numeric constant values and Math operation implementations.

When the standard library was installed before the first SWF load, activation
prepares a complete copied attribute table before publishing any builtin changes.
Only unchanged canonical methods with default host flags and unchanged numeric
constants without custom metatables and with default flags receive protection. Custom values/flags/accessors,
deleted fields and replaced/global-accessor Math namespaces remain untouched.
The raw global/slot selection does not call getters. The live global retains
the namespace until commit. Allocation failure publishes no partial table.

Already loaded contexts skip Math activation. Clearing protection with
ASSetPropFlags therefore remains effective after another load, including after
ordinary assignment replaces a field. Graph transport preserves the resulting
attributes without requiring standard installation in the receiving context.

В SWF 4–8 методы и константы Math скрыты от перечисления, защищены от удаления
и доступны только для чтения. Host без SWF сохраняет прежние атрибуты: 0 для
констант, DONT_ENUM для методов. Числовые значения и реализации операций не
менялись; точность округления и трансцендентных функций остаётся отдельной задачей.

При первой загрузке SWF заранее подготавливается таблица атрибутов. Меняются
только канонические нетронутые поля с исходными host-флагами. Пользовательские
значения, флаги, аксессоры, удалённые поля и заменённый глобал Math сохраняются;
getters не вызываются. При отказе памяти ничего не публикуется. Повторная
загрузка не восстанавливает защиту, явно снятую в уже загруженном контексте.
Перенос графа сохраняет атрибуты без установки библиотеки в приёмнике.

Tests: math_loaded_attributes, math_custom_attributes, math_activation_allocation;
26 fields, C/interpreter/explicit/automatic JIT writes, enumeration/deletion,
first-load OOM retries, custom host state, reload and graph transport/GC.
Reference: pinned Ruffle globals/math.rs OBJECT_DECLS,
commit 2372a4e148a67d68efd796f4477b3cddd4f880cd.
All roadmap stages remain open.

## v0.483 — точность констант / exact constants

RU: Загруженный Math получает фиксированные binary64 значения восьми констант.
В частности, LOG10E равен 0.4342944819032518 вместо округления 1/log(10),
которое на проверенной Linux libm отличается на один младший бит. Host-only
значения сохранены. Первая загрузка обновляет только канонические host-поля
без пользовательских флагов/метатаблиц, подготавливая копии значений и атрибутов
до атомарной фиксации. Ошибка выделения памяти сохраняет числа и флаги.

EN: Loaded Math constants no longer depend on libm evaluation. All eight values
match an independent binary64 byte oracle. First-load activation replaces only
untouched host constants; custom values, flags, primitive metadata, deleted
slots and already loaded state survive. Existing host-only evaluation remains.
This does not establish correctly rounded Math functions or full Flash numeric
compatibility. Tests cover C, interpreter, explicit/automatic JIT, serialized
graph transfer/GC, first-load allocation failures, custom state and reload.

Sources: [Ruffle Math declarations](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/math.rs)
use Rust f64 constants; [Rust 1.90.0 constants](https://github.com/rust-lang/rust/blob/1.90.0/library/core/src/num/f64.rs)
supply the fixed values. Decimal literals are rounded by the C++ compiler;
byte-oracle tests verify the emitted values on the tested platform.
