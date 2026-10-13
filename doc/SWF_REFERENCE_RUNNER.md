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
