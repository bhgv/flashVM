# Компилятор FIR и API C99

[English](README.md). [Сборка](../README.ru.md).

```sh
build/fir1/fir1 check INPUT.fir
build/fir1/fir1 print INPUT.fir OUTPUT.fir
build/fir1/fir1 opt INPUT.fir OUTPUT.fir
build/fir1/fir1 swf INPUT.fir OUTPUT.swf
build/fir1/fir1 actions INPUT.fir OUTPUT.actions
build/fir1/fir1_source as2 --input INPUT.as OUTPUT.fir
```

`check` разбирает и проверяет без вывода; `print` канонизирует; `opt` добавляет
peephole; `swf` пишет FWS8; `actions` — сырые AVM1 records с ActionEnd.
`swf` не оптимизирует неявно: вход выбирается явно. Язык адаптера: `as1`, `as2`,
`as3` или `limbo`; после `--input` идёт входной путь, последний аргумент — выход.
См. [покрытие](../doc/COVERAGE.ru.md). Неявного поиска include/модулей нет.
Коды fir1: 0 — успех, 1 — неверный вход/семантика, 2 — CLI/I/O/backend.
У fir1_source: 1 — разбор/преобразование/вывод, 2 — CLI или подготовка.

## Исходники

- [fir1.atg](fir1.atg): scanner/parser FIR и семантические действия.
- [src1.atg](src1.atg): учебные source-адаптеры, приоритеты, генерация FIR.
- [source.c](source.c): CLI адаптеров с отдельными globals.
- [builder.c](../src/builder.c): построение IR с проверкой границ, версии, UTF-8.
- [ir.c](../src/ir.c): проверка и принтер.
- [swf.c](swf.c): AVM1, relocation и контейнер SWF.
- [main.c](../src/main.c): команды FIR, временный вывод перед публикацией файла.

## Внутренний API C99

Объявления: [fir.h](../include/fir.h). Возврат 1 — успех, 0 — ошибка.

| Функция | Параметры и владение |
| --- | --- |
| `fir_parse(input, module, error, capacity)` | Читает binary FILE с текущей позиции; module принадлежит вызывающему; error — изменяемый буфер, capacity — его размер в байтах. Из-за globals: один разбор на процесс, не thread-safe embedding API. |
| `fir_verify(module, error, capacity)` | Проверяет разобранный модуль вызывающего; диагностический буфер как выше. |
| `fir_print(output, module)` | Пишет канонический текст в FILE вызывающего, не закрывает поток. |
| `fir_optimise(module, error, capacity)` | Меняет проверенный модуль и проверяет повторно; сохраняет эффекты. |
| `fir_swf(output, module, raw, error, capacity)` | Только проверенный модуль; raw=0 — FWS8, иначе actions; поток не закрывает. |

Диагностика содержит строки FIR; синтаксические ошибки source-адаптера — также
колонку/байтовое смещение. Полного debugger/source map пока нет. Модуль крупный:
выделяйте его в куче, как CLI. ABI структур внутренний.

Backend использует зарезервированные movie-local имена `__fir1_value_N`.
Фильм trace-only, с нулевым размером сцены; это не пример визуального фронтенда.
Сложение/вычитание i32 заворачиваются через BitOr 0, умножение использует 16-битные
частичные произведения, сохраняя младшие биты. Смещения переходов проверяются
после генерации. Каналы требуют установки существующего расширения хостом.
Оба парсера генерирует один явно заданный внешний Coco/R, их globals никогда
не линкуются в одном executable. Сгенерированные файлы сборки не редактируются.

Локальная CMake-сборка: можно конфигурировать эту папку с теми же параметрами
COCOR_ROOT и COCOR_EXECUTABLE; executable находятся в `v1/` каталога сборки.

Справка без файла: `fir1 --help`, `fir1_source --help`; версии: `--version`.
На Linux/Termux `TMPDIR` задаёт каталог безопасных временных файлов.
Числовая семантика и окружение SSA: [lower.c](lower.c), интерфейс: [lower.h](lower.h).
[Полный цикл](../doc/PIPELINE.ru.md).

[C API и встраивание](API.ru.md).
