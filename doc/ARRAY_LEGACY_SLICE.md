# Исторический Array.slice / Loaded-SWF slice — v0.131

В загруженном SWF slice захватывает собственную data-длину до преобразования
границ и использует signed Int32 с оборачиванием. Start/end также преобразуются
через AVM1 в signed Int32; отсутствующий/undefined end означает захваченную
длину. Копируются собственные хранимые элементы: accessor и наследуемые индексы
не читаются. Пустой исходный элемент становится собственным undefined в
результате, а не дыркой. Преобразование границ может менять исходные данные:
последующее чтение видит эти изменения, но сохраняет захваченный диапазон.

Для контекста без загруженного SWF сохраняется прежний property-aware/sparse
режим с наследованием/getter и дырками. Общие own-data помощники используются
join и slice. Каждый материализуемый элемент расходует общий бюджет AVM1;
слишком большой результат завершается ошибкой бюджета без искусственного
предела 65 536 для slice. Нулевая настройка бюджета остаётся неограниченной.

array_legacy_slice проверяет настоящие и заимствованные массивы, own undefined,
accessor/наследуемые индексы и виртуальную длину без вызовов, оборачивание границ,
отрицательный start, undefined end, отрицательную wrapped-длину, исчерпание
бюджета и изменение данных/длины при преобразовании start. Проверены SWF6..8.
Правила сопоставлены с [pinned slice](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs)
и get_element/length в object/script_object.rs. Оригинальный array_slice имеет
заголовок SWF15 и остаётся вне поддерживаемого диапазона; его прохождение не
заявляется. Полные Array/прототипы/версии функций и все стадии открыты.

## English

Loaded slice captures own data length before endpoint conversion and uses
signed wrapping Int32 length/endpoints. Missing or undefined end keeps captured
length. Only own stored elements are copied; accessors and inherited slots are
ignored. Missing elements become own undefined result slots. Endpoint coercion
can mutate source data; later reads observe changes within the captured range.

Unloaded host mode retains property-aware sparse copying, inheritance/getters
and holes. Join/slice share own-data helpers. Materialization charges the shared
AVM1 work budget; slice has no arbitrary 65,536 cap. Huge copies fail when the
configured budget expires, while zero remains unlimited.

array_legacy_slice covers real/borrowed sources, own undefined slots, ignored
accessors/prototype indices/virtual length, wrapping endpoints, negative start,
undefined end, negative wrapped length, budget exhaustion and coercion mutation
in SWF6..8. Rules follow pinned primary source. Original array_slice has a SWF15
header and remains outside the supported range; no oracle pass is claimed.
Full Array/prototype/function-version semantics and all stages remain open.
