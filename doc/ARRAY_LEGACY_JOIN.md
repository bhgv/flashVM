# Исторический Array.join / Historical Array.join — v0.124

Для контекста с загруженным SWF join/toString массива читает собственные
сохранённые данные элементов, без обхода прототипа и вызова getter. Длина
настоящего Array берётся из нативного поля; у заимствованного получателя — из
собственных данных length, без виртуального getter. Преобразование длины идёт
по правилам AVM1 и signed ToInt32; неположительная длина даёт пустой результат.

Элементы, включая дырки/undefined/null, преобразуются в строку по версии:
undefined до SWF 7 — пустая строка, затем "undefined"; null — "null";
boolean до SWF 5 — "1"/"0". Переданный undefined-разделитель преобразуется
так же, а пропущенный означает запятую. C API без загруженного SWF сохраняет
свой режим с наследуемыми getter и пустыми частями null/undefined.

Существующий помощник преобразования строк AVM1 вынесен из интерпретатора
для общего применения; addProperty теперь использует его в загруженных SWF.
Публичное `fvm_value_to_string` остаётся версионно независимым C-преобразованием.
Общий бюджет обхода/байтов/callbacks и очистка рекурсивного guard сохранены.
Проверки array_legacy_join загружают настоящие минимальные SWF 4..8 и проверяют
собственные данные, невызванные getter, разделитель, signed length и бюджет.

[Primary join/element rules](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs)
и [own-data lookup](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs)
сверены; scalar string rules взяты из уже проверенного помощника AVM1.
Исходный array_prototyping SWF 8 завершился с точными 121 байтами через настоящий
root timeline и явное подтверждение quit. Теперь прошли тринадцать внешних
проб с документированными режимами; прочие Array методы, native-extension
представления/mapped arguments, рекурсия, происхождение версий функций и все
стадии остаются открытыми.

## English

Loaded-SWF join reads own stored element data, ignores virtual getters/prototypes,
and uses AVM1 signed wrapping own length. Array's native length is used directly;
nonpositive lengths yield an empty result after separator conversion. Every part
uses versioned string conversion, including holes/undefined/null and early-SWF
booleans. Explicit undefined separators are coerced; omission still means comma.
Unloaded C API contexts retain their existing property-aware behavior.

The existing AVM1 string helper is shared without changing interpreter rules or
public host conversion. AddProperty names use it in loaded contexts too. Native
work/callback budgets and recursion cleanup remain. Minimal original-format
SWF 4..8 test contexts cover the listed rules and bounded large signed lengths.
The pinned primary rules above were checked, and original array_prototyping SWF8
completes with 121 exact bytes under real-root/explicit-quit policies. Thirteen
whole probes now match; other Array methods, extension/arguments representations,
recursion, function-version provenance and every roadmap stage remain open.
