# Компиляция с debug origins: FIR 0.5.5

[English](SOURCE_DEBUG.md) · [Диагностика](SOURCE_DIAGNOSTICS.ru.md) ·
[Собственный Limbo](../../../compiler/limbo/OWN_COMPILER.ru.md)

Наш Coco/R source compiler принимает `-g DEBUG.json`, включая числовой Limbo.
Это собственный debug-producing путь, отдельно от legacy Dis/.sbl bootstrap.

```sh
./build-workspace.sh --projects fir
export TMPDIR="$PWD/build/workspace/tmp"
build/workspace/fir/fir1_source limbo --input fir/v1/examples/limbo/scopes.b build/workspace/scopes.fir -g build/workspace/scopes.debug.json
```

--module, --diagnostics-json и -g разрешены в разном порядке после input/output;
повторные/незавершённые опции отвергаются. Новый C99 API fir_source_debug принимает
аргументы fir_source_ex и дополнительный caller-owned debug FILE*. Старые API
сохранены. Три потока должны быть раздельными; одинаковые FILE* отвергаются.
Вызовы парсера сериализуются снаружи. Заимствованные FILE/source/error pointers
не остаются после возврата, в том числе после ошибки и следующего plain-вызова.

FIRDBG1 содержит schema=1, stage=source-unoptimized-fir, module, точный UTF-8
снимок и упорядоченные events. constant/binary связывают %vN с исходником,
declare/assign — имя с SSA, trace — входное значение. scope-enter/leave описывают
лексическое время жизни; у них value=-1.

scope — уникальный ID владельца в translation unit, 0 у entry. Соседние блоки
различаются ID даже при одинаковом scope_depth. Порядок enter/leave восстанавливает
вложенность. Присваивание указывает настоящего владельца переменной, включая
запись во внешнюю из внутреннего блока. Выражение относится к текущему владельцу.
ID сбрасываются при новой компиляции. Позиции: byte_offset/byte_length с 0,
физические line/байтовая column с 1; не display/character coordinates.
Учтены unary-minus helper constants: каждое сгенерированное SSA-определение
имеет origin. Native pointers, GC roots и private addresses не сериализуются.

Оба выхода сначала staging. Ошибка исходника не меняет FIR/debug, CLI сохраняет
прежние файлы. Последний I/O-copy/close может оставить префикс либо один файл:
атомарная транзакция двух файлов не обещается. Потоки закрывает владелец.

Это карта **неоптимизированного FIR**. Релокации после peephole, SWO/SWF-линковки,
Dis/JIT PC ещё не представлены. Нельзя принимать %vN/byte_offset за machine PC
или использовать карту после оптимизации без переноса. Полные function/local
layouts остаются задачей Limbo/backend. Явное поле stage не позволяет выдавать
этот шаг за готовый end-to-end debugger.

Реальные Coco/R-тесты проверяют все SSA origins, разные sibling scopes, запись
в outer binding, UTF-8, rollback, повторные API-вызовы и закрытый debug-поток.
Legacy -g/.sbl bootstrap проверяется отдельно и не заменяет наш Dis backend.


### Occurrence identity (additive FIRDBG1 fields)

Each event includes a positive `site` unique within the staged debug artifact and
`binding` (positive lexical declaration identity, or zero when not applicable).
`read`, `declare` and `assign` events distinguish repeated identifier occurrences
without changing existing SSA IDs. Compound assignments record their implicit
read separately. Coordinates remain UTF-8 byte offsets/columns. Combine site
with artifact/build identity; it is not a final Dis/native PC or global ID.
See the [broker contract](../../../disVM/broker/README.md).
