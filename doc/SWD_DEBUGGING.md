# SWF/SWD source debugging / отладка с исходниками

For the supported AVM1 generations, authoring source/line information can live
in a companion **SWD** file, associated with the SWF's 16-byte **DebugID**.
FlashVM already validated/preserved SWF DebugID/EnableDebugger tags; the new
`flashvm/source_debug.h` API imports the matching SWD source data explicitly.

The reader is an independent bounded implementation based on the record layout
in Apache Flex SDK's primary
[DebugDecoder](https://github.com/apache/flex-sdk/blob/master/modules/swfutils/src/java/flash/swf/DebugDecoder.java)
and [DebugEncoder](https://github.com/apache/flex-sdk/blob/master/modules/swfutils/src/java/flash/swf/DebugEncoder.java).
It does not incorporate their source code. Supported inputs are FWS/CWS paired
with FWD versions 6–8. Records: SCRIPT (0: module/bitmap/name/text), OFFSET
(1: module/line/absolute uncompressed SWF offset), BREAKPOINT (2), ID (3),
REGISTERS (5), explicit end (0xffffffff). Integers are little-endian; names and
source contents are terminated byte strings. Line records may precede modules.

`fvm_debug_load_swd` validates the matching root DebugID, complete framing,
unique module IDs, valid module/line references and conflicting line offsets
before atomically replacing the prior bundle. It rejects unknown records and
unsupported versions. Breakpoint records are validated but not automatically
enabled. No password check is presented as a security boundary, and authoring
source paths never trigger file or network access. Inputs/source files are copied;
one explicitly attached bundle is supported per context.

The API exposes complete source contents, a current line view, copied-body lookup
and materialized register names. Exact bytecode content correlates interpreter,
JIT and retained crash-source copies with canonical uncompressed SWF positions.
Identical bodies with conflicting source locations return NOT_FOUND; register
queries also require a unique body origin. Unknown/ambiguous names and source
lines remain unavailable rather than being guessed. This is not decompilation.

Source lookup is cold: reports, explicit debugger queries and actual breakpoint
hits. It adds no source-file scan to the per-instruction hot path. Symbols keep
borrowed activation source links available even when crash action is NONE.
Breakpoints still route the selected named body through the interpreter; an
active native frame is not patched. A crash callback in JIT can inspect the
executed source line without replaying that function. Manual action maps take
precedence for copied report location; they do not create source text.

Limits: 64 MiB canonical SWF decoding, 16 MiB SWD, 65536 SWD records, 4096 body
candidates and 128 MiB compared body bytes per lookup. Exceeding cold lookup
limits returns UNSUPPORTED. Malformed input and atomic failure preservation are tested;
this does not certify every historic authoring tool's SWD dialect. AVM2/ABC
embedded debug opcodes, automatic external file loading and multi-document
symbol registries remain outside this implementation.

When SWD resolves a crash frame, its filename/line and executing source line are
copied into the report. ZIPs also include available embedded sources under
`sources/moduleN.txt` with original-name/module metadata in `sources/index.json`.
These names are archive-generated; author paths cannot escape the archive.
Clearing symbols invalidates live source views while copied report lines remain
valid. See [complete API reference](DEBUG_API.md), [RU reference](DEBUG_API.ru.md)
and [source breakpoint example](../examples/debugger/source_breakpoints/README.md).

## Русский

Для AVM1 связь с исходниками читается из отдельного SWD, который должен совпасть
с DebugID в SWF. Импорт принимает FWS/CWS и FWD6–8, исходный текст, строки,
имена register-переменных и идентификаторы модулей. Старые символы заменяются
только после полной успешной проверки. Обрезанные данные, несовпадающий DebugID,
неизвестные записи и неверные ссылки отвергаются. Breakpoint из файла сам по
себе не включает остановку. Имена файлов используются как метки: никакие пути
автоматически не открываются и не загружаются.

Интерпретатор, JIT и postmortem-копии используют одну связь с исходным AVM1-телом.
API выдаёт полный текст файла, текущую строку без CR/LF и известные имена
существующих регистров. Неоднозначные одинаковые тела не связываются с чужой
строкой: возвращается NOT_FOUND. Поиск выполняется при запросе, остановке или
ошибке, а не на каждой инструкции. При загруженных символах callback breakpoint
получает имя файла/строку, даже если действие при сбое — NONE.

В отчёт копируется выполнявшаяся строка; ZIP получает также встроенные исходники.
После удаления символов postmortem-копия остаётся доступной. Поддерживается один
явно подключённый набор на контекст. AVM2, автоматическая загрузка исходников,
все исторические диалекты и реестр нескольких документов пока не заявлены.
Обычный и FORCE сохраняют прежнее разделение стоимости и доступности данных.

Validation / проверки v0.534: 1110 CTest cases in each Linux minimal and JIT
profile; five targeted ASan/UBSan cases. swd_sources covers ordered/unordered
records, every truncated input prefix, DebugID/version/record rejection, CWS,
ambiguous duplicate bodies, source lines after symbol removal, JIT fault stops
and interpreter breakpoint registers. swd_allocation checks atomic OOM retries
without replacing a pending exception. The independent Python ZIP reader checks
source module contents, index, executing line, CRC and JSON. The tutorial checker
checks line comments, absence of assertions and every public debugger function's
reference entry and README links. These checks do not certify foreign OS/CPU
execution or every historic SWD producer.
