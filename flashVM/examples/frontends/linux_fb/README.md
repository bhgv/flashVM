# Direct Linux frontend / Фронтенд Linux без оконной системы

[main.c](main.c) and [device.h](device.h) are annotated C89 source using actual
Linux fbdev, evdev/mousedev and V4L2 system calls. No SDL, GTK, Qt or display server
is required. The shared FlashVM marker is copied to the top-left visible 320×200
rectangle of the existing framebuffer mode. No mode change, page flip or vsync
is requested. Optional camera capture supplies a grayscale background; the
red/green pointer marker is composited above it.

Русский: пример реально пишет кадр в framebuffer, получает положение/кнопку из
одного устройства ввода и по явной опции читает кадры камеры. Используются
настоящие ioctl/read/mmap/poll, а не заглушки. Пример не декодирует SWF-фильм,
не записывает видео и не реализует стандартный Flash Camera API.

## Build / Сборка

Linux headers, a C/C++ compiler and CMake are sufficient; graphics backend is
required for the common raster producer. From the component directory:

```sh
cmake -S . -B build-fb -DFVM_DEFS_FILE="$PWD/cmake/profiles/player-jit.defs" -DFVM_BUILD_EXAMPLES=ON -DFVM_EXAMPLE_LINUX_FB=ON
cmake --build build-fb --target flashvm_example_linux_fb flashvm_linux_device_layout -j2
ctest --test-dir build-fb -R '^flashvm_linux_device_layout$' --output-on-failure
```

The layout/decoder fixture uses synthetic native structures and buffers; it is
not a physical framebuffer, evdev or camera run. Default builds leave this
Linux-only target OFF; another target OS is a configure error.

## Choose actual device nodes / Выбор устройств

Run from a dedicated local console with access to framebuffer/input/video nodes.
Use the system's normal group/ACL policy for `video`, `input` and the active seat;
do not globally chmod devices. Inspect `ls -l /dev/fb* /dev/input/ /dev/video*`,
`cat /proc/bus/input/devices`, `fbset -i`, `evtest /dev/input/eventN`, and
`v4l2-ctl --device=/dev/video0 --list-formats-ext` (last tools are optional
system packages). Stop another display owner before writing the same framebuffer.
This tutorial does not acquire a VT/DRM master, switch consoles or restore old
pixels. Repaint your console after exit. Device paths and access vary by system.

```sh
./build-fb/examples/frontends/flashvm_example_linux_fb --fb /dev/fb0 --frames 120
./build-fb/examples/frontends/flashvm_example_linux_fb --fb /dev/fb0 --evdev /dev/input/event3
./build-fb/examples/frontends/flashvm_example_linux_fb --fb /dev/fb0 --mouse /dev/input/mouse0
./build-fb/examples/frontends/flashvm_example_linux_fb --fb /dev/fb0 --evdev /dev/input/event3 --video /dev/video0 --frames 120
```

`/dev/input/mouse` is not a universal node name. If your system provides that
path/symlink as a mousedev three-byte stream, pass `--mouse /dev/input/mouse`.
Usual nodes are `/dev/input/mouse0` or aggregate `/dev/input/mice`; neither is
an evdev record stream. Choose only one of --evdev/--mouse per run, otherwise
usage fails before opening devices. Never feed /dev/input/eventN to --mouse.

Русский: номер N нужно выбрать по своей системе. Поддерживается именно
существующий узел устройства, пример не создаёт `/dev/fb0`, `/dev/input/*`
или `/dev/videoN`. На современном DRM-only Linux fbdev может отсутствовать;
на обычном Android доступ к этим Linux-узлам часто не предоставляется приложению.
Это пример для Linux-консоли, не APK и не обход системных прав.

## Options and result / Параметры и результат

| Option | Meaning / Значение |
| --- | --- |
| --fb PATH | Existing framebuffer, default /dev/fb0, read/write mapping; fixed 320×200 top-left visible region only. |
| --evdev PATH | One nonblocking legacy relative or ABS_X/ABS_Y pointer source; BTN_LEFT/BTN_TOUCH accepted, other keys ignored. |
| --mouse PATH | One nonblocking default three-byte PS/2 mousedev stream; left button, signed X/Y, partial packet preservation. No wheel negotiation. |
| --video PATH | Explicit camera capture: single-plane MMAP progressive YUYV only, requested 320×200, negotiated size capped at 1920×1080. |
| --frames N | Positive bounded presentation count; default runs until SIGINT/SIGTERM. Count is writes, not physical scanout or camera frames. |

Expected finite output: `linux_fb frames=120 camera_frames=N wheels=W last_delta=D status=0`.
A camera can produce zero frames before a very short presentation run ends;
inspect the independent camera_frames count rather than claiming capture from
presentation success. Disconnect, permission, unsupported layout/format, malformed
frame and native/VM errors exit nonzero. SIGINT/SIGTERM request normal cleanup.

