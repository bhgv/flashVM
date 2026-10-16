# Function.apply — корни и семантика загруженного SWF

## v0.417: корни подготовки

В host и SWF4–8 упакованный получатель this удерживается во время чтения списка
аргументов. Инициализированный массив значений подключается к корням GC до первого
getter, поэтому уже полученные строки, объекты и циклы переживают сборку внутри
следующего getter. Корни действуют до завершения вызова целевой функции и снимаются
при успехе, ошибке, исключении и отказе памяти. Вспомогательный корень заимствует
существующий C++-массив без выделений и без изменения контекста вызова.

function_apply_gc получает свежую строку с NUL и циклический объект из script
getters. Третий getter действительно запускает GC и проверяет реестры живых
значений до обращения к данным. Проверены null/числовой this, ошибка, исключение
с NUL, отсутствие вызова цели после отказа getter, очистка и повторный вызов для
host/SWF4–8. Тест не проходил на исходной v0.416 до исправления.

Полные правила apply, длина списка, версии преобразований, большие количества
аргументов и платформенная приёмка ещё не подтверждены. Все девять стадий открыты;
цель остаётся SWF ≤8.

Validation v0.417 / Проверка: minimal Release 833/833; AGG Release 2/2 and
heap/global ASan+UBSan 2/2 apply/function tests. Generated native launchers restore
workspace executable modes; stack ASan/LSan disabled. Foreign runtime acceptance
and all nine stages remain open.

## v0.418: собственная длина загруженного списка

В SWF4–8 примитивный второй аргумент даёт пустой список. Для объекта берётся
собственная сохранённая длина, без прототипов и вызова getter: учитываются данные
addProperty и native Array. Имя length сопоставляется без учёта регистра в SWF4–6
и точно в SWF7–8. Значение преобразуется один раз через AVM1 Number/Int32 с
усечением и оборачиванием; undefined/NaN/бесконечность дают ноль. Объект длины
удерживается при самоустранении и GC внутри valueOf. Упаковка this предшествует
преобразованию длины. Host API сохраняет прежнюю политику.

function_apply_loaded проверяет примитивы, дробные/обёрнутые длины, addProperty,
наследование, регистр, Array, собирающий valueOf, исключения и восстановление.
Отрицательная signed длина и длина выше 65536 пока дают UNSUPPORTED до выделений;
это ограничение реализации, а не полное поведение Adobe для таких значений.
Источники сравнения закреплены в [EN](FUNCTION_APPLY.md). Все стадии открыты.

Validation v0.418 / Проверка: minimal Release 834/834; AGG Release 3/3 and
heap/global ASan+UBSan 3/3 apply/function tests. Native launchers restore workspace
executable modes; stack ASan/LSan disabled. Other-platform acceptance remains open.

## v0.419: обычные свойства аргументов

Загруженный apply читает числовые имена через обычный объектный путь для любого
типа источника. Собственные и наследуемые свойства/getters Number/Boolean/String/
Date больше не пропускаются сырым индексным API. Получатель getter — исходный
объект; отсутствующие элементы дают undefined. Уже полученные значения остаются
снимками, даже если следующий getter меняет исходный слот и запускает GC.
Host API сохраняет прежний индексный путь. function_apply_properties проверяет
SWF4–8, четыре native-класса, наследуемый script getter, NUL, мутацию и GC,
пропуски, ошибки/исключения, отсутствие вызова цели после отказа и восстановление.
Полная семантика __resolve/metatable, заимствование apply и большие списки ещё
не подтверждены; все стадии открыты.

Validation v0.419 / Проверка: minimal Release 835/835; AGG Release 4/4 and
heap/global ASan+UBSan 4/4 apply/function tests. Native mode-restoring launchers
used; stack ASan/LSan disabled. Other-platform runtime acceptance remains open.
