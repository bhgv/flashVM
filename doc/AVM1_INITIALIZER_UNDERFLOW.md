# Пустые операнды инициализаторов / Missing initializer operands — v0.128

В загруженном SWF InitArray и InitObject получают undefined при попытке снять
операнд с пустого стека. Отсутствующий счётчик преобразуется в NaN и даёт пустой
результат. Нехватка элементов создаёт собственные индексы со значением undefined;
нехватка имени/значения пары обрабатывается независимо. Имена undefined используют
версию SWF: пустая строка до SWF7, «undefined» начиная со SWF7. Снятие ограничено
фактически существующими операндами, без арифметического выхода за начало стека.
Внешний C-режим без загруженного SWF сохраняет строгие ошибки.

avm1_initializer_underflow проверяет отсутствие счётчика, элементов и пар,
частичный массив, полную пару с недостающей второй, нечётную пару, собственность
пустых индексов и имена SWF6..8; отдельно проверяет прежние ошибки C-режима.
Правила сопоставлены с pinned Ruffle initializer и
[pop implementation](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/runtime.rs).
Предел материализации 65 536, полная семантика конструкторов, дескрипторов,
происхождение версии функций/перенесённых клипов и все стадии остаются открытыми.

## English

Loaded SWF InitArray/InitObject pop undefined from an empty operand stack.
A missing count converts to NaN and produces an empty result. Missing Array
items create own undefined slots. Missing Object pair names and values default
independently, using the versioned undefined-name spelling. Only existing
operands are removed; stack indexing cannot underflow. Unloaded C execution
retains strict errors.

The named test covers missing counts/items/pairs, partial arrays, one complete
pair followed by a missing pair, odd pairs, own undefined slots, SWF6..8 names
and unloaded errors. Rules follow the pinned primary source. The 65,536
materialization cap, full constructor/descriptor semantics, version provenance
and every roadmap stage remain open.
