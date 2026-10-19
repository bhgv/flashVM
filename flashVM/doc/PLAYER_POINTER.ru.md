# Первичный контакт и порядок ввода плеера

v0.596 связывает [ядро Mouse](MOUSE.ru.md) с SDL2-плеером для desktop/Android.
Физическая мышь не нужна: первое принятое касание создаёт стандартные
onMouseDown/onMouseMove/onMouseUp. [English](PLAYER_POINTER.md).

## Сбор событий и выбор контакта

Оконный адаптер собирает события в главном потоке без входа в VM. Движение
мыши и её основной кнопки находятся в одной FIFO с поддержанными клавишами,
сохраняя порядок получения. Средняя/правая кнопки обновляют HostInput, но не
создают уведомлений основной кнопки Mouse.

Первый допустимый SDL_FINGERDOWN владеет контактом до соответствующего
SDL_FINGERUP или потери фокуса/ухода в фон. Дополнительные пальцы/устройства
игнорируются, отпускание второго пальца не завершает первый. Удержанная
физическая основная кнопка не позволяет касанию перехватить контакт. Во время
активного касания физические mouse events игнорируются. После отпускания
первичного пальца оставшийся вторичный не повышается автоматически: нужен
новый down. Опознанные SDK синтетические finger от мыши и mouse от касаний игнорируются;
также отключаются synthesis hints SDL, исключая двойную доставку.

Фильтр logical-size renderer SDL уже корректирует нормализованные координаты
пальца по viewport/letterboxing. Адаптер один раз умножает их на фиксированный
логический размер сцены; mouse events уже содержат логические пиксели SDL.
При доставке добавляется authored origin SWF, скриптовые матрицы не меняются.
Неконечные/выходящие за диапазон координаты или отсутствие логического размера
фиксируют [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) до изменения контакта/позиции хоста.
Это один первичный контакт, а не распознавание жестов или полный multitouch;
прижатие координат к краям viewport самим SDL сохраняется.

## Общая очередь и доставка

FVM_DESKTOP_PENDING_EVENTS ограничивает FIFO 128 событиями. В ней нет managed
указателей VM или заимствованных event buffers. Каждое событие хранит свою
позицию и нужный снимок клавиатуры либо pointer type. Движения не объединяются
молча. Переполнение фиксирует [FVM_ERROR](../include/flashvm/flashvm.h#L43); плеер сообщает сбой вместо успешной
доставки потерянных событий.

`fvm_desktop_window_pump_input(view, context, origin_x, origin_y)` синхронно
обрабатывает входную порцию в общем потоке-владельце перед кадром ролика.
Нулевые владельцы, неконечный origin и вложенный pump дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45)
до изъятия события. Клавиатурный callback видит позицию своего события и его
keyboard snapshot; pointer event вызывает [fvm_input_dispatch_mouse](../include/flashvm/mouse.h#L25) с drag до
обработчиков. Перестановки «сначала все клавиши/потом указатель» нет.
Итоговые HostInput/polling публикуются затем по прежнему циклу плеера.

Добавленные callback события ожидают следующего pump. Потеря фокуса или фон
отменяют ожидающие события и очищают удержанные keys/buttons/contact хоста
без искусственных up callbacks. Смена epoch также завершает старую активную
порцию: если callback сбросил фокус и поставил новый ввод, старая порция его
не поглотит. Отмена не скрывает ранее зафиксированное переполнение.
Escape и Android Back по прежнему закрывают плеер.

Текущее событие изымается перед входом в VM. Любая ошибка VM/callback, включая
OOM подготовки, расходует его, оставляя позднейшие события; опубликованные
эффекты следуют контракту ядра. Обычный плеер сообщает ошибку и закрывает ролик.
Зафиксированная ошибка сбора запрещает pump до уничтожения окна. Reentry flag
снимается на всех обычных возвратах статуса. Callback не должен уничтожать или
повторно входить в presentation lifecycle либо бросать исключение через C.
`fvm_desktop_window_pump_keyboard` остаётся внутренней совместимой обёрткой
над общей FIFO с нулевым stage origin.

Плеер явно устанавливает Mouse только для SWF6–8; старые ролики запускаются без
нового namespace. Колесо, видимость курсора, Button/ClipActions, автофокус/цель,
IME, multitouch-жесты и security origins этим мостом не добавлены. Ядро без
плеера по прежнему собирается без SDL.

## Ролики без звука

v0.597 исправляет найденный при запуске готовых роликов дефект: --mute раньше
не создавал ни устройство, ни callback корневого StartSound, поэтому первый
звуковой event останавливал timeline с [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Теперь --mute устанавливает
явный muted_sound: подтверждает только эти проверенные уведомления без декодирования,
очереди PCM, worker и открытия аудиоустройства. Обычный режим со звуком и его
ошибки не меняются. Прочие неподдержанные scene tags/actions по прежнему отклоняются.

Userdata callback заимствованы из живого FVM_DesktopAudio плеера; очистка всегда
снимает callback до обнуления этой памяти. sound_events учитывает и намеренно
пропущенные звуковые уведомления, audio_device остаётся нулём. StopSounds успешен
по существующему контракту пустого контекста: фиктивный output/stop binding не создаётся.

`frontend/tests/muted_movies.py` выполняет все три готовых ролика по 120 кадров
в интерпретаторе и настоящем native JIT, с dummy video и заведомо неверным
аудиодрайвером. Требует успешный выход, звуковые события, реальные счётчики
исполнения и отсутствие аудиоустройства. Это muted-runtime проверка, а не
прослушивание, аппаратная или codec-приёмка.

## Повторение ограниченных проверок

Из компонента FlashVM с настоящим SDK SDL2:

```sh
cmake -S . -B build -DFVM_BUILD_DESKTOP_FRONTEND=ON -DSDL2_DIR=/path/to/SDL2/cmake
cmake --build build --target flashvm_frontend_pointer flashvm_frontend_keyboard
ctest --test-dir build -R '^flashvm_frontend_(pointer|keyboard)$' --output-on-failure
```

`frontend/tests/pointer.c` использует настоящую SDL с явно выбранным dummy video.
Проверяет общий порядок, origin, два пальца, подавление синтетических дубликатов,
короткие тапы, добавление/ошибку/сброс из callback, вложенный pump и переполнение.
SDL_PushEvent через изменённый logical renderer проверяет настоящий letterbox
фильтр, а не вторую ручную проекцию. Прежний keyboard fixture проверяет polling,
короткие нажатия, сброс и переполнение через новую FIFO. Это native-library/runtime
fixtures, а не физический экран/касания/звук или приёмка Android/Windows/macOS.
Новый APK и результаты физического устройства в этой точке не выдаются.
Все стадии остаются открытыми; см. [состояние](ROADMAP_STATUS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_dispatch_mouse` | function | [include/flashvm/mouse.h:25](../include/flashvm/mouse.h#L25) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
