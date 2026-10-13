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

## v0.95 — SWF 8 shift probe

Original SWF 8 biturshift_swf8 now completes with exact 93-byte expected output.
Top-level declarations and the SWF 8 signed URShift result were both necessary.
Четвёртый внешний SWF совпал с эталоном; полная AVM1 приёмка остаётся открытой.

## v0.106 — Root timeline comparison

`flashvm_reference_swf --timeline TICKS INPUT.swf` imports the actual root
MovieClip, publishes `_root` and `_level0`, then advances 1–10000 ticks.
Loading never substitutes an empty clip for unsupported resource/control tags:
those inputs fail explicitly. The original no-option mode remains available.
This bounded mode does not simulate host input, audio or rendering schedules.
The optional `flashvm_reference_runner` test generates two-frame FWS/CWS files
and checks root receiver identity, frame progression, argument limits, corrupt
input and unsupported resource tags. Standard installation follows loading so
SWF version information is already available.

Six additional original pinned Ruffle assets complete with exit 0 and exact
output: equals2_swf5 (7487 bytes), equals2_swf6/7 (7488 each),
lessthan2_swf5/6 (10642 each), lessthan2_swf7 (11000). Together with the four
previous matches this gives ten passing external probes, without patched assets.
The broader swf4_actions_coercion_order asset now reaches CharToAscii and fails
there; its matching prefix is not acceptance evidence for the entire probe.

Режим `--timeline` загружает настоящий корневой MovieClip и исполняет заданное
число тактов. Шесть эталонов SWF 5–7 полностью совпали, включая `this`/`_root`.
Теперь точно совпадают десять внешних проверок. Неизвестные теги, отсутствующие
Action-коды и полная совместимость сцены остаются открытыми; этапы не закрыты.

## v0.122 — explicit host quit acknowledgement / подтверждение quit

`--acknowledge-quit` explicitly installs a host sink that accepts only the
complete ASCII-case-insensitive `fscommand:quit` URL with zero flags. The tool
continues bounded execution after the request; it does not exit, navigate or
perform network/file requests. This is a selected host policy, not proof of
actual player-quit semantics. Other URLs and request flags remain unsupported.
The option precedes optional `--timeline TICKS`. Default behavior is unchanged.

The C89 runner test checks default rejection, acknowledgement in both modes,
continued post-request traces, GetURL/GetURL2, unknown URLs/flags and arguments.
Original `add_swf5` completes with 229 exact expected bytes in action mode, and
`array_trivial` SWF 6 completes with 1447 exact bytes through the actual root
timeline, both with acknowledgement. No headers/tags/outputs were edited.
Together with ten earlier matches this gives twelve whole-probe matches under
the documented runner/host policies; no roadmap stage is closed.

Явный параметр устанавливает обработчик только запроса `fscommand:quit` без
флагов. Он подтверждает запрос и продолжает ограниченное выполнение, без выхода
и внешних действий; остальные URL отклоняются. Это тестовая политика хоста,
а не подтверждение поведения завершения настоящего проигрывателя. Два исходных
SWF 5/6 полностью совпали с эталонами; теперь проверены двенадцать проб с
указанными режимами и политиками, без изменения файлов. Все стадии открыты.
