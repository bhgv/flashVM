# Date prototype and methods / Прототип и методы Date — v0.198

Загруженный Date.prototype — обычный Object без timestamp. Установка после
загрузки создаёт этот layout; загрузка после установки активирует штатный
invalid host прототип с сохранением identity и свойств. Подготовка Date включена
в общий commit до любых изменений: OOM сохраняет все старые прототипы.

Loaded Date instance methods преобразуют слева направо до семи аргументов,
останавливаясь перед первым explicit undefined. Только затем проверяется native
тип receiver. Другой native тип возвращает undefined; ошибки преобразований
сохраняются даже на неверном receiver. Getters и toString также преобразуют
аргументы. Входные аргументы обрабатываются один раз правилами AVM1, затем
существующий date engine получает числовую проекцию. Host поведение сохранено.

## English

Loaded Date.prototype is ordinary Object without a native timestamp. Installation
after loading creates it directly; loading after installation activates the
unchanged invalid host prototype while retaining identity/properties. Date joins
atomic prototype preparation before any mutation; OOM preserves every layout.

Loaded Date instance methods coerce at most seven operands left to right,
stopping before the first undefined, then check the native receiver. Other native
types return undefined; conversion errors propagate even for wrong receivers.
Getters/toString also coerce supplied arguments. Numeric projections prevent
repeat callbacks before the existing date engine runs. Unloaded host guards,
callback IDs and graph/API formats remain unchanged.

Tests cover SWF4–8/both installation orders, invalid load/identity/NUL marker,
old instances, wrong receivers, seven-operand limit/undefined stop/errors,
versioned setTime strings, GC/channel/GC and restored class construction. The
allocation sweep includes Date alongside Array/Number/Boolean/String.
Reference: pinned Ruffle date.rs create_class/method, commit
2372a4e148a67d68efd796f4477b3cddd4f880cd. Constructor/UTC coercion, Date() global
semantics, calendar fidelity, customized layouts and all stages remain open.
