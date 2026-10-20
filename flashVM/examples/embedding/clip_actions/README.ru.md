# Явная доставка ClipActions в C89

[English](README.md). Host загружает встроенный SWF8 sprite с четырьмя authored records:
EnterFrame записывает `seen=17`, Load — `seen=42`. Обычный tick размещает и продвигает
sprite, затем host выбирает actor, явно доставляет Load и проверяет marker. Затем публикует pointer state, рассылает
retained MouseMove по сцене и снимает coalesced redraw request.
Окно, OS input, компилятор, внешний файл и SDL не нужны.

## Сборка и запуск

Из корня независимого компонента FlashVM:

```sh
cmake -S . -B build-clips -DDEFS=INTERPRETER -DFVM_BUILD_EXAMPLES=ON
cmake --build build-clips --target flashvm_examples_embedding_clip_actions_main
./build-clips/flashvm_examples_embedding_clip_actions_main
```

Ожидаются exit code 0 и:

```text
explicit Load marker: 42
scene MouseMove marker: 84; redraw: 1
scene MouseMove method: 126; redraw: 1
scene KeyPress key: 1; marker: 168; redraw: 1
```

[main.c](main.c) — самостоятельный C89 host. [clip_action_movie.h](clip_action_movie.h)
содержит независимо закодированные immutable wire bytes, не текстовую AS-программу
и не borrowed resource handle. Loader копирует records. Host владеет runtime/context
и pin корня, удерживающим actor/library/code. Context destruction освобождает граф
и pin до конца жизни локальных данных. После cleanup указатели не используются.

## Контракты API

[Подробный контракт](../../../doc/CLIP_ACTIONS.ru.md). Все вызовы — в потоке владельца.
Доставка синхронная, ошибки сохраняют уже выполненные эффекты. Native barrier
блокирует сборку во время исполнения; GC выполняется на host safe points. Явный Load
повторяем: host отвечает за once-only lifecycle. Native onLoad этим API не вызывается.