## Data flow and limits / Поток данных и ограничения

Framebuffer supports little-endian packed TRUECOLOR 16/24/32 bpp with ordinary
nonoverlapping RGB fields of 1..8 bits, optional alpha. It rejects DIRECTCOLOR,
indexed/planar/nonstandard/grayscale/reversed-bit formats. Checked virtual offsets,
stride and final-row byte span must fit smem_len before any write. RGB values are
quantized to reported fields (including RGB565); padding/other screen area remain
untouched. Reconfigure/restart if another owner changes display mode externally.

Evdev accumulates REL_X/Y and calibrated ABS_X/Y, forwards button records in their
native order at the position known at that record, and publishes accumulated motion
at SYN_REPORT. Initial held-at-open button state is not synthesized. Pointer is
clamped to the tutorial rectangle. SYN_DROPPED cancels any hold, discards pending
wheel/unpublished relative motion, ignores through the next SYN_REPORT, then
queries current keys and (for legacy absolute devices) both axes. A held snapshot
begins a new contact; a released snapshot publishes a move at the recovered position.
Query/calibration failures stop with an error rather than guessing state. It does not decode ABS_MT slots, keyboard layout or multitouch.
Mousedev reassembles three-byte packets, honors header sign bits and Y inversion;
overflow stops rather than inventing movement. Both paths bound work per tick.

V4L2 uses QUERYCAP → S_FMT → REQBUFS → QUERYBUF/mmap/QBUF → STREAMON;
poll → DQBUF → validate/copy → QBUF repeats on the VM owner. Preview copies only
Y bytes using actual row stride, nearest-neighbor scaled into independent native
storage; raw Y range is preserved, so this is explicitly grayscale, not RGB/YUV
colorspace-correct video. MJPEG, multi-plane and interlaced formats fail explicitly.
Every valid acquired slot is returned even when its frame is rejected. STREAMOFF
precedes munmap/close. No V4L2 pointer survives its dequeue scope. Shutdown errors
from best-effort munmap/close/STREAMOFF are not separately surfaced after an earlier
failure; normal device-loss diagnostics remain visible.

Русский: формат, размер и шаг строки берутся из ответа драйвера. Память камеры
не передаётся VM и не сохраняется после возврата буфера драйверу. Копия яркости
используется как фон; корректное цветное преобразование не заявляется. Потеря
синхронизации ввода завершает пример, мультитач не подменяется одной мышью.
Отсутствие реальных устройств в CI не считается успешной проверкой оборудования.

## FlashVM API, ownership / API и владение

