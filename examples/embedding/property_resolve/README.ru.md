# Разрешение отсутствующего метода в исходную C-функцию

[English](README.md) · [Исходник](main.c)

Пример на C89 загружает минимальный SWF6, устанавливает `Object.__resolve`
и вызывает отсутствующий метод `sum`. Обработчик возвращает исходную нативную
C-функцию; VM вызывает её с исходным получателем и аргументами. Свойство `sum`
не сохраняется. При успехе программа печатает `sum = 42`.

Из корня репозитория, с CMake и компиляторами C89/C++98:

```sh
cmake -S . -B build-resolve -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DCMAKE_BUILD_TYPE=Release
cmake --build build-resolve --config Release --target flashvm_examples_embedding_property_resolve_main
./build-resolve/flashvm_examples_embedding_property_resolve_main
```

Генератор Windows может поместить `.exe` в `Release/`. Этот запуск на Linux
не подтверждает выполнение на других ОС.

Получатель закреплён на время построения. Временные закрепления функций
защищают публикацию; управляемый захват обработчика удерживает исходную
C-функцию. Один userdata не является корнем GC. Для нативных callbacks
сохраняется обычный барьер сборки. Уничтожение контекста освобождает весь граф,
включая закреплённые объекты.

Пример работает локально. Исходные C-функции через один порт брокера между
процессами, двумя VM в разных потоках и компьютерами по LAN показаны в
[function_rpc](../function_rpc/README.ru.md). Семантика обработчика и ограничения:
[OBJECT_RESOLVE](../../../doc/OBJECT_RESOLVE.ru.md).
