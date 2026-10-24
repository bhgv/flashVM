# Выборка генераторов Coco/R C/Limbo

[English](README.md) · [Источники](../UPSTREAMS.ru.md)

Источник: https://github.com/bhgv/coco-r-limbo, master. Точный commit и SHA-256 —
в UPSTREAM.json. C-программа генерирует C, а с --limbo — Limbo. Исходник native
Limbo-генератора: upstream/limbo/cocor.b. Оба runtime fragment сохранены: это
не документация, а необходимые входные данные генерации.

```sh
./build-workspace.sh --projects cocor-limbo --tests
```

Собираются C-генератор и cocor.dis, используя локальный source-built bootstrap.
Отдельный CMake-проект собирает только C; для cocor.dis нужно явно передать
LIMBO_EXECUTABLE и LIMBO_MODULE_DIR. Запуск Dis требует совместимой VM по ABI.
Ядро disVM пока не является самостоятельным launcher для cocor.dis.
Draw упомянут только в стандартной сигнатуре Context, реализация GUI не линкуется.
Runtime fragments должны оставаться рядом; upstream-генератор принимает их
каталог опцией -r. Готовые Dis-файлы и внешняя установка Inferno не переносятся.
