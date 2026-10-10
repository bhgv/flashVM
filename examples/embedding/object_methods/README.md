# Native object methods / Нативные методы объектов

**EN:** This C89 example registers `add`, creates an object with `base = 35`,
then calls `fvm_object_call(context, object, "add", 1, ...)`. The callback
receives that object as `self`, reads its `base` property and returns 42.

**RU:** Пример на C89 регистрирует `add`, создаёт объект со свойством
`base = 35` и вызывает `fvm_object_call`. Обработчик получает объект в
`self`, читает `base` и возвращает 42.

Build / Сборка: `cmake -S . -B build && cmake --build build`.
Run / Запуск: `build/flashvm_example_object_methods`.

**Limitations / Ограничения:** Methods are currently resolved by name in the
native callback registry, not through function-valued object properties.
The `.as` file illustrates intended syntax, not a currently working
ActionScript integration test. `CallMethod` AVM1 supports this native bridge.

Методы пока ищутся по имени в реестре C-функций, а не как функции-свойства
объекта. Файл `.as` иллюстрирует планируемый синтаксис, но ещё не является
проходящим интеграционным тестом ActionScript.