| API | Назначение и владение |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L62) | Инициализировать ABI host-конфигурации до создания runtime. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L64) | Скопировать конфигурацию и получить внешнего владельца; без worker/устройства. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L68) | Создать managed heap; дальнейшие вызовы — в её потоке владельца. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L20) | Установить классы/методы до создания загруженной сцены. |
| [fvm_movieclip_load_swf](../../../include/flashvm/movieclip.h#L76) | Скопировать/проверить SWF и сохранить private handlers; root заимствован, scripts при загрузке не исполняются. |
| [fvm_gc_pin](../../../include/flashvm/gc.h#L48) | Удержать root/actor/library между вызовами; pin примера освобождает context_destroy. |
| [fvm_movieclip_advance](../../../include/flashvm/movieclip.h#L99) | Выполнить tick, разместить actor и доставить EnterFrame; Load автоматически не доставляется. |
| [fvm_object_get](../../../include/flashvm/value.h#L72) | Синхронно прочитать actor и scalar marker; borrowed object остаётся достижимым через root. |
| [fvm_movieclip_dispatch_clip_actions](../../../include/flashvm/clip_actions.h#L55) | Исполнить retained Load bytecode с receiver actor и key_code ноль; без выбора цели, native onX, продвижения кадра или показа. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L70) | Освободить граф/registration/pin после завершения вызовов; managed pointers недействительны. |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L66) | Освободить внешнего владельца после контекста. |

[FVM_CLIP_EVENT_LOAD](../../../include/flashvm/clip_actions.h#L5) выбирает raw authored event. Остальные flags и KeyPress selectors
описаны в заголовке и руководстве. Проверяется одна выбранная запись, не планирование
оригинального Player, сеть, hit testing, видимый вывод или приёмка чужой платформы.
При ошибке автоматического retry нет. [Каталог](../../README.md#example-embedding-clip_actions).
Tree path v0.626: [CLIP_INPUT.ru.md](../../../doc/CLIP_INPUT.ru.md). Встроенный
MouseMove записывает marker 84 и вызывает updateAfterEvent; host получает один
pending request. Здесь намеренно raw state publication; если нужны namespace
listeners/drag policy, интеграция должна также вызвать namespace dispatch перед деревом.

В v0.627 пример затем устанавливает native onMouseMove и вызывает общий маршрут:
запись снова пишет 84, метод проверяет её результат и пишет 126, оба запроса redraw
объединяются. [Контракт методов](../../../doc/CLIP_INPUT_METHODS.ru.md).

| API | Назначение, ошибки и владение |
| --- | --- |
| [`fvm_input_set_pointer`](../../../include/flashvm/input.h#L72) | Синхронно опубликовать координаты; не рассылает события и не выбирает цель. |
| [`fvm_movieclip_dispatch_clip_input`](../../../include/flashvm/clip_actions.h#L71) | Обойти удерживаемое дерево и вызвать только записи MouseMove; частичные эффекты не откатываются. |
| [`fvm_movieclip_dispatch_input`](../../../include/flashvm/clip_actions.h#L86) | С тем же pinned root вызвать записи и затем optional method; ошибки прекращают остаток batch. |
| [`fvm_stage_take_update`](../../../include/flashvm/stage.h#L45) | Снять объединённый redraw flag; не рисует и не продвигает timeline. |
| [`fvm_object_define_method_rooted`](../../../include/flashvm/value.h#L115) | Зарегистрировать native method на borrowed actor; здесь userdata/root равны нулю, временного capture нет. |
| [`fvm_object_set`](../../../include/flashvm/value.h#L74) | Синхронно записать scalar marker; вернуть ошибку в наружную доставку, не откатывая предыдущую запись. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | Создать неменеджерный числовой scalar в локальном [FVM_Value](../../../include/flashvm/value.h#L24). |
| [`fvm_call`](../../../include/flashvm/value.h#L138) | Синхронно вызвать updateAfterEvent в event scope; передать ошибку хосту, не выполнять показ. |


Шаг v0.628 публикует копию удерживаемого Left transition, переводит virtual 37 в
SWF selector 1 и доставляет KeyPress с marker 168 и одним redraw request. Это явный
synthetic host transition, не требование OS-клавиатуры. SWF сохраняет этот четвёртый
record. [Контракт](../../../doc/CLIP_KEYPRESS.ru.md).

| API | Назначение и владение |
| --- | --- |
| [`fvm_input_get_keyboard`](../../../include/flashvm/keyboard.h#L71) | Скопировать native snapshot в caller-owned struct точного размера; неверный ABI сохраняет поля. |
| [`fvm_input_set_keyboard`](../../../include/flashvm/keyboard.h#L69) | Синхронно проверить и скопировать удерживаемое состояние без неявной доставки события. |
| [`fvm_input_clip_key_code`](../../../include/flashvm/keyboard.h#L66) | Проверить и перевести неизменяемую копию key-down; ноль означает skip, ошибки сохраняют output selector. |
| [`fvm_movieclip_dispatch_key_press`](../../../include/flashvm/clip_actions.h#L97) | Заимствовать pinned scene для точного selector; разрешить input redraw, сохранить выполненные эффекты при ошибке. |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call` | function | [include/flashvm/value.h:138](../../../include/flashvm/value.h#L138) |
| `FVM_CLIP_EVENT_LOAD` | macro / define | [include/flashvm/clip_actions.h:5](../../../include/flashvm/clip_actions.h#L5) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_input_clip_key_code` | function | [include/flashvm/keyboard.h:66](../../../include/flashvm/keyboard.h#L66) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:71](../../../include/flashvm/keyboard.h#L71) |
| `fvm_input_set_keyboard` | function | [include/flashvm/keyboard.h:69](../../../include/flashvm/keyboard.h#L69) |
| `fvm_input_set_pointer` | function | [include/flashvm/input.h:72](../../../include/flashvm/input.h#L72) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:99](../../../include/flashvm/movieclip.h#L99) |
| `fvm_movieclip_dispatch_clip_actions` | function | [include/flashvm/clip_actions.h:55](../../../include/flashvm/clip_actions.h#L55) |
| `fvm_movieclip_dispatch_clip_input` | function | [include/flashvm/clip_actions.h:71](../../../include/flashvm/clip_actions.h#L71) |
| `fvm_movieclip_dispatch_input` | function | [include/flashvm/clip_actions.h:86](../../../include/flashvm/clip_actions.h#L86) |
| `fvm_movieclip_dispatch_key_press` | function | [include/flashvm/clip_actions.h:97](../../../include/flashvm/clip_actions.h#L97) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:76](../../../include/flashvm/movieclip.h#L76) |
| `fvm_object_define_method_rooted` | function | [include/flashvm/value.h:115](../../../include/flashvm/value.h#L115) |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../../../include/flashvm/value.h#L72) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../../../include/flashvm/value.h#L74) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_stage_take_update` | function | [include/flashvm/stage.h:45](../../../include/flashvm/stage.h#L45) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../../../include/flashvm/value.h#L24) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
<!-- source-links:end -->
