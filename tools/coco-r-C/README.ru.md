# Выборка Coco/R C для сборки

[English](README.md) · [Источники и обновление](../UPSTREAMS.ru.md)

Источник: https://github.com/bhgv/coco-r-C, master. Commit и контрольные суммы —
в UPSTREAM.json. Перенесены C-генератор, его .atg/frame, используемые C-шаблоны и
исходная атрибуция. C++, примеры, бинарники и коллекции документации исключены.
Импортированный код не переформатируется и не исправляется скрыто.

```sh
./build-workspace.sh --projects cocor --tests
```

Для независимой сборки: `cmake -S tools/coco-r-C -B build/cocor`, затем
`cmake --build build/cocor`. Нужны C-компилятор и CMake; Python 3 — только тестам.
Для отделённых компонентов задайте COCOR_ROOT=tools/coco-r-C/upstream
(абсолютный путь) и COCOR_EXECUTABLE на собранный cocor.
