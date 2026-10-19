# Сохраняемые обработчики SWF ClipActions — v0.625

[English](CLIP_ACTIONS.md). Загрузчик PlaceObject2 теперь сохраняет ClipActions
экземпляра sprite вместо отказа на всём optional-поле. Код копируется и проверяется
без исполнения. [fvm_movieclip_advance](../include/flashvm/movieclip.h#L66) доставляет EnterFrame после frame actions
клипа, перед необязательным onEnterFrame. Остальные события доступны явным C89 API;
плеер не выбирает и не планирует их автоматически.

## Формат и строгий профиль

Первичный источник: [Adobe SWF specification](https://open-flash.github.io/mirrors/swf-spec-19.pdf),
разделы ClipActions/ClipEventFlags. Используются значения wire format и строгий
профиль версии файла, без всех послаблений и recovery оригинального Player.

| SWF | Разрешённые события | Размер flags |
| --- | --- | --- |
| 5 | Load, EnterFrame, Unload, MouseMove/Down/Up, KeyDown/Up, Data | UI16 |
| 6 | Предыдущие плюс Initialize, Press, Release, ReleaseOutside, RollOver/Out, DragOver/Out, KeyPress | UI32 |
| 7–8 | Предыдущие плюс Construct | UI32 |

Reserved prefix обязан быть нулевым; верхние reserved bits запрещены. AllEventFlags
должен совпадать с OR всех masks записей. Требуется полный нулевой end marker, без
лишних байтов. Пустая таблица допустима. Усечённые длины, selector, action records и
marker не выходят за границу placement. Для KeyPress ActionRecordSize включает
один raw key byte; этот байт не передаётся AVM1. Существующий inspector допускает
неявный ActionEnd точно на границе блока. Неподдерживаемый AVM1 остаётся unsupported.

Ограничения одной таблицы: [FVM_SWF_CLIP_ACTION_MAX_RECORDS](../src/swf/clip_action_constants.h#L15) записей,
[FVM_MOVIECLIP_MAX_FRAME_ACTION_BYTES](../src/standard/movieclip_constants.h#L21) байтов actions в записи и
[FVM_SWF_CLIP_ACTION_MAX_BYTES](../src/swf/clip_action_constants.h#L16) байтов actions суммарно. Это границы реализации.
Повреждённые/слишком большие записи дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); события вне профиля
версии и ClipActions статической графики — [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Отдельное поле clipDepth
остаётся unsupported. PlaceObject3, DefineButton actions, выбор hit target и текстовый
AS-компилятор не добавлены.

## Явная доставка C89

Подключите `flashvm/clip_actions.h` и вызовите [fvm_movieclip_dispatch_clip_actions](../include/flashvm/clip_actions.h#L55):

- context владельца и удерживаемый same-context MovieClip;
- ровно один бит FVM_CLIP_EVENT_* из публичного заголовка, не комбинацию;
- raw SWF selector 0..[FVM_SWF_CLIP_ACTION_MAX_KEY](../src/swf/clip_action_constants.h#L18) для [FVM_CLIP_EVENT_KEY_PRESS](../include/flashvm/clip_actions.h#L22);
  для всех других событий требуется ноль.

Совпавшие records исполняются по порядку исходника, receiver — выбранный клип.
Combined record исполняется один раз для выбранного события; key фильтруется только
при доставке KeyPress. API не вызывает onX method, не обновляет Key/Mouse/focus,
не выбирает hit target, не удаляет клип, не продвигает кадр, не делает timer poll
и не рисует. Пустая/отсутствующая таблица и отсутствие matches — успешный результат.
Выходного счётчика доставок нет. Native empty clip использует host-профиль, sprite
library сохраняет source SWF version также после переноса графа.

Выбор событий и once-only lifecycle — задача host. Два вызова Load/Construct/
Initialize/Unload выполнят обработчики дважды; скрытого loader cursor нет. Явный
EnterFrame плюс advance также доставит дважды. Load/unload, construct/initialize и data остаются явными. С v0.626 desktop также
рассылает пять global MouseMove/Down/Up и KeyDown/Up через [дерево сцены](CLIP_INPUT.ru.md).
EnterFrame остаётся частью ticks, targeted interaction/lifecycle scheduling открыт.
Native Mouse/Key/focus API сохраняют отдельные контракты.

API синхронный, только для потока владельца. Клип заимствован: удерживайте pin либо
родительскую сцену между вызовами. Null/чужой/nonclip, static art, lifecycle/sound
reentry, неверный event/key дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) до доставки. Валидное событие,
недоступное в строгом source-профиле, даёт [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46).

## Снимок, бюджет и ошибки

Подготовка оплачивает записи и каждый байт actions, проверяет непрерывные индексы,
selectors, masks, границы и lowercase hex, фиксирует selectors и code values.
Выбранный AVM1-блок затем проверяет/исполняет bounded executor. OOM или неверные
метаданные подготовки не исполняют handlers. Замена живой таблицы и изменение
metadata более поздней записи из callback не меняют уже подготовленный пакет;
следующая доставка увидит изменения.

Подготовка, доставка и AVM1 делят [FVM_ExecutionWorkScope](../src/core/execution_work_scope.hpp#L9) с внешним advance/script.
Внешний публичный dispatch дополнительно оплачивает вход, совпавшие records —
единицу доставки. Native work проверяет worker stop на первой единице, затем через
64 единицы. Числовой предел отключается отдельно от stop. Native callback internals
и прочие helpers не становятся hard real-time preemptible.

Budget/stop, [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44), ошибки реального callback и [FVM_EXCEPTION](../include/flashvm/flashvm.h#L48) пропускают
остаток с сохранением выполненных эффектов. Ошибка байткода поздней записи может
следовать за доставленной ранней. Нет rollback или resumable batch cursor.
Обработайте/очистите script exception обычным exception API; не повторяйте ошибочный
пакет автоматически.

MouseMove/Down/Up и KeyDown/Up дают динамическую eligibility updateAfterEvent.
EnterFrame и прочие события сами её не дают; разрешённый внешний interval/input
scope сохраняется. Scope восстанавливается при ошибке/exception/OOM; уже выставленный
redraw остаётся pending. [Контракт перерисовки](UPDATE_AFTER_EVENT.ru.md).

[GC:] Native barrier действует на весь вызов. Внутри callback [fvm_gc_collect](../include/flashvm/gc.h#L44) даёт
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45), [fvm_gc_step](../include/flashvm/gc.h#L90) — [FVM_OK](../include/flashvm/flashvm.h#L42) без сборки. Собирать нужно на последующей
host safe point. Captured code остаётся действительным после замены owner edge.
Native userdata и active work scope в граф не записываются. Код — owned ASCII hex,
а не заимствованный указатель в исходный файл.

## Сцена, duplicate и перенос

Отсутствующее поле ClipActions сохраняет прежнюю таблицу. Присутствующее заменяет
edge данного экземпляра, включая явную пустую таблицу. Seek/loop reconstruction
переносит последнее authored-поле существующей политикой сцены, отдельно от timeline
frames. Динамический duplicate удерживает immutable placement handlers; последующая
замена edge другого экземпляра независима. Это не добавляет копирования обычного
динамического свойства onEnterFrame.

Общий graph transport переносит таблицу, selectors, strings, библиотеку и версию.
После уничтожения отправителя укоренённый импортированный sprite исполняет records.
Это не новый shared display object между потоками; остальные native hooks сохраняют
свои правила переноса.

## Доказательства и пример

[Wire/events](../tests/swf_clip_actions.cpp) и [malformed](../tests/swf_clip_actions_malformed.cpp)
проверяют независимые SWF5–8, raw flags, selectors и все prefix truncations.
[Scene/duplicate](../tests/swf_clip_actions_update.cpp) — замену поля, stopped ticks
и duplicate. [Graph](../tests/swf_clip_actions_graph.cpp) — teardown отправителя,
traced strings и source version. [Budget/errors](../tests/swf_clip_actions_budget.cpp)
и [workers](../tests/swf_clip_actions_worker.cpp) — общий предел, committed effects,
exceptions и настоящие OS threads. [Snapshot](../tests/swf_clip_actions_snapshot.cpp)
и [corrupt graph](../tests/swf_clip_actions_corrupt_graph.cpp) — изменение таблиц/
metadata и отказы подготовки. [OOM](../tests/swf_clip_actions_allocation.cpp) —
последовательные отказы loader/graph, первого placement/tick и dispatch с последующим рабочим вызовом.
[Redraw](../tests/swf_clip_actions_redraw.cpp) и [JIT](../tests/jit_clip_actions.cpp)
проверяют scopes/фазы и interpreter/native equivalence.
[Настоящий player](../frontend/tests/clip_actions.py) исполняет SWF5–8 в SDL dummy-video
interpreter/JIT и отвергает wrong-value controls; это не видимый экран/устройство.

[Проверки публичного API](../tests/clip_actions_api.cpp) охватывают native empty
clip, owner/type, sound phase, неверные аргументы и позднюю загрузку SWF4. Временная
программа со старым timeline loader v0.624 отвергает положительный ClipActions SWF.

[C89-пример](../examples/embedding/clip_actions/README.ru.md) содержит маленький SWF,
явно доставляет Load после первого автоматического EnterFrame. Полное планирование
ClipActions, эталонный порядок событий, автоматические input targets, codecs и
межплатформенная аппаратная приёмка остаются открытыми. Все девять этапов открыты.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_CLIP_EVENT_KEY_PRESS` | macro / define | [include/flashvm/clip_actions.h:22](../include/flashvm/clip_actions.h#L22) |
| `FVM_EXCEPTION` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:44](../include/flashvm/gc.h#L44) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:90](../include/flashvm/gc.h#L90) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:66](../include/flashvm/movieclip.h#L66) |
| `fvm_movieclip_dispatch_clip_actions` | function | [include/flashvm/clip_actions.h:55](../include/flashvm/clip_actions.h#L55) |
| `FVM_MOVIECLIP_MAX_FRAME_ACTION_BYTES` | macro / define | [src/standard/movieclip_constants.h:21](../src/standard/movieclip_constants.h#L21) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_SWF_CLIP_ACTION_MAX_BYTES` | macro / define | [src/swf/clip_action_constants.h:16](../src/swf/clip_action_constants.h#L16) |
| `FVM_SWF_CLIP_ACTION_MAX_KEY` | macro / define | [src/swf/clip_action_constants.h:18](../src/swf/clip_action_constants.h#L18) |
| `FVM_SWF_CLIP_ACTION_MAX_RECORDS` | macro / define | [src/swf/clip_action_constants.h:15](../src/swf/clip_action_constants.h#L15) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
