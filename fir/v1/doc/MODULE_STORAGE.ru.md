# Литеральные имена AVM1 storage

FIR implementation 0.5.2 исправляет допустимые имена модулей с точкой. Прежнее имя SSA-переменной содержало module text буквально. Поэтому модуль ide.as1 создавал имя, трактуемое как property path, и выводил undefined вместо 42.

Backend теперь использует резервный префикс __fir1$, две lowercase hex-цифры на каждый исходный байт имени модуля и суффикс индекса значения. Кодирование инъективно для точек, подчёркиваний и регистра; префикс не пересекается с прежними __fir1_ именами. SWO1 module metadata сохраняется, поэтому linker duplicate-module checks используют исходное имя. Imports, exports и cross-module function ABI не добавляются. Длинные имена увеличивают actions; прежние проверяемые object limits и signed branch limits сохраняются. Private storage не является публичным API переменных.

Регрессия проверяет семь имён: варианты dot/underscore и регистра, имена, похожие на legacy prefix, и максимальное имя source module. Object inspection доказывает различие storage names и сохранение metadata. При FIR_SWL и FIR_FLASHVM_RUNNER выполняются отдельные модули и общий связанный SWF с ожидаемым trace 42 от каждого.

[Backend implementation](../compiler/object.c) · [Regression](../tests/module_names.py).
