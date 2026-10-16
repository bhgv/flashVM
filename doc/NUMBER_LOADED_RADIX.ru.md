# Недесятичные значения Number — v0.384

В загруженных SWF4–8 Number.toString с выбранным недесятичным основанием теперь
использует Int32-политику закреплённого AVM1 reference. Конечные значения внутри
signed 32-bit диапазона усекаются к нулю: 42.9 в основании 3 даёт 1120, а -0.25
даёт 0. Дробная часть не выводится. Правила преобразования radix и fallback
сохраняются. Десятичное форматирование и прежние точные host-представления
дробей в степенях двойки/больших целых не меняются.

Диапазон проверяется до усечения. Выход за диапазон, NaN и обе бесконечности
выбирают INT32_MIN, без wrap и насыщения к положительному максимуму. В reference
wrapped negation оставляет INT32_MIN отрицательным; отрицательные остатки дают
байты ниже ASCII zero. Намеренно получаются пунктуация/control bytes, например
-(0000000 в основании 16. Реализация воспроизводит их ограниченными точными
операциями без signed overflow и зависимости от отрицательного деления C++98.
Counted text сохраняет control bytes при принятии строки и GC.

Это политика закреплённого reference для текущего player. Исторические
NaN/Infinity-различия Flash Player, включая прежний ноль/иной мусор, не выбираются
по версии SWF и не подтверждаются этой работой. Версия SWF не является версией
Flash Player. Нового независимого прогона оригинального player нет; fixtures
проверяют явно описанную политику сопоставления исходников.

[GC:] Число захватывается до преобразования radix. Callback может изменить box,
но текущий результат использует захваченное значение. Сохраняются нативный
барьер, исключения и владение выходом. При ошибке и повторе восстанавливаются
корни и глубины. Форматирование не меняет данные Number. Callback IDs, ABI и
wire не меняются; восстановленные методы выбирают loaded/host-политику контекста
получателя.

Проверки: number_loaded_radix — все 34 недесятичных основания SWF4–8, signed
границы, дроби, sentinel, NaN/Infinity, decimal и stored data, настоящий GC
с корнем результата и перенос Number в loaded/host без standard-install.
Allocation sweep — обычное/sentinel/NaN форматирование, GC/retry.
Callbacks — захваченное значение, изменение box, нативный барьер, exception/error.
jit_number_radix — все основания и обычные/sentinel входы в интерпретаторе,
явном и автоматическом JIT. Две группы по 17 программ позволяют проверить
нативное исполнение без штатного fallback после 32 записей кеша.

Сопоставление: [Number to_string](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/number.rs)
и [Int32 clamp](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/clamp.rs)
закреплённого Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd.
Все девять стадий остаются открытыми.

Validation v0.384: all 803 AGG / 803 Cairo / 784 minimal Linux Release checks
have passing evidence after nine ELF execution-permission repairs and targeted
reruns (four AGG, four Cairo, one minimal). Eight restricted heap/global
ASan+UBSan checks pass: four new radix/callback/allocation/native cases plus
legacy radix/conversion, C89 standard Number and native numeric regressions.
Stack ASan and LeakSanitizer remain disabled. The new scoped radix constants
header passes a strict C89 syntax probe. No original external SWF oracle or
foreign-OS acceptance was added. All nine roadmap stages remain open.
