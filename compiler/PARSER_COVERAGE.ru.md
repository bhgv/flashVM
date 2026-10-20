# Парсеры: покрытие и границы серии 0.5.1

Эта серия существенно расширяет все пять синтаксических интерфейсов, но не
объявляет их полными компиляторами или полностью соответствующими языкам.
Парсеры не строят AST исходных языков, не разрешают имена и не проверяют типы.
Ниже явно отделены распознавание синтаксиса, исполняемый FIR и оставшаяся работа.

| Компонент | Добавлено и проверено | Что ещё нужно для полного фронтенда |
| --- | --- | --- |
| AS1/AS2 | ASI; ограничения перевода строки; and/or/not; on/onClipEvent; include/initclip-обёртки | Escape в идентификаторах, Unicode-категории и точный профиль Flash, AST/контекст/левая часть; раскрытие include отдельно |
| AS3 | ASI, metadata, вложенные Vector, regexp lexical goal, XML/XMLList/E4X и выражения внутри XML | Полная regexp/XML-валидация, Unicode/escape, оставшиеся директивы/лексические случаи и AST/контекст |
| Limbo | Полиморфизм/constraints, exceptions, fixed, **, срезы, типовые аргументы | Именованные casts, полное различение type/value по символам, оставшиеся неоднозначности qualifier и AST/контекст |
| FIR | Несколько сигнатур, параметры блоков/рёбер, invoke, декларации/отладочные записи и составные типы | Построение и верификация расширенного IR, opcode-каталог/эффекты, полноценное понижение новых конструкций |

AS1 имеет версию 0.3.0, AS2 — 0.3.1, AS3/Limbo — 0.4.0, инструменты FIR — 0.5.0,
отдельный FIR syntax API — 0.1.0. Формат текста FIR остаётся 1.0; ABI и версии
компонентов не связаны с версией проигрывателя. Файлы ATG — авторитетные
источники; C99-код генерируется внешним bhgv/coco-r-C в каталогах сборки.
C-блоки в грамматиках заканчивают строку и выровнены форматтером; точки
производств вынесены на отдельные строки с пустым разделителем.

## Интерфейсы из C

У AS/Limbo используются *_check(bytes,size,diagnostic) и *_read(FILE*,diagnostic).
У нового FIR API — firsyn_check/firsyn_read из firsyn.h. Все возвращают 1/0,
сохраняют владение входом у хоста, не печатают и не завершают процесс. Диагностика
принадлежит хосту и может быть NULL. Глобальные имена разделены префиксами.
Один парсер требует внешней сериализации; последовательное восстановление после
ошибки поддержано. Это не обещание reentrant-вызовов или thread safety одного API.

```sh
cmake -S compiler -B build-five \
  -DCOCOR_ROOT=/host/coco-r-C \
  -DCOCOR_EXECUTABLE=/host/coco-r-C/sources/cocor \
  -DFIR_SYNTAX_ROOT=/absolute/path/to/fir/v1/frontend
cmake --build build-five
ctest --test-dir build-five --output-on-failure
```

FIR_SYNTAX_ROOT — явный необязательный SDK/source-root. Без него compiler
по-прежнему строит четыре независимых языка и не ищет sibling FIR. Дополнительный
тест compilers_five_embedding связывает все пять библиотек в один C99-процесс,
чередует ошибки/успешные разборы и не использует CLI/подпроцессы.

## Проверка и ограничения доказательств

База серии: GitHub master 54f324c5b37c7cbbe65c58f3fb774cf01099b776.
На Linux x86-64 прошли 19 CTest-групп компиляторов и 6 FIR, включая реальные
C API и запрет обхода backend через syntax-режим. Отдельно проверен хост всех
пяти API. Переименованные отдельные копии compiler и fir/v1 также собраны и
прошли свои тесты. Повторные прогоны в копиях не считаются новыми сценариями.
Код внешнего Coco/R не изменялся и не включён в репозиторий.

Все пять CLI и fir1 кросс-собраны для ARM64 Linux/musl, RISC-V64 Linux/musl и
x86-64 Windows/GNU; проверены ELF/PE и коды архитектур. Использован адаптированный
для этой среды Zig 0.13, а не сертифицированный неизменённый SDK. Иностранные
бинарники не запускались; новых запусков macOS/Android и sanitizer-прогонов нет.
Восемь файлов upstream Inferno (sys/draw/math/string/bufio.m, echo/cat/ls.b)
приняты синтаксически; это не типизация, раскрытие include или исполнение.

Старые отчёты [VALIDATION.md](VALIDATION.md) относятся к своей версии;
их числа, sanitizer- и VM-результаты не переносятся на эту серию.

## Sources / Исходники

| Parser | Grammar | C interface | Examples |
| --- | --- | --- | --- |
| AS1 | [as1.atg](as1/frontend/as1.atg) | [frontend.h](as1/frontend/frontend.h) | [README](as1/examples/README.md) |
| AS2 | [as2.atg](as2/frontend/as2.atg) | [frontend.h](as2/frontend/frontend.h) | [README](as2/examples/README.md) |
| AS3 | [as3.atg](as3/frontend/as3.atg) | [frontend.h](as3/frontend/frontend.h) | [README](as3/examples/README.md) |
| Limbo | [limbo.atg](limbo/frontend/limbo.atg) | [frontend.h](limbo/frontend/frontend.h) | [README](limbo/examples/README.md) |
| FIR | [firsyn.atg](../fir/v1/frontend/firsyn.atg) | [firsyn.h](../fir/v1/frontend/firsyn.h) | [README](../fir/v1/examples/structural/README.md) |

[Five-API host](tests/embed_five.c) · [Build guide](../BUILDING.md) ·
[FIR executable format](../fir/v1/doc/FORMAT.md) ·
[Inferno reference grammar](https://github.com/inferno-os/inferno-os/blob/master/limbo/limbo.y)
