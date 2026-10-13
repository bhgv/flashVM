# SWF trace runner / Запуск эталонных проверок

Configure with `-DFVM_BUILD_REFERENCE_RUNNER=ON`, build `flashvm_reference_swf`,
and run `flashvm_reference_swf path/to/test.swf > output.txt`. The C89 tool
loads a bounded (64 MiB input) SWF, installs standard objects and the host trace
sink, then executes top-level DoAction tags in file order. Trace writes counted
bytes plus one newline; diagnostics go to stderr. Exit status is 0 on completed
action execution, 1 on load/execution/output failure, 2 for invalid invocation.
The interpreter instruction budget is ten million. FWS and CWS use the normal
VM loader. This is not a frame/input scheduler; it does not run nested sprite
timelines, initialize a display root, or certify complete movie compatibility.

Инструмент собирается явно через FVM_BUILD_REFERENCE_RUNNER и не добавляется
как тест без входного SWF. Вывод отделён от ошибок; ненулевой exit status
нельзя считать успешной эталонной сверкой даже при совпавшем префиксе вывода.
Проверены минимальные FWS/CWS с Trace, повреждённый вход и неверный запуск.

First external probe at pinned [Ruffle commit](https://github.com/ruffle-rs/ruffle/tree/2372a4e148a67d68efd796f4477b3cddd4f880cd/tests/tests/swfs/avm1):
`undefined_to_string_swf6` (original SWF 6) exposes missing legacy string
coercion and undefined-owner CallMethod semantics: execution currently fails
and output differs. `array_slice` and `array_sort` assets in that pinned corpus
are SWF 15, outside FlashVM's requested <=8 range, and are rejected. They cannot
serve as <=8 acceptance evidence without independently compiling their source
for a supported player version. Fixtures/results stay outside the repository.
These findings are open issues, not passing conformance results.

Первый настоящий SWF 6 выявил различия undefined/toString/CallMethod.
Два выбранных Array файла имеют версию 15 и не подтверждают совместимость SWF 8.
Стадии 1/2 остаются открытыми до исправлений и повторного сравнения.

## v0.92 — First exact external SWF match

The original `undefined_to_string_swf6/test.swf` now completes with exit 0 and
exactly matches the pinned 22-byte output file. Ordinary ToString/Add2 use an
empty undefined string below SWF 7; Trace keeps `undefined`. Null/undefined
CallMethod consumes validated operands and returns undefined.

Настоящий SWF 6 завершился без ошибки; все 22 байта совпали с закреплённым
эталоном. avm1_version_strings проверяет переключение SWF 5–8, обе стороны
конкатенации, null, trace и корректность числа аргументов у пустого receiver.
Host C string conversion is unchanged. Numeric parsing, dynamic method-name
coercion and full call semantics remain open. New external probes
`swf6_string_as_bool` and `equals_swf5` complete but still differ in historic
numeric string parsing. Other selected original SWF 4/8 probes encounter
unsupported actions; their output prefixes are not passing acceptance results.

## v0.93 — Three matching external probes

Original SWF 6 `swf6_string_as_bool` (97 bytes) and SWF 5 `equals_swf5`
(261 bytes) now join `undefined_to_string_swf6` (22 bytes) with successful
execution and exact pinned expected output. Files were not patched or relabelled.
Три исходных SWF завершились успешно; вывод полностью совпал с эталоном.
Other corpus probes remain open; top-level DefineLocal and missing actions still
block selected SWF 4/8 tests. No stage is closed by these three matches.
