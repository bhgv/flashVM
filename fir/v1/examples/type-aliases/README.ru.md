# Исполняемые псевдонимы типов Limbo

[English](README.md) · [Контракт](../../doc/SOURCE_ALIASES.ru.md) · [Скалярный пример](main.b) · [Гибридная таблица](table.b)

Нужны согласованные FIR 0.49 и DisVM 0.32. Команды выполняются из корня репозитория после сборки инструментов. Каждая непустая строка примеров снабжена пояснением. main.b показывает связывание в области объявления: Count остаётся int, хотя модуль позднее объявляет Base как real.

```sh
export TMPDIR="$PWD/build/tmp"
mkdir -p "$TMPDIR"
build/fir/fir1_source limbo --input fir/v1/examples/type-aliases/main.b build/aliases.fir -g build/aliases.debug.json
build/fir/fir1 check build/aliases.fir
build/fir/fir1 opt build/aliases.fir build/aliases.opt.fir
build/fir/fir1 dis build/aliases.opt.fir build/aliases.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 2 build/aliases.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 2 build/aliases.dis
build/fir/fir1 object build/aliases.opt.fir build/aliases.swo
build/swf-linker/swl link build/aliases.swf build/aliases.swo
```

Оба запуска Dis выдают DATA 0 42 и DATA 8 1. Второе слово отмечает выполнение trace. SWF выводит число через trace в хосте, показывающем этот вывод; открытое окно само по себе не проверяет результат. Здесь действует цепочка исходник → FIR → SWO → линкер → SWF. Команда dis использует существующий прямой бэкенд.

Для table.b повторите первые шесть команд с отдельным именем выходных файлов. Проверяются псевдонимы big, string и гибридной таблицы; результат также 42. Табличный бэкенд предназначен для Dis, а не числового SWF-профиля. Учебная обёртка value() не является полным ABI Inferno init.

Псевдонимы разрешаются до генерации FIR и не меняют представление данных во время исполнения. Отклоняются неизвестные, опережающие и самоссылочные имена, дубликаты, более 128 псевдонимов и локальные переменные, затеняющие псевдоним. В модуле псевдонимы идут до value. Произвольные тела функций, импорты и приведения по имени псевдонима пока вне этого адаптера. У компилятора интерфейсов есть отдельный более широкий механизм связывания. Отладочные позиции сохраняют байтовые смещения UTF-8 исходника.
