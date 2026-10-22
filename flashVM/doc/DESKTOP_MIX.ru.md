# Desktop root stream/event composition


v0.975: Добавлен [fvm_swf_stream_output_pump_mix_tail](../include/flashvm/swf_stream_output.h#L82) после EOF: точный retry и согласованные счётчики без изменения публичных структур.


v0.976: Проверены pairing backend, failed/throwing retry, stream-reentry и запрет рассогласованных эпох.


v0.977: Добавлены bounded-владение composition и маршрутизация ID; mixed preflight пока закрыт до интеграционных проверок.


v0.978: Разрешена bounded root-композиция stream/StartSound; навигация согласуется до принятия событий кадра, прежние пути сохранены.


v0.979: Worker подаёт согласованные хвосты после финального EOF, учитывает active/pending события и сохраняет StopSounds-остановку stream.


v0.980: Production C89 refill проверен на явном FIFO: base retry, 5 кадров base+event и 4 кадра хвоста с точными PCM.


v0.981: Root-действия проверены interpreter/native JIT: queued NoMultiple, failed clear, GC и новые события после StopSounds.


v0.982: Проверены pending EOF-tail, пауза без rewind, failed navigation clear и свежие события в новой эпохе.


v0.983: Base и EOF-tail удерживаются до конца frame callbacks; проверен настоящий SDL worker до и после stream EOF.


v0.984: Один predictor используется в preflight/open: отдельный composition-бюджет отвергается до устройства, partial-владение очищается.


v0.985: Проверены переходы источников, future gates, rewind-эпохи и точный PCM противоположных голосов в C89 worker после VM teardown.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_output_pump_mix_tail` | function | [include/flashvm/swf_stream_output.h:82](../include/flashvm/swf_stream_output.h#L82) |
<!-- source-links:end -->