The shared [common/demo.h](../common/demo.h) creates a separate runtime/context,
allocates a 320×200 opaque RGBA surface, dispatches Mouse coordinates and renders
a green/red pointer marker. See the [individual FlashVM API table](../README.md#public-flashvm-api-used-by-the-shared-host)
for every call, its parameters, status, ownership and declaration. The frontend
owns the native event loop, window/device and frame storage. All VM calls and
presentation callbacks run on one owner thread. Frames and userdata are borrowed
through callback return only; no managed script pointer escapes, so no GC pin is
needed. Stop callbacks before releasing context, runtime and native buffers.

Русский: [общий README](../README.md) подробно описывает каждую функцию FlashVM,
параметры и ошибки. Это исполняемый пример встраивания с видимым маркером,
не полный SWF-проигрыватель. Координаты — пиксели мира 320×200. Ошибка VM или
вывода останавливает пример с ненулевым кодом. Ни один чужой поток не входит в
VM. Нативные объекты не являются корнями GC. Оконные SDK остаются внешними
зависимостями; системная shared-сборка примера не является статическим релизом.

## Native API / Нативный API

| API | Parameters, behavior and ownership / Параметры и действие |
| --- | --- |
| [open](https://man7.org/linux/man-pages/man2/open.2.html) | path, O_RDWR or O_RDONLY|O_NONBLOCK: explicit node ownership; -1 is failure. Camera is never opened unless --video is supplied. |
| [close](https://man7.org/linux/man-pages/man2/close.2.html) | owned fd: release after streaming/mappings stop, including partially initialized owners. |
| [ioctl](https://man7.org/linux/man-pages/man2/ioctl.2.html) | fd, named request, writable native structure: device-specific ABI; negative status is real error; only video_ioctl retries EINTR. |
| [mmap](https://man7.org/linux/man-pages/man2/mmap.2.html) | NULL address, exact driver length, READ|WRITE, MAP_SHARED, fd, driver offset: acquired native mapping; MAP_FAILED is not NULL and is handled explicitly. |
| [munmap](https://man7.org/linux/man-pages/man2/munmap.2.html) | mapped address and exact recorded length: release after STREAMOFF, before descriptor close. |
| [read](https://man7.org/linux/man-pages/man2/read.2.html) | input fd, record/byte output, ABI-specific size: nonblocking; EAGAIN/EINTR defer; EOF or short evdev record fails. |
| [poll](https://man7.org/linux/man-pages/man2/poll.2.html) | two pollfd slots, count=2, timeout=16ms: wait for pointer/camera; negative fd ignored; HUP/ERR/NVAL stop; per-tick input work bounded. |
| [sigaction](https://man7.org/linux/man-pages/man2/sigaction.2.html) | SIGINT/SIGTERM, initialized action, NULL old: handler sets only volatile sig_atomic_t stop flag; cleanup stays on main thread. |
| [sigemptyset](https://man7.org/linux/man-pages/man3/sigemptyset.3.html) | action mask output: initialize empty blocked-signal set before installing handler. |
| [strtoul](https://man7.org/linux/man-pages/man3/strtoul.3.html) | decimal text,end pointer,base=10: reject negative/zero/suffix/ERANGE for --frames. |
| [FBIOGET_FSCREENINFO](https://docs.kernel.org/fb/api.html) | fd and fb_fix_screeninfo output: obtain real memory length, line stride, packed/visual type before mapping. |
| [FBIOGET_VSCREENINFO](https://docs.kernel.org/fb/api.html) | fd and fb_var_screeninfo output: obtain bpp, virtual/visible sizes, offsets and component fields. No mode-setting ioctl is issued. |
| [EVIOCGVERSION](https://docs.kernel.org/input/input.html) | fd and int version output: reject an ordinary file/non-evdev node selected with --evdev. |
| [EVIOCGABS](https://docs.kernel.org/input/event-codes.html) | ABS_X or ABS_Y request, input_absinfo output: require both nonempty ranges for calibrated legacy absolute pointer; optional for relative mice. |
| [VIDIOC_QUERYCAP](https://docs.kernel.org/userspace-api/media/v4l/vidioc-querycap.html) | v4l2_capability output: use device_caps when DEVICE_CAPS advertised; require single-plane capture plus STREAMING. |
| [VIDIOC_S_FMT](https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-fmt.html) | v4l2_format in/out: request 320x200 YUYV progressive; validate actual returned format/dimensions/stride, no assumption exact request is honored. |
| [VIDIOC_REQBUFS](https://docs.kernel.org/userspace-api/media/v4l/vidioc-reqbufs.html) | capture type, MMAP memory, requested count=2: accept returned count 1..8; descriptor close frees driver allocation on failure. |
| [VIDIOC_QUERYBUF](https://docs.kernel.org/userspace-api/media/v4l/vidioc-querybuf.html) | capture/MMAP/index in, length/m.offset out: map only returned buffer region; retain exact span. |
| [VIDIOC_QBUF](https://docs.kernel.org/userspace-api/media/v4l/vidioc-qbuf.html) | capture/MMAP slot: hand ownership to driver after initial mapping and after every valid dequeue, including rejected/error frames. |
| [VIDIOC_DQBUF](https://docs.kernel.org/userspace-api/media/v4l/vidioc-qbuf.html) | capture/MMAP result: temporarily borrow returned slot; EAGAIN means not ready; validate index/bytesused/flags before copying. |
| [VIDIOC_STREAMON](https://docs.kernel.org/userspace-api/media/v4l/vidioc-streamon.html) | pointer to capture buffer type: start only after queueing all mappings; remember successful acquisition. |
| [VIDIOC_STREAMOFF](https://docs.kernel.org/userspace-api/media/v4l/vidioc-streamon.html) | same capture type: stop streaming before munmap/close, including error cleanup. |

## Local helpers / Локальные функции

`fb_layout_valid(fixed,mode)` performs address/field validation without I/O;
`fb_channel(value,field)` quantizes a byte; fb_open/fb_close own fd/map.
`video_ioctl` retries only EINTR; video_open negotiates bounded MMAP acquisition;
`video_copy(video,data,length)` checks the accessible span and copies luminance;
video_step dequeues/requeues at most one slot; video_close stops/unmaps/closes.
`linux_present(frame,host)` borrows the validated frame and writes packed pixels;
pointer_open selects one ABI, pointer_read bounds read work, evdev_event and
mouse_byte decode their own protocols, pointer_send clamps/publishes coordinates.
request_stop only sets a signal flag; frame_limit validates decimal input; main
parses options, owns the loop, reports errors and performs all cleanup.

## evdev wheel / Колесо evdev (v0.607)

pointer_open queries relative-axis capabilities. If REL_WHEEL_HI_RES is advertised,
its high-resolution counts are authoritative (120 counts/detent) and REL_WHEEL
compatibility events are ignored even in separate reports. Otherwise legacy
REL_WHEEL detents are used; the decoder also prefers explicit high-res data found
in a report. Accumulate until SYN_REPORT, publish the completed pointer position,
then deliver nonzero vertical lines using 3 lines/detent. Reset packet accumulators
before callbacks. REL_HWHEEL remains unsupported; three-byte mousedev does not
acquire wheel bytes or negotiate IntelliMouse mode. SYN_DROPPED discards pending
wheel before the explicit snapshot recovery described below. No native worker or VM keyboard policy is introduced.

| API/code | Parameters and behavior |
| --- | --- |
| [EVIOCGBIT](https://docs.kernel.org/input/event-codes.html) | EV_REL, exact writable byte-mask span: query supported relative axes on selected evdev node; setup fails on query error instead of guessing the active wheel protocol. |
| [REL_WHEEL / REL_WHEEL_HI_RES](https://docs.kernel.org/input/event-codes.html#ev-rel) | Scalar relative records: detents vs high-resolution counts, not pixel distances; select one representation to avoid duplicates. |
| [demo_wheel](../common/wheel.h) | Demo, identity affine, clamped completed pointer, finite normalized line delta: real VM delivery and first-error latch; no button transition. [Contract](../../../doc/MOUSE.md#wheel-events). |

Русский: режим high-res выбирается по возможностям устройства. Legacy-копии не
считаются второй прокруткой, даже если пришли в отдельном SYN_REPORT. Колесо
доставляется только после полного пакета; потерянный пакет прекращает пример.
Тест использует искусственные input_event и проверяет дробь, выбор high-res,
отдельные legacy-пакеты и отмену — физический evdev этим не подтверждается.

## v0.612 evdev loss recovery / Восстановление evdev

The decoder follows the kernel [SYN_DROPPED contract](https://docs.kernel.org/input/event-codes.html#ev-syn):
ignore through the next report, query EVIOCGKEY, and query EVIOCGABS(ABS_X/Y)
when using calibrated absolute axes. Relative/wheel displacement lost to overflow
cannot be recovered. The last accepted VM position is retained for relative input;
absolute calibration/value is validated and copied before VM publication. Repeated
loss markers do not query early. ioctl EINTR is retried; disconnect/invalid ranges
fail closed. Sequential ioctl snapshots are not a hardware-atomic transaction.

Русский: при переполнении очереди адаптер завершает прежнее удержание в последней
принятой позиции и отбрасывает неполный пакет. До следующего SYN_REPORT все
записи игнорируются. Затем EVIOCGKEY восстанавливает текущее состояние кнопок;
для абсолютного указателя EVIOCGABS получает обе координаты и калибровку. Новое
удержание начинается только из успешного снимка; потерянные относительные движения
и колесо не выдумываются. Ошибка любого запроса или нулевой диапазон завершают
пример без частичного обновления VM. Это не multitouch-протокол и не физическая
приёмка устройства.

| API | Parameters / Параметры |
| --- | --- |
| `EVIOCGKEY(bytes)` | fd is the explicit evdev descriptor; output is a zeroed bounded bitmap of keys/buttons / fd — выбранный evdev; выход — ограниченная битовая карта |
| `EVIOCGABS(ABS_X/ABS_Y)` | output is input_absinfo with current value/min/max; both validated before use / input_absinfo содержит значение и диапазон, проверяются обе оси |
| `pointer_resync(pointer, demo)` | internal owner-thread adapter, consumes copied device state and publishes one VM transition / внутренний адаптер, без сохранения kernel pointers |

Device-free tests: `flashvm_evdev_recovery` and `flashvm_linux_device_layout`.

## v0.614 primary-source reconciliation / Согласование кнопок

The decoder stores BTN_LEFT and BTN_TOUCH separately. Their logical OR is the
single VM primary hold. Only aggregate edges dispatch Mouse down/up: repeated
press/release records and autorepeat value 2 do not duplicate callbacks, and
releasing one source does not release a still-held other source. Keyboard and
secondary-button records remain ignored. Recovery snapshots populate the same
two bits before resuming input, so an unrelated release cannot end a recovered
contact. This is a one-primary tutorial policy, not general multitouch or pointer IDs.

Русский: BTN_LEFT и BTN_TOUCH больше не перезаписывают друг друга. Удержание VM
сохраняется, пока хотя бы один источник нажат; down/up отправляются только при
изменении объединённого состояния. Снимки восстановления обновляют эти же два
признака. Повторы и value=2 не создают новых событий. Не добавляется отдельный
поток клавиатуры или поддержка нескольких указателей.

`flashvm_evdev_buttons` verifies actual public Mouse listener counts for both
source orders, duplicates, autorepeat and unrelated keys/buttons.
`flashvm_evdev_recovery` also checks release after a recovered held touch.
