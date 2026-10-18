# Loaded arguments Array / Array аргументов загруженного SWF

v0.217 builds loaded script arguments as native Arrays using the retained builtin
Array prototype and length flags DONT_ENUM|DONT_DELETE (3). Initial own elements
are copied without prototype setter/getter dispatch. Strings receive context-owned
counted copies, preserving embedded NUL and metatables. Named parameters and
arguments indices are independent in loaded execution: changing either does not
update the other. Unloaded host calls keep the existing generic mapped-argument
contract, including their accessor/activation lifetime model.

В загруженном SWF arguments — Array с сохранённым builtin-прототипом и флагами
length=3. Исходные элементы копируются напрямую, без inherited callbacks;
строки сохраняют собственные counted-байты, NUL и метатаблицу. Именованный
параметр и arguments[i] изменяются независимо. Незагруженный host-контекст
сохраняет прежние связанные аргументы и корни дескрипторов.

Reference: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/function.rs load_arguments (cloned actual values and separately
installed parameters) and core/src/avm1/globals/array.rs ArrayBuilder (retained
array prototype, raw own values, protected nonenumerable length).
`avm1_arguments_array` checks both write directions, host retention, SWF4–8,
prototype caching after public Array.prototype replacement, failing inherited
index callbacks, counted string ownership, Array join, GC and channel transfer.
`avm1_arguments_caller` continues checking caller/callee graphs, exceptions and
allocation failures under the new Array layout. No ABI/graph format change.

Тест проверяет перечисленные сценарии; caller/callee, исключения и OOM дополнительно
проверяет avm1_arguments_caller. ABI и формат графов прежние. Upstream arguments
SWF имеет версию 15 и не используется как исполняемый эталон SWF<=8. Полная
совместимость Function2/областей видимости остаётся открытой.

The inspected upstream arguments SWF has header 15 and is excluded from SWF<=8
runtime evidence. Complete Function2/scope/version equivalence remains open.
Without standard installation, imported script calls use the existing Array
factory fallback; this is not proof of full system-prototype canonicalization.

Raw own __proto__ reflection/deletion remains part of the open shared prototype
model; native Array layout here does not certify that separate behavior.
Собственное __proto__ и его отражение/удаление остаются частью незавершённой
общей модели прототипов; эта правка их не подтверждает.

Validation / Проверка: Linux GCC/G++ 13.3, C90/C++98: AGG 375 and Cairo
Release 374 tests passed (two/eight launch-permission retries). Seven selected
AVM1 tests passed in NO_OS_PORTS and heap/global ASan + UBSan, with stack/leak
checks explicitly disabled. All 17 unchanged selected original SWF<=8 traces
still match byte-for-byte, with zero exits and empty stderr. No real non-Linux
platform run or completed roadmap stage is claimed.
