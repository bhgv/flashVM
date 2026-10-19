# updateAfterEvent и объединение redraw — v0.623

[English](UPDATE_AFTER_EVENT.md). [fvm_standard_install](../include/flashvm/standard.h#L20) добавляет
`updateAfterEvent()` для host/SWF5–8 со стабильным ID [FVM_STAGE_UPDATE_CALLBACK](../src/standard/stage_update_constants.h#L5).
Результат undefined, лишние аргументы не преобразуются. Ниже SWF5 возвращается
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46), включая установку до загрузки старого SWF. Конфликтующие
собственные имена сохраняются по активным правилам регистра SWF.

## Допустимые маршруты и время жизни

Глобальная функция отмечает запрос только внутри динамически допустимой цепочки:

- Function либо object/method callback повторяющегося setInterval.
- Native-доставка Mouse move/down/up слушателям.
- Доставка onKeyDown/onKeyUp выбранному native MovieClip.

Обычный вызов, отдельный setTimeout, wheel, наблюдатели Key, Selection,
AsBroadcaster и onEnterFrame сами не дают права на запрос. Вложенные функции и
маршруты сохраняют уже допустимый внешний interval/input scope. Если MovieClip
одновременно выбран и зарегистрирован наблюдателем Key, допустим только первый
вызов как выбранному клипу, а не последующий observer-вызов. Полная автоматическая
доставка ClipActions/target hit testing этим не добавлена.

[FVM_StageUpdateEventScope](../src/standard/stage_update.hpp#L9) восстанавливает внешнее состояние при успехе, ошибке,
script exception и OOM. Уже отмеченный запрос остаётся pending при последующей
ошибке callback. [GC:] Булевы поля не удерживают managed pointers; GC внутри
script/native-body callback не потребляет запрос и не завершает scope.
Нативные барьеры остаются активными. Сам global не выделяет память, не вызывает
coercion и не рисует рекурсивно; обвязка Function/global может выделять память.

Повторные запросы объединяются в один локальный флаг контекста. Перенос Function
сохраняет callback ID, но не pending/eligibility отправителя. Импортированная
Function использует текущую цепочку получателя. Это не сериализованное свойство
фильма и не межконтекстный handle дисплея.

## C89 API host

Все операции — на потоке владельца, без памяти и callback. Нулевой контекст и фазы
уничтожения/сборки/освобождения дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) с сохранением состояния
и переданных результатов. Установка standard и script-event scope не нужны.

| API | Контракт |
| --- | --- |
| [fvm_stage_request_update](../include/flashvm/stage.h#L38) | Отметить объединяемый запрос host независимо от script-события; timeline/render/device не выполняются. |
| [fvm_stage_get_update](../include/flashvm/stage.h#L41) | Прочитать 0/1 в обязательный `int *` без потребления и передачи владения. |
| [fvm_stage_take_update](../include/flashvm/stage.h#L45) | Прочитать и снять флаг в обязательный `int *` до отрисовки: новый запрос тогда сохраняется. При повторе неудачной презентации снова вызвать request. |

Renderer владеет презентацией и обработкой её ошибок. Чтение/потребление флага
не доказывает показ пикселей в окне. Самостоятельный host может использовать свой
адаптер и прочитать флаг после восстанавливаемой ошибки script. Явные host-запросы
не зависят от SWF/event-ограничений глобальной функции.

## Общий desktop player

Активный цикл потребляет запрос до внеочередного render текущей сцены между
кадрами. [clear_stage](../frontend/common/player.c#L73) и [fvm_movieclip_render_view](../include/flashvm/movieclip.h#L79) используют тот же presenter,
что обычные кадры. Timeline, frame scripts и authored deadline не продвигаются.
Совпавший с обычным кадром запрос удовлетворяется этим кадром. Запрос во время
презентации остаётся pending и не теряется за ожиданием. Фоновая приостановка и
ограниченный показ последнего кадра сохраняют прежнюю политику. При ошибке render
этот player завершает фильм и освобождает контекст.

`--stats` добавляет `presentations=N refreshes=M`, сохраняя строку `frames=...`:
refreshes считает успешные redraw между кадрами, presentations — все успешные
показы host. Timeline ticks считаются отдельно. Это не сертификат точного
расписания/event order оригинального Flash и не latency benchmark.

## Исполняемые проверки

- [Маршруты/host API](../tests/stage_update.cpp): host/SWF4–8, interval/method,
  timeout/Mouse/wheel/focus/Key, объединение, ошибки и lifecycle.
- [Scopes/GC](../tests/stage_update_scopes.cpp): дублированная focus/observer-доставка,
  наследование scope вложенным wheel, собирающий script body и восстановление
  после ошибок/исключений.
- [OOM](../tests/stage_update_allocation.cpp): host API без выделений и восстановление
  eligibility при каждом отказе подготовки активации interval.
- [Версии/конфликты](../tests/stage_update_versions.cpp): property/native slots,
  регистр, поздняя загрузка SWF4, игнорирование преобразуемых аргументов и фазы.
- [Графы](../tests/stage_update_graph.cpp): экспорт внутри допустимого callback,
  уничтожение отправителя, отсутствие переноса флагов, imported Function после GC.
- [Native JIT](../tests/jit_stage_update.cpp): прямые/alias вызовы, interval и Mouse
  в interpreter/manual native/automatic JIT с проверкой native entries.
- [Player/пиксели](../frontend/tests/update_after_event.py): отдельно настоящий SDL
  dummy-video player и [протокольный capture backend](../frontend/tests/update_capture.c).

Протокольный backend использует управляемые часы и настоящие VM/player/rasterizer,
но не native window. При 2 fps interval 10 мс скрывает красную сцену и дважды
вызывает global; второй кадр возвращает красный. Обязательны ровно 49 белых
презентаций в 10–490 мс между красными в 0/500 мс. Контроль без запроса даёт только
два красных кадра. Проверяются coalescing и отсутствие продвижения timeline.
Временный player с циклом v0.622 пропустил все белые interframe-пиксели. Настоящий
SDL dummy-video отдельно проверяет успешные presenter calls/counts. Ни один
вариант не выдаётся за приёмку видимого устройства.

[Пример C89](../examples/embedding/timers/README.ru.md) читает/потребляет запрос
interval и проверяет объединение явных запросов host без graphics backend.

[Справочник Adobe AS2](https://open-flash.github.io/mirrors/as2-language-reference/global_functions.html),
раздел updateAfterEvent, указывает Flash 5 и ограниченные event/interval вызовы
для обновления независимо от FPS. Точное поведение оригинального Player,
полные MovieClip/ClipActions-маршруты и другие платформы/устройства остаются
открытыми. Целая стадия роудмапа этим ограниченным профилем не закрывается.

С v0.625 выбранные retained MouseMove/Down/Up и KeyDown/Up ClipActions также
дают eligibility; остальные retained events её не дают. [ClipActions](CLIP_ACTIONS.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_render_view` | function | [include/flashvm/movieclip.h:79](../include/flashvm/movieclip.h#L79) |
| `fvm_stage_get_update` | function | [include/flashvm/stage.h:41](../include/flashvm/stage.h#L41) |
| `fvm_stage_request_update` | function | [include/flashvm/stage.h:38](../include/flashvm/stage.h#L38) |
| `fvm_stage_take_update` | function | [include/flashvm/stage.h:45](../include/flashvm/stage.h#L45) |
| `FVM_STAGE_UPDATE_CALLBACK` | macro / define | [src/standard/stage_update_constants.h:5](../src/standard/stage_update_constants.h#L5) |
| `FVM_StageUpdateEventScope` | class | [src/standard/stage_update.hpp:9](../src/standard/stage_update.hpp#L9) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../include/flashvm/standard.h#L20) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
