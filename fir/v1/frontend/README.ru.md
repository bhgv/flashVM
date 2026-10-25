# Структурный парсер FIR 1.0

Версия фронтенда 0.1.2; версия инструментов FIR 0.7.0. Это отдельный C99-парсер
по [firsyn.atg](firsyn.atg), генерируемый внешним bhgv/coco-r-C. Он распознаёт
расширенную структуру FIR; не строит FirModule и не подтверждает типы, SSA,
доминирование, таблицу операций, эффекты или возможности backend.

Поддержаны несколько функций с типизированными параметрами/результатами,
параметры блоков и аргументы переходов, if с двумя исходами, jump/return,
invoke с normal/catch, записи file/loc, source_language/source_semantics,
requires с версией, type/global/import/export. Типы: целые 8/32/64 бит,
f32/f64/bool/any/void, прежние string/channel, as.string/limbo.string,
as3.namespace/as3.multiname, ref<@Class>, array<T>, chan<T>, tuple<T,...>,
func(T,...)->T и именованные @Type. Операции имеют имя и структурные операнды;
const.any имеет явный вид number/string/bool/null/undefined. Неизвестное имя
операции синтаксически допустимо; его обязательно должен отклонить или разрешить
последующий верификатор. Успех здесь не разрешает оптимизацию или генерацию SWO.

Первой физической строкой остаётся FIR 1.0 с LF/CRLF, без BOM/предшествующего
комментария. Вход — UTF-8 без NUL, максимум 2 МиБ. Комментарии // и /* … */;
строки и escape совместимы с текущим FIR-reader. Координаты в байтах.
Приоритет и ассоциативность выражений здесь не угадываются: FIR — явные операции.

## CLI и сборка

```sh
cmake -S . -B build -DCOCOR_ROOT=/host/coco-r-C \
  -DCOCOR_EXECUTABLE=/host/coco-r-C/sources/cocor
cmake --build build
ctest --test-dir build --output-on-failure
build/firsyn_parse INPUT.fir
```

Сборка из этого каталога самостоятельна, включая переименованную копию.
Для проекта FIR целиком используйте cmake -S fir/v1; по умолчанию добавляется
режим `fir1 syntax INPUT.fir`. Его можно отключить через FIR_BUILD_SYNTAX_FRONTEND=OFF.
Команды check/print/opt/object продолжают использовать прежний строгий
исполняемый профиль. Новые конструкции не понижаются в SWO автоматически.
Coco/R нужен на машине сборки, исполняемый парсер — на целевой машине.

## C API

Цель CMake — firsyn_frontend, публичный заголовок — [firsyn.h](firsyn.h).
`firsyn_check(bytes,size,diagnostic)` и `firsyn_read(FILE*,diagnostic)` возвращают
1 при успехе, 0 при отклонении. diagnostic необязателен. Байты, FILE и диагностика
принадлежат вызывающему коду. read читает с текущей позиции, не перематывает и не
закрывает FILE. Ошибки не печатаются; процесс не завершается; VM не создаётся.
На одной библиотеке вызовы должны быть сериализованы: парсер не reentrant.
Глобальные символы имеют префикс firsyn и не пересекаются с fir1/src1/AS/Limbo.
После любой ошибки следующий последовательный вызов снова работает.
Код 1401 — неверный заголовок; -1 — кодировка/размер/аргумент, -2 — отказ
сканера, -3/-4 — предел вложенности/работы. При отказах ресурса координаты могут
быть нулевыми. Ограничения: 512 активных защищённых вызовов и 4 000 000 входов
в защищённые правила; это не число скобок или гарантия полной верификации.

[Пример](../examples/structural/README.ru.md) · [Исполняемый профиль](../doc/FORMAT.ru.md) ·
[Сборка для других ОС/CPU](../../../BUILDING.ru.md).

## Точные объявления и правила

[firsyn_check](firsyn.h#L31) · [firsyn_read](firsyn.h#L32)

[letter](firsyn.atg#L51) | [digit](firsyn.atg#L56) | [hex](firsyn.atg#L61) | [quotedbyte](firsyn.atg#L66) | [escaped](firsyn.atg#L71) | [linebyte](firsyn.atg#L76) | [nonstar](firsyn.atg#L81) | [nonstarslash](firsyn.atg#L85) | [ident](firsyn.atg#L91) | [metadata](firsyn.atg#L102) | [value](firsyn.atg#L110) | [symbol](firsyn.atg#L119) | [label](firsyn.atg#L128) | [numeric](firsyn.atg#L138) | [hexnum](firsyn.atg#L176) | [textlit](firsyn.atg#L193) | [linecomment](firsyn.atg#L204) | [blockcomment](firsyn.atg#L212) | [Firsyn](firsyn.atg#L234) | [ModuleItem](firsyn.atg#L244) | [Signature](firsyn.atg#L275) | [Parameters](firsyn.atg#L284) | [Function](firsyn.atg#L292) | [Type](firsyn.atg#L305) | [TypeList](firsyn.atg#L344) | [Block](firsyn.atg#L352) | [Instruction](firsyn.atg#L368) | [Operation](firsyn.atg#L386) | [Operands](firsyn.atg#L410) | [Operand](firsyn.atg#L418) | [Target](firsyn.atg#L439) | [Literal](firsyn.atg#L451)

0.1.2: генерация удаляет старые результаты и проверяет exit status, диагностику и файлы. Тест generator_failure проверяет отклонение ошибочной грамматики.

Coco/R resolver различает метки после return/операций без операндов и настоящие
label-операнды: комментарии, пустые и типизированные параметры проверены.
Это синтаксическое распознавание, не разрешение генерации расширенных типов/phi.
