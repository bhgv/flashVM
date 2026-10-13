# Stage background / Фон сцены

`flashvm/stage.h` exposes a context-wide RGBA background selection. Querying
an unset selection returns FVM_NOT_FOUND without changing the output. The host
can set any RGBA color or clear the selection. MovieClip import preserves valid
SetBackgroundColor (tag 9, exactly three RGB bytes) as frame controls. Execution
selects an opaque color only while the context has no selection; subsequent
controls preserve the first selection or host override. Loading alone does not
select a color. Rebuilding a timeline visits its earlier controls in order.

Graph transport preserves frame controls; the receiving context keeps its own
ambient background selection. This primitive state has no managed GC edges.
The API does not clear or render host pixel buffers. Full Stage scripting API,
render integration and complete timeline compatibility remain open.

API позволяет получить, задать и сбросить RGBA-фон контекста. Тег SWF выбирает
первый цвет при исполнении кадра, но не заменяет заданный приложением фон.
Загрузка сама по себе фон не меняет; восстановление кадров учитывает предыдущие
теги. При передаче клипа фон контекста получателя остаётся самостоятельным.
Автоматическая очистка пикселей и полный объект Stage пока не реализованы.

Tests: stage_background (C89 API), swf_background (two-frame FWS, seeks, host
alpha override, graph transport, GC, malformed control/input).

v0.105 validation: all 267 tests pass in AGG and Cairo Release (five AGG
executables needed their environment-stripped execute bits restored and were
rerun successfully). Three focused tests pass in portable and ASan/UBSan
headless builds. Sanitizer stack instrumentation and leak detection remain
disabled as documented by the validation profiles.

В v0.105 прошли 267 тестов каждой графической сборки и по три профильных
теста переносимой и санитарной сборок. Ограничения инструментов проверки
памяти сохранены; эти результаты не закрывают этапы роудмапа.
