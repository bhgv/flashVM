# Карты исходников отладочных символов Limbo

Версия **0.1.14**. [English](README.md).

Независимый C99/CocoR reader **source-map prefix limbo .sbl 2.1**. Это исполняемая инфраструктура отладчика, а не полный компилятор Limbo, встраивание Dis или debugger UI. Types/functions/locals/globals после PC prefix намеренно непрозрачны и семантически не проверяются. Прежние версии .sbl первым компонентом не поддерживаются.

## Сборка и проверки

Явно укажите внешний host generator. Проверены bhgv/coco-r-C и C-output frontend [bhgv/coco-r-limbo](https://github.com/bhgv/coco-r-limbo):

```sh
cmake -S compiler/limbo/symbols -B build/sblmap \
  -DCOCOR_ROOT=/host/coco-r-limbo \
  -DCOCOR_EXECUTABLE=/host/coco-r-limbo/cocor
cmake --build build/sblmap
ctest --test-dir build/sblmap --output-on-failure
build/sblmap/sblmap program.sbl
```

[.atg](sblmap.atg) — авторитетный источник LF/CRLF framing и структуры module/file/PC с объявленными counts. C99 actions вызывают [проверяемый decoder](src/map.c). Generated files остаются в build; внешний scanner/runtime не копируется в Git. Namespace отделяет generated globals от других frontend libraries. Выполнение native Limbo generator и Limbo-реализация decoder этими проверками не приняты.

## Семантика prefix и вывода

Читаются header, имя модуля, count/имена файлов, PC count и один source span/group на logical Dis PC. Формат позиции: [file:][line.]position,[file:][line.]position, затем пробел и statement/block identity. Пропущенные file/line наследуются; stop position обновляет состояние следующего start. File IDs нумеруются с нуля; нулевые source line/column сохраняются как исходные неизвестные данные. Координаты редактора и nearest-line breakpoint rule не выдумываются.

Каждая строка требует полного physical terminator. Prefix text fields ограничены 1023 байтами; максимум 4096 файлов, 262144 PCs, input 4 MiB, неотрицательные числа до 2147483647. NUL отклоняется во всём вводе. File spellings сохраняют пробелы/tab/opaque bytes и никогда не открываются как OS paths. Unicode normalization и encoding validation не подразумеваются. Для tail проверяется line framing, но не syntax types/functions/globals. prefix_bytes указывает сразу за PC table либо её нулевым count, до непрозрачного tail.

CLI выводит tabular metadata module/files/pc/prefix, file rows и expanded entries. Имена файлов могут содержать tab: потребитель сохраняет остаток после первых двух разделителей. Exit 0 — map прочитана, 1 — parse/output failure, 2 — CLI/open/allocation setup error. Корректный prefix не доказывает корректность всего .sbl или соответствующего .dis.

## C API и владение

[Публичные C99 declarations](include/sblmap.h). Caller предоставляет доступные непересекающиеся input/output/optional error buffers. Parse использует общее generated state и требует внешней сериализации; API не reentrant. Успешная map владеет копиями строк/entries без заимствования input. Read-only queries не используют parser globals; owner исключает одновременный destroy. Библиотека не вызывает VM callbacks, Dis/FlashVM GC, sockets и filesystem reads.

| API | Контракт |
| --- | --- |
| [sblmap_parse](include/sblmap.h#L37) | Копирует ограниченный ввод и публикует новый *out только при SBL_OK. Прежней map продолжает владеть caller; optional error buffer очищается/записывается. INVALID/LIMIT/NOMEM/UNSUPPORTED сохраняют *out. |
| [sblmap_destroy](include/sblmap.h#L40) | Освобождает partial/complete map; null safe. Заимствованные views прекращают действие. |
| [sblmap_module](include/sblmap.h#L42) | Заимствует имя до destroy; null map даёт null. |
| [sblmap_files](include/sblmap.h#L44) | Число файлов; null даёт ноль. |
| [sblmap_file](include/sblmap.h#L46) | Заимствует spelling до destroy; неверные index/map дают null. |
| [sblmap_count](include/sblmap.h#L48) | Число logical PCs; null даёт ноль. |
| [sblmap_prefix_bytes](include/sblmap.h#L50) | Точная граница prefix; null даёт ноль. Tail не валидируется. |
| [sblmap_entry](include/sblmap.h#L52) | Копирует scalar source/group поля logical PC; неверные аргументы и NOT_FOUND сохраняют output. |
| [sblmap_find](include/sblmap.h#L56) | Первое точное совпадение start-file/start-line от first_pc; NOT_FOUND сохраняет output. Fuzzy/spanning-line selection нет. |

SBL_OK — успех; INVALID — неверный input/arguments, LIMIT — admission bound, NOMEM — allocation failure, UNSUPPORTED — header не 2.1, NOT_FOUND — отсутствие query result. При неверных аргументах diagnostic может быть пуст; status авторитетен. Scanner fatal paths преобразуются в API failures. Полная allocation-fault cleanup и LeakSanitizer этой серией не приняты.

## Доказательства и оставшаяся отладочная работа

17 Python-сценариев включают 4096 независимо сжатых spans, все prefix truncations, перенос file/line state, CRLF, неверные числа/punctuation, limits, NUL, opaque tail и file spellings. C embedding fixture проверяет владение через 64 failure/success resets, сохранение outputs и exact-line lookup. [Optional upstream writer test](tests/upstream_writer.py) компилирует настоящий body функции sblsrcconv upstream в bounded host harness и проверяет 4096 spans. Это не запуск полного upstream compiler/VM.

```sh
python3 compiler/limbo/symbols/tests/upstream_writer.py \
  --upstream /host/Inferno-OS-bhgv --tool /host/build/sblmap
```

[Upstream references](UPSTREAM.md) разделяет Dis debug services и wm/Tk presentation. [Направление компиляторов](COMPILER_DIRECTION.ru.md) фиксирует C/Limbo/CocoR и будущий AS. Следующие слои требуют полного symbol/type/local decoding, module identity/reload, адаптации имеющегося Dis control, channel transport, JIT mappings и настоящего target execution.
