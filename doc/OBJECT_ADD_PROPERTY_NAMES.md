# Имена addProperty / addProperty names — v0.123

Переданный первый аргумент `Object.addProperty` преобразуется в строку до
проверки getter/setter. Число 2 означает имя "2", объект может дать имя через
`toString`, встроенный NUL сохраняется. Преобразование выполняется и при
недостатке следующих аргументов; его ошибка передаётся без установки свойства.
При отсутствии самого имени метод возвращает false.

Дескриптор с null setter отмечается read-only. Замена на дескриптор с setter
снимает read-only, сохраняя остальные применимые флаги. Выделение флагов и
дескриптора завершается до удаления старого значения; при ошибке нового
дескриптора добавленный пустой флаг откатывается. Проверяется сохранение значения
и флагов при каждом отказе выделения памяти до успешного вызова.

Правило преобразования сопоставлено с
[pinned primary source](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/object.rs).
Тесты: object_add_property_names, object_add_property_allocation,
standard_function, array_generic. Разрешение невызываемых объектов getter/setter,
замена защищённых свойств, accessor числовых полей/length настоящего Array и
историческая доступность метода пока не завершены. Внешняя проба
array_prototyping также требует исправления исторического Array.join; её
частичный вывод не является подтверждением полного прохождения. Стадии открыты.

## English

AddProperty coerces a supplied name before getter/setter validation, including
numeric/object names and counted NULs. Coercion still runs when later arguments
are missing; errors propagate without installing a descriptor. A missing name
returns false. Null setters set read-only, and writable replacement clears that
bit while retaining the remaining applicable flags.

Flag/descriptor allocation precedes stored-value removal; failed descriptor
allocation rolls back a newly inserted empty flag. Exhaustive allocation tests
verify the previous value/flags through success. The named tests cover conversion
order, numeric/counted names, errors, flags and allocation boundaries. Rules
were checked against the linked primary source. Non-callable descriptor objects,
protected replacement, real Array numeric/length accessors and version exposure
remain open. The original array_prototyping probe also needs historic join fixes;
partial output is not whole-probe acceptance. All roadmap stages remain open.
