# Синтаксический фронтенд LIMBO

Версия 0.5.0. [English](README.md).

Читаемый файл [limbo.atg](limbo.atg) — исходник сканера и парсера.
Используется C-бэкенд [bhgv/coco-r-C](https://github.com/bhgv/coco-r-C).
Встроенные предикаты, драйвер и генерируемый код собираются как C99 без расширений.
Это распознаватель синтаксиса. AST, таблица символов, проверка типов, генератор
байткода и запись SWF пока отсутствуют.

## Сборка и отделение

Нужны уже собранный внешний C-генератор и соответствующая ему папка `frames/`.
Его исходники, шаблоны и документация в репозиторий не копируются.
Команды из этой папки фронтенда:

```sh
cmake -S . -B build \
  -DCOCOR_ROOT=/absolute/path/to/coco-r-C \
  -DCOCOR_EXECUTABLE=/absolute/path/to/coco-r-C/cocor
cmake --build build
ctest --test-dir build --output-on-failure
./build/limbo_parse /path/to/source
```

Готовой программе генератор не нужен. Python 3 используется только тестами;
для сборки без него задайте `-DBUILD_TESTING=OFF`. Нужны CMake 3.16+ и C99-компилятор.
При многоконфигурационной сборке Windows программа находится в папке выбранной
конфигурации, например `build/Debug`; запуск на Windows здесь не проверялся.
Необязательная сборка всех фронтендов: [../../CMakeLists.txt](../../CMakeLists.txt).
Те же параметры работают при сборке из родительского проекта языка.

Папку можно копировать и переименовывать: сборка не требует FlashVM, линковщика,
соседних фронтендов и файлов общего корня. В отдельном дистрибутиве сохранить
общую корневую MIT LICENSE и уведомления авторов шаблонов в генерируемых файлах.
C-код и заголовки создаются только в `build/generated/` и не коммитятся.
У каждого фронтенда есть программа и C-библиотека с префиксами символов.
Повторные вызовы сбрасывают состояние; вызовы одного парсера сериализуются.
C API описан в README проекта языка.

## Драйвер и диагностика

[main.c](main.c) открывает один файл, проверяет UTF-8 и перематывает тот же
дескриптор перед разбором. Отклоняются некорректные, усечённые и избыточные
последовательности, суррогаты, значения выше U+10FFFF и встроенный NUL.
Начальный UTF-8 BOM поддерживается выбранным генератором. Нужен файл с возможностью
позиционирования. Позиции и столбцы считаются **в байтах**. Категории Unicode в
именах и семантика escape-последовательностей пока отдельно не проверяются.
Строки сохраняют исходные байты.

Публичный интерфейс — [frontend.h](frontend.h), [api.c](api.c).

`limbo_check(bytes, size, diagnostic)` / `limbo_read(FILE*, diagnostic)`
возвращают 1 при принятии, 0 при отказе; diagnostic может быть NULL.
Память и потоки принадлежат вызывающему. Вызовы одного парсера сериализовать;
после отказа разрешён новый вызов. Принимается не более 2 MiB UTF-8.
Коды -3/-4 обозначают ограничения глубины/работы; это не ошибка языка.
При ресурсном отказе координаты могут быть нулевыми. Проверка синтаксиса
не заменяет анализ типов, символов и допустимости return/break.

## Покрытие и границы синтаксиса

Распознаются implement и интерфейсные единицы, include, module, ADT и варианты
pick, определения функций, группы параметров, базовые/составные типы, константы,
импорты, кортежи, массивы/срезы, создание/отправка/приём каналов, spawn, case/alt/pick,
циклы, вызовы и уровни приоритетов выражений. AheadArrow останавливает тело ветви
перед следующим внешним условием без потребления входа; учитывает кавычки,
комментарии и вложенность круглых/квадратных скобок. Пустые тела ветвей допустимы.

Это начальный профиль классического языка, а не полный компилятор Inferno.
Полиморфные/exception-конструкции добавлены в 0.4.2; проверки типов/контекста
пока отсутствуют. Предпросмотр условий не заменяет семантическое разрешение:
сложным условиям с фигурными скобками требуется доработка. include распознаётся,
но файлы не открываются и не разворачиваются. Проверки числовых диапазонов,
оснований и категорий Unicode в именах относятся к дальнейшим проходам.

## Проверки

[tests/check.py](tests/check.py): генерация/сборка на Linux, корректный и ошибочный
синтаксис, UTF-8/BOM, повреждённые кодировки, NUL и отсутствующий входной файл.
Время каждого процесса ограничено. Проверена также сборка и тесты отдельно
скопированной и переименованной папки фронтенда с `-pedantic-errors`.

## Указатель правил грамматики

[Limbo](limbo.atg#L371) | [Top](limbo.atg#L382) | [NamedTop](limbo.atg#L389) | [DeclarationTail](limbo.atg#L403) | [DeclType](limbo.atg#L414) | [AdtMember](limbo.atg#L448) | [Tags](limbo.atg#L463) | [Names](limbo.atg#L470) | [Name](limbo.atg#L478) | [Type](limbo.atg#L485) | [Parameters](limbo.atg#L514) | [Formal](limbo.atg#L533) | [FunctionTail](limbo.atg#L547) | [Block](limbo.atg#L556) | [Statement](limbo.atg#L567) | [IfStatement](limbo.atg#L627) | [WhileStatement](limbo.atg#L635) | [ForStatement](limbo.atg#L649) | [CaseStatement](limbo.atg#L667) | [AltStatement](limbo.atg#L676) | [CaseArm](limbo.atg#L686) | [PickStatement](limbo.atg#L696) | [PickArm](limbo.atg#L705) | [ArmBody](limbo.atg#L719) | [Qualifiers](limbo.atg#L728) | [Expression](limbo.atg#L745) | [Assignment](limbo.atg#L754) | [Cons](limbo.atg#L786) | [LogicalOr](limbo.atg#L796) | [LogicalAnd](limbo.atg#L804) | [BitOr](limbo.atg#L812) | [BitXor](limbo.atg#L820) | [BitAnd](limbo.atg#L828) | [Equality](limbo.atg#L836) | [Relational](limbo.atg#L848) | [Shift](limbo.atg#L862) | [Additive](limbo.atg#L874) | [Product](limbo.atg#L886) | [Unary](limbo.atg#L901) | [Initializer](limbo.atg#L981) | [Postfix](limbo.atg#L990) | [Primary](limbo.atg#L1032)

## Описание языка

[Limbo language reference, syntax summary](https://www.inferno-os.org/inferno/papers/limbo.html).

Добавлены метки while/do/for/case/alt/pick, list of, унарный *,
проверяемая форма array и срезы с необязательными границами. См. [обзор](../README.ru.md).

## Синтаксис 0.4.2

Добавлены implement со списком модулей, полиморфные функции/ADT/типы и for-ограничения,
exception/raises/raise и обработчики блока, fixed-типы/преобразования, **/**=,
array of byte-преобразование, завершающая запятая list и срезы с пропущенными
границами. Последний параметр * проверяется (E1302); пустые case/alt/pick и
пустое for-ограничение отклоняются. Сверка с текущей грамматикой Inferno исправила
старый запрет нижней границы среза: a[:n] и a[:] допустимы.
Include-файлы не раскрываются; cast по именованному/параметризованному типу поддержан. Символы/типы и полная
семантическая неоднозначность type/value в полиморфных аргументах ещё требуют
работы. Это не полный Inferno-компилятор и не генератор Dis.

[Полиморфный пример](../examples/Polymorphic.b), [официальная грамматика](https://github.com/inferno-os/inferno-os/blob/master/limbo/limbo.y).

Новые правила / [PolyParameters](limbo.atg#L1047) | [ForPoly](limbo.atg#L1088) | [Raises](limbo.atg#L1112) | [FixedType](limbo.atg#L1142) | [Export](limbo.atg#L1183)

[Parser coverage 0.6.0](../../PARSER_COVERAGE.ru.md) documents XML/cast fixes and remaining boundaries.

Вспомогательный C-код в авторитетной ATG: [NamedCastAhead](limbo.atg#L112).

[Лексический профиль 0.6.0](../../LEXICAL_PROFILE.ru.md).

0.6.0: [AheadArrow](limbo.atg#L18).
