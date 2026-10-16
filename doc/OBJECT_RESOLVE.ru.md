# Загруженный Object.__resolve — v0.450

Если обычное чтение загруженного свойства исчерпало цепь прототипов, FlashVM
ищет raw-собственные данные `__resolve` по исходной цепи. SWF4–6 используют
Flash-регистр, SWF7–8 — точное имя. Примитивные записи пропускаются. Первый
Object останавливает поиск: невызываемый Object даёт undefined; Function или
вызываемый super вызывается с исходным this и одним counted-именем запроса.
Getter самого resolver не вызывается: virtual-свойство даёт сохранённые данные,
а native-метод — своё Function-значение.

Существующее свойство, включая undefined, прекращает поиск до resolver.
Numeric fallback Object/Array использует тот же путь. Проверка собственного
наличия resolver не вызывает. Отсутствующее родительское свойство ищется по
родительской цепи с исходным this; входной super удерживается во время GC и
мутации графа, даже если вызывающий код не закрепил его отдельно.

Специальные числовое/default/строковое преобразования не применяют `__resolve`
к отсутствующим valueOf/toString. Отдельный счётчик ограничивает native/script
resolver 64 вложенными callback и восстанавливается при ошибках. Прежние
пределы прототипов и native-barrier сохраняются. Resolver имеет приоритет над
расширением метатаблицы FlashVM __index; без Object-resolver прежний fallback
остаётся. Host-режим не меняется.

Основание — закреплённые Ruffle
[object.rs search_prototype/find_resolve_method](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object.rs),
[script_object.rs get_local_stored](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs)
и [value.rs special conversions](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/value.rs).
Предел/ошибки, приоритет метатаблицы и C-статусы — явные правила FlashVM, а не
заявление полной эквивалентности плееру.

object_resolve воспроизводит отсутствие hook и проверяет host/SWF4–8, регистр,
собственные/наследуемые/примитивные/невызываемые/virtual/native записи, NUL/Unicode,
индексное чтение, аргумент/this, GC, собственную возвращённую строку, исключения/
ошибки и подавление resolver при own-query/существующем undefined.
jit_object_resolve повторяет GetMember в трёх режимах с настоящими native-входами.
object_resolve_special проверяет исключение преобразований, предел native-рекурсии,
повреждённый цикл прототипов и восстановление. object_resolve_super проверяет
исходный this, самоудаление/отсоединение, удержание входного фасада и исключения.

Direct C/super method resolver описан в v0.451 ниже. Scope-поиск переменных,
перенос/OOM, остальные ресурсы и все стадии остаются открытыми.
Новых проходов исходного SWF-корпуса не заявлено.

Проверено v0.450: полный Linux minimal 890/890; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 6/6. Затем добавленные случаи приоритета метатаблицы
повторены во всех четырёх профилях. Обновлены dependency-index для новой private
раскладки контекста. Применены средства восстановления права запуска;
stack ASan/LeakSanitizer отключены. Все стадии и другие ОС открыты.

## v0.451 — вызов метода, возвращённого resolver

Загруженные direct C missing-method и явные super-методы используют raw-поиск
resolver. Возвращённая Function исполняется с исходным this, снимком исходных
аргументов и без владельца свойства. C для отсутствующего/нефункционального
значения сохраняет UNSUPPORTED; AVM1 parent-call даёт undefined. Настоящие ошибки
resolver/метода, включая UNSUPPORTED, передаются и не маскируются как отсутствие.
Special-преобразования по-прежнему пропускают resolver; host и global native
fallback без resolver сохранены. Lua-style callable Object не является AVM1 Function.

object_resolve_call воспроизвёл отказ direct C-вызова; отдельный контроль
выявил различие super/nonfunction callable. Host/SWF4–8 проверяют обычный/
родительский receiver, C/AVM1 API, Function/scalar/Object/Lua callable/отсутствие/
исключение/UNSUPPORTED, counted-имя, циклический/NUL аргумент, самоудаление,
отсоединение прототипа, изменение исходных аргументов, GC и очистку состояния.
jit_object_resolve_call повторяет CallMethod в трёх режимах с настоящими
native-входами, проверяя отсутствие повторного callback при ошибке. Scope,
перенос/OOM и все стадии остаются открытыми.

Проверено v0.451: полный Linux minimal 892/892; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 5/5. Применены средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Все стадии и другие ОС открыты.

## v0.452 — публикация результата resolver при OOM

Resolver вызывается с частным результатом; наружу он публикуется только при OK.
Отказ копирования возвращённого view временного counted-имени оставляет выход
undefined, а не висячую строку. Исключение хранится в контексте; error-значения
не публикуются как успешный результат. Побочные эффекты регистрации/callback
не откатываются.

object_resolve_allocation воспроизвёл небезопасный выход при отказе new/new[].
Новые контексты перебирают границы до первого завершённого OK/исключения в
SWF4–8: named NUL/indexed, native Function/script body/native method resolver,
циклический capture как payload, очистка корней/рекурсии/script depth, pending
payload, GC/повтор и удержание успешной строки. Это границы C++ allocation, не
все C/OS allocator и не доказательство конкурентности. Scope, перенос графов и
все стадии остаются открытыми.

Проверено v0.452: полный Linux minimal 893/893 и AGG 912/912; целевые
AGG/NO_OS_PORTS на Linux/heap-global ASan+UBSan — по 6/6. Первый полный AGG-прогон
обнаружил один старый jit_array_sort_on_case.o, созданный до private-изменения
контекста. Он пересобран, полный повтор 912 случаев прошёл. Применены средства
восстановления права запуска; stack ASan/LeakSanitizer отключены. Все стадии открыты.
