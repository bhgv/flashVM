# Исторический concat / Loaded-SWF concat — v0.132

В загруженном SWF concat использует собственную data-длину источников и signed
Int32 с оборачиванием. Читаются только собственные хранимые элементы; accessor
и наследуемые индексы не вызываются. Пустые элементы становятся собственными
undefined. Настоящие Array-аргументы разворачиваются; обычный Object-аргумент
сохраняется одним элементом. Заимствованный метод разворачивает обычный receiver
по его длине. Длина каждого источника захватывается при переходе к нему.
Незагруженный C-режим сохраняет property-aware/sparse копирование.

Каждый элемент расходует общий бюджет AVM1. Огромные положительные signed-длины
останавливаются при его исчерпании; отрицательная wrapped-длина даёт ноль
элементов. Произвольный предел 65 536 для concat не возвращается.
array_legacy_concat проверяет SWF6..8, настоящие/заимствованные источники,
явные undefined, accessor/наследование, виртуальную длину, обычный Object
в аргументах, signed-длины и восстановление после ошибки бюджета.
array_concat_allocation дополнен перебором отказов выделения для загруженного
режима: источник неизменен, корни/глубина восстановлены, GC после каждой попытки,
успешный результат материализует собственные пустые индексы.

Правило сверено с [pinned concat](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs).
Полная семантика Array/прототипов/версий функций и все стадии остаются открытыми.

## English

Loaded concat reads own stored elements and signed wrapping own-data lengths,
ignoring accessors and inherited indices. Missing elements become own undefined
slots. Array arguments spread; plain Object arguments remain singletons. An
ordinary borrowed receiver spreads by its length. Each source length is captured
at its turn. Unloaded host mode retains property-aware sparse copying.

Each element charges the common AVM1 budget. Huge positive signed lengths fail
when it expires; negative wrapped lengths contribute zero items. There is no
arbitrary 65,536 concat cap. array_legacy_concat covers SWF6..8 real/borrowed
sources, missing slots, descriptors/inheritance, virtual length, singleton
Objects, wrapping lengths and budget restoration. array_concat_allocation
exhausts allocation boundaries in loaded mode, checking source preservation,
root/depth restoration and GC after each retry, through a fully materialized
successful result. Full Array/prototype/function-version semantics and every
roadmap stage remain open.
