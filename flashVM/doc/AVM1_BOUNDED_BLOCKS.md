# Ограниченные блоки AVM1 / Bounded AVM1 blocks — v0.126

SWF DoAction допускает отсутствие ActionEnd ровно на границе полного блока,
включая пустой блок. Импорт кадров и исполнение сохранённых кадров используют
общий внутренний инспектор ограниченных блоков. Интерпретатор создаёт конечную
запись исполнения на границе; исходные байты не меняются. Переход на эту границу
допустим, внутрь записи или за пределы блока — недопустим. Обрезанные payload и
лишние байты после явного ActionEnd по-прежнему отклоняются.

Публичный [fvm_avm1_inspect](../include/flashvm/avm1.h#L40) остаётся строгим: требует явного ActionEnd. Execute
и execute_receiver допускают неявный конец в контексте с загруженным SWF.
Сохранённые кадры используют структурную границу независимо от загрузки SWF
в принимающем контексте; перенос MovieClip проверен через очередь и после GC.
Версия семантики перенесённого клипа пока определяется принимающим контекстом,
а не исходным SWF: исправление структуры блока не закрывает этот вопрос.
Публичный define_frame сохраняет прежнюю строгую проверку входного кода.
Синтетический конец учитывается в бюджете исполнения, но не в числе
закодированных инструкций инспектора.

avm1_bounded_blocks проверяет эти границы и перенос; avm1_initializer_counts
сохраняет проверки счётчиков. Оригинальный Ruffle init_array_invalid SWF8
выполняется в режиме --timeline 1 без изменения файла и выводит ровно 16 байт
эталона: undefined, c, b, a. Все четырнадцать выбранных исходных проб повторно
сравниваются байт в байт; политики FSCommand описаны в SWF_REFERENCE_RUNNER.md.
[Исходный тест](https://github.com/ruffle-rs/ruffle/tree/2372a4e148a67d68efd796f4477b3cddd4f880cd/tests/tests/swfs/avm1/init_array_invalid).
Все стадии роудмапа остаются открытыми.

## English

Length-delimited SWF DoAction accepts implicit End exactly at the boundary,
including an empty block. Frame import/execution share the private bounded
inspector. Execution appends a terminal action record without modifying source
bytes. Branching to this boundary works; targets inside records or beyond the
block, truncated payloads and trailing bytes after explicit End remain invalid.

Public inspect still requires explicit End. Public execute/execute_receiver
accept implicit End in loaded SWF contexts; unloaded standalone calls remain
strict. Stored frame execution uses its structural boundary even after graph
transfer to an unloaded context. The test transfers through a queue and collects
before advancing the clip. Semantic SWF-version provenance of transferred clips
remains open, as do full initializer semantics and every roadmap stage.
Public define_frame keeps strict validation. The execution budget charges the
synthetic terminal, while inspector instruction counts describe encoded records.

Original pinned init_array_invalid SWF8 now completes in --timeline 1 mode,
matching all 16 expected bytes without editing input or expected output.
Fourteen selected original probes are rechecked under documented host policies.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_avm1_inspect` | function | [include/flashvm/avm1.h:40](../include/flashvm/avm1.h#L40) |
<!-- source-links:end -->
