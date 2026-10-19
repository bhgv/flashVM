# Независимые хосты отображения и событий

С v0.598 заголовок `flashvm/host.h` задаёт C89-контракт без типов SDL,
Clutter, Cogl, GLES и оконной системы. VM рисует в принадлежащий хосту
[`FVM_RasterSurface`](../include/flashvm/raster.h#L16); хост выбирает цикл событий, загрузку текстуры, расписание
и правила клавиатуры/контактов. SDL — один из адаптеров.

## Передача кадров

`fvm_host_present(surface, byte_count, callback, userdata)` проверяет указатели,
ненулевые размеры, ширину RGBA8-строки, stride, переполнение адресной арифметики
и доступный объём до вызова адаптера. Хозяин буфера передаёт реальное число
доступных байтов от pixels: произвольный указатель функция проверить не может.
Межстрочный padding допустим; padding после последней строки необязателен.
[`FVM_HOST_RGBA_BYTES`](../include/flashvm/host.h#L8) равен четырём. Порядок каналов RGBA, alpha прямая,
начало сверху слева, stride в байтах. BGRA, premultiplied alpha и переворот
строк при необходимости преобразует адаптер.

[`FVM_HostPresentCallback`](../include/flashvm/host.h#L11) получает копию описателя и заимствованные пиксели
синхронно в вызывающем потоке. До возврата нужно загрузить/скопировать кадр;
для асинхронной отправки нужна собственная копия. Вызывающий сохраняет буфер
и описатель живыми и неизменными до возврата. Владение не передаётся, API
ничего не удерживает; userdata, включая NULL, передаётся без изменений.
Текстура, графический контекст и окно принадлежат хосту.

Ошибка проверки: [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45), callback не вызывается. При успехе
проверки callback вызывается ровно один раз, его статус возвращается как есть,
включая [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46). Исключение C++ std::bad_alloc превращается в
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44), остальные — в [`FVM_ERROR`](../include/flashvm/flashvm.h#L43). Аппаратные ошибки, longjmp и
висячие указатели контрактом не покрываются. Контекст VM и GC не используются;
ограничения потока/графического контекста адаптера сохраняются. SDL дополнительно
проверяет совпадение размеров с текстурой и допустимый signed pitch.

## Координаты событий

`fvm_host_pointer_to_world(world_to_host, host_x, host_y, world_x, world_y)`
обращает [`FVM_Affine`](../include/flashvm/path.h#L33), переводя событие в мировые пиксели. Передавайте матрицу
из мира SWF в пространство реального события, учитывая начало сцены,
преобразование actor, letterbox и масштаб устройства ровно один раз.
При масштабе 2 и переносе (100,50) событие (120,90) даёт мир (10,20).
Поддержаны поворот, shear и отражение; внешние/отрицательные координаты
не обрезаются, hit testing здесь не выполняется.

Аргументы и коэффициенты должны быть конечными, матрица невырожденной.
NULL, совпадающие выходные указатели, непредставимая арифметика и неконечный
результат дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45), сохраняя оба выхода. Нормализация
коэффициентов позволяет обрабатывать большие равномерные масштабы;
численно схлопнувшийся определитель отклоняется. Это host double, а не
эмуляция SWF binary32. Нет выделений, VM, callback, dispatch или GC;
независимые данные можно обрабатывать независимо в разных потоках.
Совместно изменять одни аргументы одновременно нельзя.

Результат передавайте в [`fvm_input_dispatch_mouse`](../include/flashvm/mouse.h#L29) на потоке владельца VM,
полные снимки клавиатуры — в [`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L90). Выбор первого
контакта, отмена, подавление синтетических событий, потеря фокуса и порядок
остаются обязанностью хоста. Из рабочего потока ввода копируйте события,
затем доставляйте их владельцем VM. SDL уже переводит события в логическую
сцену: повторное применение масштаба окна ошибочно. Его смешанная очередь
сохраняется; автоматический выбор Flash-фокуса не добавлен.

## Выбранный хост Clutter/Cogl (v0.600)

Изучен [shell revision 2a616b2](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/tree/2a616b264f0045a3ba00ce77aa1930ca905956ea)
вне `inferno-os`. В [app/jni/clutter_main.c](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/app/jni/clutter_main.c)
есть загрузка RGBA через clutter_texture_set_from_rgb_data и события с
clutter_event_get_coords. Отдельный хост сможет передавать кадры FlashVM
через эту границу и обращать координаты stage/actor указанным API.
Жизненный цикл GLES/actor остаётся в оболочке. Существующий xmouse_btn
относится к другому клиенту и не является точкой подключения VM.

С v0.600 [пример Clutter3D](../examples/frontends/clutter3d/README.md) использует
именно исходники Clutter/Cogl указанного Git через явный
[SDK](../external/clutter3d/README.md). Экспорт физически исключает inferno-os.
После двух исправлений configure обе библиотеки собраны статически; Linux-пример
передаёт кадры и проверяет координаты actor под Xvfb/Mesa. Это учебный host,
не полный player и не приёмка Android/GLES на устройстве. С v0.601 проверен также Linux EGL/GLES2 и исправлена потеря дробных координат
в actor inverse. Android surface lifecycle, прямой GPU-рендер сцены, zero-copy, полный multitouch и IME остаются открытыми,
как и все девять стадий.

## Проверки

flashvm_tests_host проверяет padding/последнюю строку, короткие буферы,
переполнение, ошибки/исключения callback, вырожденные/неконечные матрицы,
поворот, shear и большие масштабы. flashvm_frontend_window проверяет
реальный SDL-адаптер: отказ до загрузки и точные цвета. Frontend — C89,
реализация — C++98. Выполненные прогоны указаны в отчёте стадий.
## Исполняемый пример v0.599

[Хост C89 без SDL](../examples/frontends/headless/README.md) реально рисует
квадрат, проверяет/передаёт RGBA-кадр, обращает масштаб и перенос события,
доставляет down/up в context. Смешанная SDL-очередь теперь тоже применяет
[fvm_host_pointer_to_world](../include/flashvm/host.h#L26) к каждому сохранённому событию; после уже выполненной
SDL-нормализации остаётся только восстановить начало сцены. Порядок, ошибки
и сброс фокуса сохраняются. При отсутствии графики пример явно пропускается.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Affine` | struct | [include/flashvm/path.h:33](../include/flashvm/path.h#L33) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_host_pointer_to_world` | function | [include/flashvm/host.h:26](../include/flashvm/host.h#L26) |
| `fvm_host_present` | function | [include/flashvm/host.h:18](../include/flashvm/host.h#L18) |
| `FVM_HOST_RGBA_BYTES` | macro / define | [include/flashvm/host.h:8](../include/flashvm/host.h#L8) |
| `FVM_HostPresentCallback` | callback typedef | [include/flashvm/host.h:11](../include/flashvm/host.h#L11) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:90](../include/flashvm/keyboard.h#L90) |
| `fvm_input_dispatch_mouse` | function | [include/flashvm/mouse.h:29](../include/flashvm/mouse.h#L29) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_RasterSurface` | struct | [include/flashvm/raster.h:16](../include/flashvm/raster.h#L16) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
