# Платформенные адаптеры ввода и сети

Заголовки `ports/input_port.h` и `ports/network_port.h` совместимы с C89. Реализации находятся отдельно в `ports/windows`, `ports/linux`, `ports/mac`, `ports/android`. CMake выбирает нужный вариант через `FVM_ENABLE_OS_PORTS`. Ядро VM не зависит от ОС.

## Ввод

[`fvm_port_mouse_poll`](../ports/android/input.c#L11) читает реальное состояние кнопок мыши и, где возможно, координаты. Маска кнопок: 1 — левая, 2 — правая, 4 — средняя. Windows: `GetCursorPos` и `GetAsyncKeyState`; macOS: CoreGraphics; Linux: evdev (`FVM_INPUT_DEVICE=/dev/input/eventN`, требуются права чтения). В Linux координаты пока равны нулю: это **не** позиция курсора. На Android глобального курсора нет, поэтому функция возвращает 0. Для NativeActivity предусмотрена [`fvm_port_android_touch_poll`](../ports/android/input.c#L18) с `AInputQueue *`: извлекаются число пальцев, координаты первого касания и код действия. Очередь принадлежит приложению.

Распознавание тапов, мультитапов и свайпов выполняет хост; затем события передаются через [`fvm_input_dispatch`](../include/flashvm/input.h#L82) и существующие правила соответствия кнопкам мыши. Автоматический вызов обработчиков ActionScript ещё не реализован.

## Сеть

[`fvm_port_tcp_connect`](../ports/android/network.c#L58) создаёт TCP-соединение через системные сокеты и DNS: Winsock на Windows, POSIX/BSD на других системах. [`fvm_port_socket_send`](../ports/android/network.c#L100) и [`fvm_port_socket_recv`](../ports/android/network.c#L117) возвращают число байтов либо -1 при ошибке; 0 у `recv` означает закрытие соединения. [`fvm_port_socket_close`](../ports/android/network.c#L130) освобождает ресурсы. **Операции блокирующие**: вызывайте их из отдельного потока хоста. Пока нет TLS, таймаутов, HTTP, `XMLSocket`, `LoadVars` и проверки Flash sandbox. Не открывайте эти функции недоверенному SWF без политики доступа к сети. Для Android необходимо разрешение `INTERNET` в манифесте.

## Примеры

`examples/embedding/os_ports/main.c` — компилируемый пример на C89, `example.as` — демонстрация будущей интеграции, а не работающий сценарий. Многопоточные задания отрисовки пока не подключены к растеризации MovieClip.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_dispatch` | function | [include/flashvm/input.h:82](../include/flashvm/input.h#L82) |
| `fvm_port_android_touch_poll` | function | [ports/android/input.c:18](../ports/android/input.c#L18) |
| `fvm_port_mouse_poll` | function | [ports/android/input.c:11](../ports/android/input.c#L11) |
| `fvm_port_socket_close` | function | [ports/android/network.c:130](../ports/android/network.c#L130) |
| `fvm_port_socket_recv` | function | [ports/android/network.c:117](../ports/android/network.c#L117) |
| `fvm_port_socket_send` | function | [ports/android/network.c:100](../ports/android/network.c#L100) |
| `fvm_port_tcp_connect` | function | [ports/android/network.c:58](../ports/android/network.c#L58) |
<!-- source-links:end -->
