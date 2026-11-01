# Нативный штрих: примеры C и Dis

Для обычного рисования соберите графический runtime, запустите пункт 8 меню сборки
и выберите Files → SVG. До отпускания движения остаются в C. В установленной IDE
не требуется запуск отдельного инструмента или Python-процесса.

C-пример `stamp-check.c` в установленном комплекте — это
[test_ink_primitives.c](../../../../disVM/runtime/test_ink_primitives.c). Он создаёт
неизменяемый `FVM_Path` и собственный RGBA-буфер, вызывает штамп, проверяет пиксели,
показывает замораживание точек и подтверждение замены изображения. Реализация:
[editor_stamp.c](../../../../disVM/runtime/editor_stamp.c),
[интерфейс](../../../../disVM/runtime/editor_stamp.h). Объекты VM не создаются и
не удерживаются.

Limbo-пример `worker-check.b` — это
[native_ink_test.b](../../tests/native_ink_test.b). Он через настоящие каналы
документа ставит в очередь два замороженных штриха, ждёт их через Save, проверяет
целые единицы Undo/Redo и отказ при неверном идентификаторе. Для него нужен
разработческий [test_ink_runner.c](../../../../disVM/runtime/test_ink_runner.c),
который заполняет C-буферы перед запуском настоящего runtime. Этот стенд не является
ещё одной IDE и не устанавливается как плеер.

```sh
cmake --build build/workspace/disvm-editor --target disvm_ink_primitives_test disvm_native_ink_runner
build/workspace/disvm-editor/disvm_ink_primitives_test
ctest --test-dir build/workspace/disvm-editor -R 'native_ink' --output-on-failure
```

`test_native_ink.py` независимо читает полученные SVG в режимах интерпретатора и
JIT. `test_live_ink.c` подаёт 600 настоящих SDL-движений в штатный C-обработчик:
в Dis предлагаются только Begin и End с идентификатором. У этой проверки явно
названа граница готовой сцены; отдельный тест настоящего Limbo-worker не заменён
макетом.

Интерфейс C/Dis: [idehost.m](../../bridge/idehost.m), очередь и отдельный процесс:
[ideinkworker.b](../../bridge/ideinkworker.b). Полный
[контракт](../../../NATIVE-INK.ru.md) описывает время жизни, пределы, публикацию,
приближённый предварительный штамп и согласованные сигнатуры runtime.
