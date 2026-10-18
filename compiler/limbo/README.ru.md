# LIMBO

Версия 0.1.0. [English](README.md).

Независимый проект компилятора на C99. Сейчас реализован синтаксический
фронтенд: [грамматика, сборка и ограничения](frontend/README.ru.md).
Семантический анализ, AST, генерация байткода и SWF пока не реализованы.

Сборка из этой папки с теми же явными путями к генератору:

```sh
cmake -S . -B build \
  -DCOCOR_ROOT=/path/to/coco-r-C \
  -DCOCOR_EXECUTABLE=/path/to/coco-r-C/cocor
cmake --build build
ctest --test-dir build --output-on-failure
```
