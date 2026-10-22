# Desktop root stream/event composition


v0.975: Добавлен [fvm_swf_stream_output_pump_mix_tail](../include/flashvm/swf_stream_output.h#L82) после EOF: точный retry и согласованные счётчики без изменения публичных структур.


v0.976: Проверены pairing backend, failed/throwing retry, stream-reentry и запрет рассогласованных эпох.


v0.977: Добавлены bounded-владение composition и маршрутизация ID; mixed preflight пока закрыт до интеграционных проверок.


v0.978: Разрешена bounded root-композиция stream/StartSound; навигация согласуется до принятия событий кадра, прежние пути сохранены.


v0.979: Worker подаёт согласованные хвосты после финального EOF, учитывает active/pending события и сохраняет StopSounds-остановку stream.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_output_pump_mix_tail` | function | [include/flashvm/swf_stream_output.h:82](../include/flashvm/swf_stream_output.h#L82) |
<!-- source-links:end -->
