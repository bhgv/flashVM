# Separately compiled native module / Отдельно компилируемый C-модуль

This example extends the existing [C extension tutorials](../../extending/README.md)
and [object method example](../object_methods/README.md). It uses the public
rooted callback API; it does not introduce another plugin ABI or a shared-library
loader. The host publishes one namespace object named `NativeDemo`.

Пример продолжает существующие руководства по расширению FlashVM. Модуль
компилируется отдельно от хоста, но связывается с ним обычным линкером.
Его функции доступны через объект `NativeDemo`. Автоматическая загрузка
`.so`/`.dll` этим примером не реализуется.

| File / Файл | Responsibility / Назначение |
| --- | --- |
| [module.h](module.h) | Factory contract and host-owned state / Контракт фабрики и состояние хоста |
| [module.c](module.c) | Argument validation, methods, setter and GC captures / Проверка аргументов, методы, setter и захваты GC |
| [main.c](main.c) | Publication, calls, collection, negative checks and teardown / Публикация, вызовы, сборка мусора и освобождение |
| [main.as](main.as) | Two real property writes from compiled ActionScript / Две записи свойства из скомпилированного AS |

`add(a, b)` accepts bounded signed integers and returns their sum. `seed()`
reads a managed object retained by callback captures. Assigning an integer to
`accumulate` updates the C state immediately when the action executes; it does
not schedule work for a later timeline frame.

`add(a, b)` проверяет целые аргументы и возвращает сумму. `seed()` читает
управляемый объект, удерживаемый захватами callback. Setter `accumulate`
меняет состояние C при исполнении операции, без ожидания следующего кадра.
Текущий профиль компилятора примера использует присваивания свойству;
нативные методы отдельно проверяются настоящими вызовами API из C.

## Build and run / Сборка и запуск

From the workspace root / Из корня общего репозитория:

```sh
cmake -S disVM/runtime -B build/native-module -DDVM_RUNTIME_EDITOR=ON -DDVM_RUNTIME_TESTS=ON
cmake --build build/native-module --target disvm_flashvm_native_module_example disvm_flashvm_native_module_swf
ctest --test-dir build/native-module -R '^disvm_flashvm_native_module_example$' --output-on-failure
```

The test passes the linked SWF to the executable. The executable also works
without a SWF argument to exercise the C-side calls alone.
Тест передаёт исполняемому примеру собранный SWF. Без аргумента SWF программа
проверяет только вызовы со стороны C.

## Lifetime / Время жизни

Create and temporarily pin the seed object before allocating the namespace.
Immediately pin or publish the returned namespace before another allocating
VM operation. Callback captures trace the seed; the `void *` host state itself
is not a GC root. Keep that state and all callback machine code alive until
the context is destroyed. Removing the global property alone does not permit
unloading code: a script may still retain another reference.

Сначала создаётся и временно закрепляется объект seed. Полученный namespace
нужно сразу закрепить либо опубликовать до следующей операции с выделением
памяти VM. Захваты callback удерживают seed; указатель на состояние C сам по
себе корнем GC не является. Состояние и код callback живут до уничтожения
контекста. Удаления глобального свойства недостаточно для выгрузки кода:
скрипт может сохранить другую ссылку.

All VM operations here run on the context's owning thread. A native module
does not make a context concurrently callable from arbitrary threads.
Все операции VM выполняются потоком владельца контекста; подключение C-модуля
само по себе не разрешает конкурентный доступ к контексту.
