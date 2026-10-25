# Ввод retained ClipEventKeyPress — v0.628

[English](CLIP_KEYPRESS.md). [`fvm_input_clip_key_code`](../include/flashvm/keyboard.h#L68) переводит копию keyboard
transition в SWF selector. [`fvm_movieclip_dispatch_key_press`](../include/flashvm/clip_actions.h#L106) доставляет именно
этот raw byte по выбранному дереву MovieClip. SDL-плеер выполняет этап после
успешных KeyDown records и именованных методов.

## Коды и чистое преобразование

[Adobe SWF specification](https://open-flash.github.io/mirrors/swf-spec-19.pdf),
CLIPACTIONRECORD и DefineButton2/BUTTONCONDACTION, задаёт KeyCode byte для
ClipEventKeyPress со ссылкой на таблицу кнопок. Специальные SWF-коды отличаются
от virtual keys опроса Key; печатные символы используют ASCII. Это описание wire
selectors, а не подтверждение полного порядка событий оригинального Player.

| Клавиша | Native virtual code | SWF selector |
| --- | --- | --- |
| Left / Right | 37 / 39 | 1 / 2 |
| Home / End | 36 / 35 | 3 / 4 |
| Insert / Delete | 45 / 46 | 5 / 6 |
| Backspace / Enter | 8 / 13 | 8 / 13 |
| Up / Down | 38 / 40 | 14 / 15 |
| Page Up / Page Down | 33 / 34 | 16 / 17 |
| Tab / Escape | 9 / 27 | 18 / 19 |
| Печатный символ | Copied last_ascii | 32–126 |

Raw domain описан `FVM_CLIP_KEY_*`. Передача Key.LEFT=37 как selector не означает
Left: raw 37 — печатный процент. Mapper получает const [FVM_KeyboardState](../include/flashvm/keyboard.h#L50)*, pressed
0/1 и обязательный output. Проверяет весь ABI/held/toggle/code/ASCII snapshot до
изменения output. Invalid input возвращает INVALID_ARGUMENT, сохраняя output.
Нет allocation, обращения к контексту, публикации состояния или мутации входа.

Release, отсутствующий latest code, не удерживаемая latest key и unmapped/nonprintable
дают OK и [`FVM_CLIP_KEY_NONE`](../include/flashvm/clip_actions.h#L24) (ноль); хост пропускает такой результат. На удерживаемом
press специальные virtual identities приоритетны, иначе берётся printable last_ascii.
Mapper не вычисляет символ из virtual letter и повторно не применяет Shift/Caps:
SDL уже подготовила English ASCII с этими модификаторами. Повтор того же pressed
snapshot снова переводится. IME/Unicode composition, автоопределение раскладки и
приёмка физических клавиш не добавляются.

## Явная доставка по дереву

Передайте pinned/удерживаемый native root того же контекста на потоке владельца.
Допустим raw unsigned byte 0–255: он буквально сравнивается с retained selector.
Явные zero/reserved допустимы как буквальный запрос, сохраняя прежний parser/selected
record контракт. Ноль не wildcard; значение больше 255 недопустимо. API не вызывает
onKeyPress/onKeyDown, Key listeners или одноимённый global, не меняет state/focus,
не двигает timeline, не опрашивает таймеры, не выбирает hit target и не рисует.

Узлы SWF1–5 обходятся без KeyPress, SWF6–8 и host version-zero policy исполняют
подходящие records. Native ambient и [context/profile limits](CLIP_INPUT_METHODS.ru.md)
сохраняются. Hidden/stopped и отсутствие focus не фильтруют этот явный global host
route. DefineButton2 conditions и точный порядок focus/visibility оригинала не приняты.

Действует прежний [контракт tree snapshot](CLIP_INPUT.ru.md): подготовить depth order
до callbacks, проверять ancestry/removal перед каждой очередью, пропускать удалённые/
перенесённые ветви, отложить newborn до следующей рассылки. Каждый получатель готовит
текущую таблицу selector/code в свой момент. Подходящие records идут в source order.
Совмещённый KeyPress/KeyDown record игнорирует key selector при KeyDown, но требует
точного совпадения при KeyPress. Persistent cursor/retry не появляется.

Capture/sort, ancestry, проверка records/code и AVM1 делят один execution-work scope.
Исчерпание бюджета/worker stop дают ERROR; неверные topology/type/phase/reentry —
INVALID_ARGUMENT; allocation — OUT_OF_MEMORY. Настоящие handler errors/exceptions
распространяются. Состояние и уже выполненные эффекты не откатываются; остаток batch
пропускается.

[GC:] Внешний native barrier удерживает граф и borrowed code. Collect запрещён,
incremental step не собирает. Повторный вход в любой input-tree API того же контекста
и teardown блокируются. Guards восстанавливаются при ошибках, native throws и OOM.
Batch/flags не сериализуются; retained selectors/code и source identity переносятся
обычными graph API.

Новый host KeyPress route разрешает updateAfterEvent в подходящих records. Запросы
объединяются и переживают ошибки. Старый selected [`fvm_movieclip_dispatch_clip_actions`](../include/flashvm/clip_actions.h#L58)
для KeyPress не получает нового разрешения сам, но наследует eligible outer scope.
Прежний контракт хоста не меняется скрыто.

## SDL: порядок, ошибки и reset

Для queued key-down: опубликовать copied state и Key observers, доставить scene
KeyDown records и onKeyDown methods, затем перевести исходный queued snapshot и
доставить ненулевой KeyPress. Изменения текущего keyboard state предыдущим callback
не меняют выбранную копию клавиши. При этом обработчики видят текущее native state,
включая сознательные изменения ранних callbacks.

Каждое tree event — отдельный синхронный API call с собственным outer budget и новым
снимком topology. Рождённый во время KeyDown клип может получить следующий KeyPress;
он не добавлялся задним числом в KeyDown batch. Общая транзакция/единый числовой
бюджет всего SDL transition не заявляется. Явная регистрация Key listener независима
от scene methods; предотвращение неявного focus-дубля из v0.627 сохраняется.

Key-up и unmapped input не вызывают KeyPress. SDL autorepeat key-down вызывает его;
прежний host counter key_presses учитывает только nonrepeat downs. Escape и Android
back остаются зарезервированы для закрытия: C-хост может передать selector 19, но
плеер не обещает доставить Escape ролику. Shift/Caps и пунктуация сохраняют English
mapping. Namespace/KeyDown error или изменённый reset epoch подавляет KeyPress.

KeyPress error потребляет текущий transition и оставляет следующие в очереди.
Append из callback ждёт следующего entry batch, recursive pump запрещён. Focus/
background reset отменяет старый FIFO. Сообщения инжектирует только отдельная test
executable, не production input.

## Доказательства и пример

- [Mapper C89](../tests/clip_key_code.c): независимая таблица, все ASCII, release/
  unheld/absent, invalid snapshots и сохранение output.
- [Tree](../tests/clip_keypress.cpp): exact/raw/reserved, combined flags, SWF5–8,
  изоляция старых API, отсутствие выдуманного метода/global и неизменность state.
- [Мутации](../tests/clip_keypress_mutation.cpp), [бюджет](../tests/clip_keypress_budget.cpp),
  [OOM](../tests/clip_keypress_allocation.cpp), [ошибки](../tests/clip_keypress_errors.cpp),
  [workers](../tests/clip_keypress_worker.cpp): удалённые/deferred клипы, hidden/stopped,
  partial effects, allocation sweep, exceptions/native throws, GC/reentry/teardown,
  host redraw и остановка настоящих OS workers.
- [Graph/JIT](../tests/clip_keypress_graph.cpp): sender teardown, traced-string GC,
  selector miss/hit и настоящие native entries.
- [Настоящий SDL adapter](../frontend/tests/clip_keypress.c): namespace → record →
  method → KeyPress, focused clip, мутация native state, Shift/Caps/repeat/release,
  F1 без mapping, append/error/reset и redraw.
- [Player](../frontend/tests/clip_keypress.py) с [test queue source](../frontend/tests/clip_player.c):
  21 downs/20 ups, 19 KeyPress, независимый ordered special/ASCII oracle, порядок
  методов, wrong-value controls и redraw именно из KeyPress, SWF5–8 interpreter/JIT.

[C89-пример](../examples/embedding/clip_actions/README.ru.md) добавляет публикацию Left,
перевод 37→1, marker 168 и снятие redraw flag. Это Linux synthetic/dummy-video, не
приёмка физического input, видимого устройства, чужой ОС или полного поведения
оригинального Player. Все девять стадий остаются открытыми.


Проверка v0.628: успешно 286 выбранных Release CTest, 101 whole-core ASan/UBSan CTest
и strict C89 example под санитайзерами. Leak detection выключен. Отдельный fixture
со старым v0.627 SDL adapter ожидаемо отверг тот же SWF8 KeyPress trace movie.
[Статус и границы приёмки](ROADMAP_STATUS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_CLIP_KEY_NONE` | macro / define | [include/flashvm/clip_actions.h:24](../include/flashvm/clip_actions.h#L24) |
| `fvm_input_clip_key_code` | function | [include/flashvm/keyboard.h:68](../include/flashvm/keyboard.h#L68) |
| `FVM_KeyboardState` | struct | [include/flashvm/keyboard.h:50](../include/flashvm/keyboard.h#L50) |
| `fvm_movieclip_dispatch_clip_actions` | function | [include/flashvm/clip_actions.h:58](../include/flashvm/clip_actions.h#L58) |
| `fvm_movieclip_dispatch_key_press` | function | [include/flashvm/clip_actions.h:106](../include/flashvm/clip_actions.h#L106) |
<!-- source-links:end -->
