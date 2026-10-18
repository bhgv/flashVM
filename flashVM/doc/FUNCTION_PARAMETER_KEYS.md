# Parameter key identity / Идентичность имён параметров — v0.85

## English

UTF-16-equivalent formal names now share one lexical binding. Only the rightmost
named duplicate parameter participates in the arguments alias map. The previous
byte-key seen set could map both a scalar spelling and its WTF-8 surrogate-pair
spelling to the same binding: writing arguments[0] incorrectly changed the last
parameter. Formal-key canonicalization now precedes alias selection.

The counted AVM1 factory accepts string-valued parameter names, including NUL
and lone surrogates. Parameter metadata, alias descriptors, getter/setter lookup,
function name metadata and named AVM1 calls retain full byte counts. Definition
records canonicalize their native names before publishing lexical/global/script
registry bindings. Counted native/script debug queries expose complete key sizes.

Object.registerClass and counted registered-class lookup now normalize symbol
identity and preserve NUL names in the private registry and graph. A NUL symbol
can be an embedding registry key; this does not assert that SWF ExportAssets
strings can encode embedded NUL. Their file framing remains terminated.

No public layout or graph version changes: ABI 3, graph 8. Register-only
Function2 parameter/arguments behavior, Flash-version rules and the remaining
Object/Array/Number conformance are separate open gates. Stage 1 stays open.

## Русский

Два написания одного UTF-16 имени теперь обозначают одну локальную переменную.
Только последний именованный дубль связан с arguments. Ранее scalar UTF-8 и
WTF-8 пара суррогатов считались разными именами при выборе alias, хотя property
lookup уже считал их одинаковыми. Запись в первый аргумент ошибочно меняла
последний параметр. Тест сначала воспроизвёл ошибку, затем прошёл после исправления.

Добавлены counted factory параметров, named AVM1 call, debug name queries и
lookup зарегистрированного класса. NUL сохраняется в metadata/alias descriptors,
get/set и переносе графа. DefinitionFunction/2 публикует канонические идентичности
имён. Private registerClass registry сохраняет полное имя; это не меняет
terminated формат SWF linkage strings. ABI 3 и graph 8 не меняются. Ограничения
Function2 register-only aliases и остальных стадий не объявляются закрытыми.

## Проверки / Verification

function_parameter_identity checks duplicate scalar/pair names, detached first
arguments entry, retained activation after GC, counted NUL formal alias updates,
function graph restore without standard installation, full-key AVM1 calls,
invalid formal types/counts and canonical decoded script/debug names.
object_register_class covers NUL/pair/scalar symbol identity, GC and independent
registry transfer. avm1_counted_variables verifies full native debug names.
Linux AGG 248/248; Cairo Release 248/248; focused headless ASan/UBSan 4/4;
portable focused 2/2. Full profiles exclude examples. ASan stack instrumentation
and LeakSanitizer remain disabled because /proc-based stack/task discovery is
unavailable. No full leak/stack or other-platform certification is claimed.
