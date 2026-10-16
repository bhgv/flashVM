# Версионные атрибуты AVM1 / AVM1 version attributes — v0.487

RU: Загруженный ASSetPropFlags сохраняет все UInt16-биты после существующего
насыщающего преобразования чисел. Три младших бита по-прежнему управляют
перечислением, удалением и записью. Старшие биты проверяются при обычном чтении
данных, getter и нативных методов, при поиске __resolve и super. Скрытое own
свойство не мешает чтению видимого свойства прототипа. Видимость пересчитывается
по текущей версии SWF при повторной загрузке, без изменения самих атрибутов.

EN: The exclusion masks are 0x7ff8 (SWF≤4), 0x7480 (5), 0x7500 (6),
0x7000 (7), and 0x6000 (8). For example bit 0x1000 hides a field in SWF4–7
and permits it in SWF8. Bit 0x8000 is retained but does not hide it in these
versions. All bits above the three ordinary attributes clear after a successful
writable data replacement. Readonly writes retain both data and version bits.
Own script setters still execute on hidden/readonly fields, while hidden
inherited virtual setters are skipped. Readonly descriptor data stays protected.
The inherited method selected for an AVM1 call remains the owner used for super;
a hidden own member cannot cause the parent method to execute twice.

RU: hasOwnProperty и propertyIsEnumerable не учитывают версионное скрытие;
перечисление зависит только от DONT_ENUM. Raw-данные Array также остаются
доступны его loaded-методам. Перезаписанные элементы и изменённая raw length
сбрасывают версионные биты, нетронутые поля сохраняют их. ASSetPropFlags может
снять атрибуты скрытого свойства без вызова getter; сопоставление имён остаётся
нечувствительным к регистру, в том числе для counted NUL-имён.

EN: Embedding attribute setters keep the documented three-bit input mask.
Attribute queries can return the extended stored UInt16 value; loaded
ASSetPropFlags can modify its version bits. Host-only reads do not apply SWF
visibility masks. Existing direct embedding function-registry dispatch keeps
its previous priority rules; AVM1 member calls use value-based lookup.
The graph wire layout is unchanged, but pre-v0.487 readers reject attributes
above 7. New readers preserve UInt16 attributes and reject values above 65535
before destination object allocation, including packets with a valid CRC.

Tests: property_version_visibility (bits 3–15/all-bits, SWF4–8, data/getter/native
method/Array index, C and three AVM1/JIT modes, membership/keys/raw slice),
property_version_updates (writable/readonly data, descriptor and index stores),
property_version_inheritance (hidden/visible inherited setter and Array length),
property_version_transport (all source/destination SWF pairs, host destination,
counted names/cycles, GC and reload), property_version_array_raw (sort/reverse/
shift/unshift/push/pop/splice), property_version_allocation (new/new[] AS flag
and decode rollback/retry, CRC-valid oversized flags and encoder rejection),
property_version_super (C, direct invoke and interpreter/explicit/auto JIT),
and property_version_resolution (__resolve, primitive conversion and restored graphs).
property_flags_conversion now checks successful UInt16 saturation beyond 7.

References: pinned Ruffle
[property attributes and VERSION_MASKS](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/property.rs),
[ASSetPropFlags](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/object.rs),
and [ScriptObject data/setter/membership rules](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs).
This is source-based comparison, not a new original Flash Player oracle run.
Builtin declarations have not yet received a complete version-annotation audit;
MovieClip intrinsic-field interactions, metatable extensions, SWF>8 and all nine
roadmap stages remain open. Public C89 signatures/callback IDs are unchanged.


### v0.488 — exhaustive UInt16 regression matrix / полный перебор

`property_version_exhaustive` checks all 65,536 attribute combinations for each
SWF4–8 version (327,680 cases). Independent literal masks select the expected
own/prototype value. Each case also checks own membership, clear-before-set
precedence using a rotated clear mask, readonly preservation of the raw value,
version-bit removal on successful writes, and deletion independently of read
visibility. Periodic collection checks the rooted lookup graph.

`property_version_exhaustive_execution` reuses one function per version and
execution mode across the entire attribute range: interpreter, explicitly bound
native JIT and automatic JIT (983,040 calls when native JIT is available).
Every invocation checks both the result and native-entry counter; frame/depth
state must return to its original state. This covers changing attributes between
calls without recreating the function or explicit compiled body.

Проверка охватывает все UInt16-наборы для SWF4–8, включая одновременные
биты видимости, readonly и запрета удаления. Повторные вызовы проверяют
наблюдаемость изменения атрибутов интерпретатором и JIT. Это регрессионная
проверка существующей реализации, а не новая функциональность плеера.
Only the current native host backend executes here; this does not establish
foreign-architecture or Windows/macOS/Android runtime acceptance. The reference
masks are source-based; no original Flash Player oracle was run.
