# Ticks остановленного MovieClip с общим бюджетом

[English](README.md). Исполняемый C89 host показывает onEnterFrame и частичное
выполнение [fvm_movieclip_advance](../../../include/flashvm/movieclip.h#L160). Остановленный однокадровый клип получает handlers.
Три ticks делят пять единиц: два callback выполняются, вход в третий узел расходует
остаток и его обработчик пропускается. Следующий внешний вызов получает свежий
бюджет и выполняет ещё один callback.

## Сборка и запуск

Из корня независимого компонента FlashVM; renderer/SDL/устройство не нужны:

```sh
cmake -S . -B build-ticks -DDEFS=INTERPRETER -DFVM_BUILD_EXAMPLES=ON
cmake --build build-ticks --target flashvm_examples_embedding_movieclip_main
./build-ticks/flashvm_examples_embedding_movieclip_main
```

Ожидаются exit code 0 и:

```text
bounded batch: 2 callbacks retained
after next call: 3 callbacks
depth order: low middle high
render: no timeline callbacks
frame call: frame 2 actions; playhead 1; callbacks 0
initializers: next two ticks fit five units
unload: instance and property retained
```

[main.c](main.c) исполняется; [tutorial_limits.h](tutorial_limits.h) именует границы.
[example_as2.as](example_as2.as) остаётся отдельным эскизом для внешнего AS-компилятора;
этот CMake target его не исполняет. Пример не рисует, не загружает SWF, не делает
poll таймеров и не проверяет OS input.

## API и владение

Контракт: [MovieClip ticks](../../../doc/MOVIECLIP_TICKS.ru.md). Все вызовы идут в
потоке владельца. Host-счётчик живёт до конца уничтожения контекста, callback только
заимствует его. Клип удерживается pin; context_destroy освобождает pin, registration
и граф до выхода счётчика из scope. Нативный GC barrier сохраняется. Неудачный пакет
автоматически не повторяется.

| API | Назначение и время жизни |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L70) | Заполнить ABI host-конфигурации до изменения instruction_limit. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L74) | Скопировать конфигурацию в новый runtime; синхронно, без потока/устройства. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L82) | Создать независимую managed heap под runtime; использовать в потоке владельца. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L22) | Установить стандартные классы/методы; ошибки передаются до создания клипа. |
| [fvm_movieclip_create](../../../include/flashvm/movieclip.h#L120) | Создать однокадровый context-owned клип; возвращённый указатель заимствован. |
| [fvm_gc_pin](../../../include/flashvm/gc.h#L60) | Удержать граф между host-вызовами; pin примера снимается уничтожением контекста. |
| [fvm_object_call](../../../include/flashvm/value.h#L184) | Синхронно вызвать stop с receiver=clip; заимствованный результат игнорируется. |
| [fvm_object_define_method_rooted](../../../include/flashvm/value.h#L171) | Зарегистрировать native onEnterFrame; userdata заимствует счётчик, managed capture отсутствует. |
| [fvm_movieclip_advance](../../../include/flashvm/movieclip.h#L160) | Синхронные ticks с общим native/AVM1 бюджетом; ошибка сохраняет выполненное и пропускает остаток. |
| [fvm_value_undefined](../../../include/flashvm/value.h#L46) | Инициализировать результат callback без выделения и передачи владения. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L86) | Освободить граф/registration до конца жизни userdata; не вызывать из callback. |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L78) | Освободить внешний runtime после контекста; callbacks/устройства уже отсутствуют. |

Пять единиц — намеренно точный регрессионный пример, не длительность и не общий
расход любой сцены со scripts/детьми. [FVM_ERROR](../../../include/flashvm/flashvm.h#L45) ожидается только на заданной границе
пакета; прочие ошибки проходят обычный cleanup. Полный эталонный порядок, ClipActions,
media, JIT/platform acceptance и все этапы роудмапа остаются открытыми.
[Каталог](../../README.md#example-embedding-movieclip).


Добавление v0.629 использует второй независимый runtime с бюджетом 64 единицы.
Создаёт high(10), low(-5), middle(2), регистрирует native handlers с заимствованными
stack entries и проверяет порядок low/middle/high. Pin корня удерживает регистрацию
детей; context cleanup предшествует концу жизни entries/log. Same-depth replacement
и newborn из sibling callback ждут следующего snapshot. Это native depth policy,
не полная сертификация порядка Player. [Контракт](../../../doc/TIMELINE_DEPTH_ORDER.ru.md).

| Дополнительный API | Назначение и владение |
| --- | --- |
| [`fvm_value_string`](../../../include/flashvm/value.h#L58) | Подготовить static child name; конструктор копирует удерживаемое имя при синхронном вызове. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L50) | Подготовить неменеджерный signed-depth scalar в caller-owned [FVM_Value](../../../include/flashvm/value.h#L26). |
| [`fvm_object_call`](../../../include/flashvm/value.h#L184) | Синхронно создать ребёнка через createEmptyMovieClip; root владеет успешным borrowed result. |

Дополнение v0.630 рисует пустую сцену в принадлежащую host память RGBA и проверяет отсутствие лишних timeline callbacks. Для пустой геометрии backend не нужен; реальные paths требуют raster backend. Четвёртая строка: `render: no timeline callbacks`. См. [контракт отрисовки](../../../doc/RASTER_DEPTH_ORDER.ru.md).

| Добавленный API | Назначение и время жизни |
| --- | --- |
| [`fvm_movieclip_render`](../../../include/flashvm/movieclip.h#L174) | Синхронно заимствует поверхность host и закреплённую сцену; без tick и асинхронного удержания памяти. Ошибка сохраняет прежние пиксели/эффекты getters. |

v0.631 adds a third independent runtime / третий независимый runtime: define frame
two, invoke its actions twice through the public execution service with a fresh five-unit
limit per outer host call, and verify `called=42`, playhead one and zero tick callbacks.
The fifth output line is shown above. / Пятая строка показана выше.
[EN contract](../../../doc/FRAME_BLOCKS.md) / [RU](../../../doc/FRAME_BLOCKS.ru.md).

| Added API / API | Purpose, ownership and blocking / Назначение и владение |
| --- | --- |
| [`fvm_movieclip_define_frame`](../../../include/flashvm/movieclip.h#L143) | Copy/validate caller-owned AVM1 bytes and label into the clip's graph; synchronous, does not execute code. / Копирует bytes/метку; без исполнения. |
| [`fvm_exec_call_frame`](../../../include/flashvm/execution.h#L456) | Borrow a rooted receiver and scalar operand synchronously; selected actions share one limit, preserve earlier effects on failure, and do not seek/tick. / Синхронно заимствует receiver/операнд; эффекты до ошибки сохраняются, playhead не меняется. |
| [`fvm_object_get`](../../../include/flashvm/value.h#L102) | Read committed scalar properties into caller-owned values; callbacks/errors follow normal property semantics. / Читает числовой результат и playhead; обычные getters могут вернуть ошибку. |

v0.632 adds a fourth independent runtime / четвёртый независимый runtime. A raw
33-byte SWF8 holds an empty sprite and one End-only DoInitAction. Loading runs no code.
The first tick fits five units; the next outer call completes two ticks in four units,
with three total onEnterFrame callbacks. / Первый tick укладывается в пять единиц,
следующие два вместе — в четыре; итого три onEnterFrame callback. Sixth output line
is shown above. [EN](../../../doc/INITIALIZER_PROGRESS.md) / [RU](../../../doc/INITIALIZER_PROGRESS.ru.md).

| Added API / API | Purpose and ownership / Назначение и владение |
| --- | --- |
| [`fvm_movieclip_load_swf`](../../../include/flashvm/movieclip.h#L129) | Synchronously validate/import caller-owned bytes, retain copied code and return a borrowed context-owned clip; no initializer runs until advance. Pin the graph before later host GC. / Копирует код без исполнения; закрепить возвращённый граф до GC. |
Снимки управляющих записей загруженной сцены описаны в [контракте](../../../doc/SCENE_CONTROLS.ru.md). Этот пример не изменяет private metadata; проверки такой мутации находятся в tests/.
## Native content clearing / Очистка содержимого

The executable also prints `unload: instance and property retained` after an
independent safe-point clear. / Пример также очищает экземпляр в безопасной
точке и проверяет сохранение собственного свойства.

| API | Purpose / Назначение |
| --- | --- |
| [`fvm_movieclip_unload`](../../../include/flashvm/movieclip.h#L264) | Atomically clear contents; caller retains the instance. / Атомарная очистка, экземпляр удерживает хост. |
| [`fvm_movieclip_attached_to`](../../../include/flashvm/movieclip.h#L252) | Borrowed native ancestry snapshot; output changes only on success. / Проверка физических связей, выход меняется при успехе. |

Contracts, ownership, errors and remaining lifecycle limits:
[MOVIE_UNLOAD.md](../../../doc/MOVIE_UNLOAD.md) /
[MOVIE_UNLOAD.ru.md](../../../doc/MOVIE_UNLOAD.ru.md).
No network/audio cancellation or synthetic unload event is demonstrated.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../../../include/flashvm/flashvm.h#L45) |
| `fvm_exec_call_frame` | function | [include/flashvm/execution.h:456](../../../include/flashvm/execution.h#L456) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:60](../../../include/flashvm/gc.h#L60) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:160](../../../include/flashvm/movieclip.h#L160) |
| `fvm_movieclip_attached_to` | function | [include/flashvm/movieclip.h:252](../../../include/flashvm/movieclip.h#L252) |
| `fvm_movieclip_create` | function | [include/flashvm/movieclip.h:120](../../../include/flashvm/movieclip.h#L120) |
| `fvm_movieclip_define_frame` | function | [include/flashvm/movieclip.h:143](../../../include/flashvm/movieclip.h#L143) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:129](../../../include/flashvm/movieclip.h#L129) |
| `fvm_movieclip_render` | function | [include/flashvm/movieclip.h:174](../../../include/flashvm/movieclip.h#L174) |
| `fvm_movieclip_unload` | function | [include/flashvm/movieclip.h:264](../../../include/flashvm/movieclip.h#L264) |
| `fvm_object_call` | function | [include/flashvm/value.h:184](../../../include/flashvm/value.h#L184) |
| `fvm_object_define_method_rooted` | function | [include/flashvm/value.h:171](../../../include/flashvm/value.h#L171) |
| `fvm_object_get` | function | [include/flashvm/value.h:102](../../../include/flashvm/value.h#L102) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../../../include/flashvm/standard.h#L22) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../../../include/flashvm/value.h#L26) |
| `fvm_value_number` | function | [include/flashvm/value.h:50](../../../include/flashvm/value.h#L50) |
| `fvm_value_string` | function | [include/flashvm/value.h:58](../../../include/flashvm/value.h#L58) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
<!-- source-links:end -->
