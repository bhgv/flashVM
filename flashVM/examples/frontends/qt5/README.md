# Qt5 frontend / Фронтенд Qt5

[main.cpp](main.cpp) is an executable QWidget/QImage frontend with no generated
moc files. Every source line explains the native/VM operation. A scoped QPainter
consumes a borrowed RGBA8888 image. Mouse primary transitions use fractional
logical widget positions; painting and input use the same resize transform.
Qt handles device pixel ratio, so the host does not apply it twice.
WindowDeactivate synthesizes UP; secondary buttons, touch, keyboard and IME
are not translated in this pointer-focused tutorial.

Русский: реальное окно Qt5, зелёный/красный квадрат под указателем. Изменение
размера масштабирует изображение и обратное преобразование ввода согласованно.
При деактивации окна нажатие снимается. Нативный таймер только запрашивает
перерисовку; никакой поток Qt не вызывает VM параллельно.

## Build and run / Сборка и запуск

Install `qtbase5-dev` (Qt5 >=5.5), CMake and compiler; from component directory:

```sh
cmake -S . -B build-qt5 -DFVM_DEFS_FILE="$PWD/cmake/profiles/player-jit.defs" -DFVM_BUILD_EXAMPLES=ON -DFVM_EXAMPLE_QT5=ON
cmake --build build-qt5 --target flashvm_example_qt5 -j2
./build-qt5/examples/frontends/flashvm_example_qt5
QT_QPA_PLATFORM=offscreen ./build-qt5/examples/frontends/flashvm_example_qt5 --smoke
```

Expected smoke: `Qt5 frames=120 wheels=0 last_delta=0.000 status=0`. Offscreen checks actual Qt software
painting, not a native display or physical input. Only this SDK host target uses
C++11, required by current Qt5 headers; the VM remains C++98 and its embedding
ABI C89. No Qt type enters public FlashVM headers. Supply `CMAKE_PREFIX_PATH`
for a separate SDK; platform plugins and their dependencies must be deployed.
Windows/macOS/Android Qt packaging and native execution remain unverified.

## Host methods / Методы хоста

`DemoWidget` owns Demo, timer ID and smoke flag; painter pointer is borrowed
only within paintEvent. `present(frame,this)` returns real consumer status;
`pointer(event,type)` converts logical coordinates once; paintEvent creates and
ends QPainter; timerEvent filters its timer ID; mouse methods accept LeftButton;
event() cancels on deactivation. Copy/assignment are prohibited. Main destroys
the stack widget after exec returns, before QApplication teardown. No lambda,
worker, retained QImage or script-managed object is used.

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
| [QApplication](https://doc.qt.io/archives/qt-5.15/qapplication.html) | argc by reference and argv: GUI application owner, created before and destroyed after all widgets; exec() runs owner loop. |
| [QWidget](https://doc.qt.io/archives/qt-5.15/qwidget.html) | DemoWidget: native widget owns timers/window; resize(w,h) sets logical extent, rect()/width()/height() describe that same extent. |
| [QWidget::show](https://doc.qt.io/archives/qt-5.15/qwidget.html#show) | No arguments: make initialized widget visible and schedule native paint. |
| [QWidget::close](https://doc.qt.io/archives/qt-5.15/qwidget.html#close) | No arguments: end window visibility; default last-window behavior exits app loop; stack owner is not deleted. |
| [QWidget::update](https://doc.qt.io/archives/qt-5.15/qwidget.html#update) | No arguments: coalesced asynchronous repaint request; no VM callback occurs on another thread. |
| [QWidget::setMouseTracking](https://doc.qt.io/archives/qt-5.15/qwidget.html#mouseTracking-prop) | true: deliver mouse motion even with no held button. |
| [QWidget::startTimer](https://doc.qt.io/archives/qt-5.15/qobject.html#startTimer) | interval ms: start owner-thread periodic callback; nonzero timer ID must be retained, zero is failure. |
| [QWidget::killTimer](https://doc.qt.io/archives/qt-5.15/qobject.html#killTimer) | timer ID: stop native delivery before freeing Demo. |
| [QWidget::event](https://doc.qt.io/archives/qt-5.15/qwidget.html#event) | event pointer borrowed: after WindowDeactivate cancellation, delegate remaining processing to native base. |
| [QPainter](https://doc.qt.io/archives/qt-5.15/qpainter.html) | this widget: begin scoped paint inside paintEvent; isActive() must succeed; destructor ends native paint. |
| [QPainter::drawImage](https://doc.qt.io/archives/qt-5.15/qpainter.html#drawImage) | target rect, borrowed QImage: scale opaque RGBA to logical widget pixels synchronously, retain no producer storage. |
| [QImage](https://doc.qt.io/archives/qt-5.15/qimage.html) | pixels,width,height,stride,Format_RGBA8888: temporary non-owning byte-ordered image; isNull() signals unusable descriptor; bytes remain valid throughout painter call. |
| [QMouseEvent::localPos](https://doc.qt.io/archives/qt-5.15/qmouseevent.html#localPos) | borrowed event: fractional logical widget QPointF; x()/y() are inverse-scaled once to world coordinates. |
| [QTimerEvent::timerId](https://doc.qt.io/archives/qt-5.15/qtimerevent.html#timerId) | borrowed event: filter against this owner's acquired timer ID. |
| [QEvent::type](https://doc.qt.io/archives/qt-5.15/qevent.html#type) | borrowed event: detect WindowDeactivate and finish stale primary contact at last valid world position. |
| [QApplication::exec](https://doc.qt.io/archives/qt-5.15/qapplication.html#exec) | No arguments: process SDK events on GUI thread; integer exit code remains visible to main. |

## SDK fixture / Проверка адаптера

Build `flashvm_example_qt5_input` and run
`ctest --test-dir build-qt5 -R '^flashvm_example_qt5_input$' --output-on-failure`.
The registered test explicitly uses QT_QPA_PLATFORM=offscreen. Real Qt event
objects pass through QApplication::sendEvent; QWidget::grab reads actual painted
pixels to check released/held marker colors after fractional scaled motion,
button press and WindowDeactivate. `--smoke` separately runs the timer/main loop.
Русский: используются настоящий SDK и программный плагин Qt, без заявления
приёмки физического экрана/устройства ввода. Qt native event injection APIs are
[test fixture APIs](https://doc.qt.io/archives/qt-5.15/qcoreapplication.html#sendEvent),
not extra FlashVM APIs; [grab](https://doc.qt.io/archives/qt-5.15/qwidget.html#grab)
returns an owned snapshot, never the borrowed Demo frame.

## Vertical wheel / Вертикальная прокрутка (v0.607)

wheelEvent(event) consumes nonzero angleDelta.y: lines = y/120 * 3, preserving
fractional partial detents and Qt's delivered sign. No extra inverted() sign flip
is applied. Zero-angle phase, horizontal-only and pixel-only streams call ignore()
and do not enter VM. Qt's event-time fractional position uses position() on SDK
>=5.14 and posF() on earlier supported headers, then the painting scale is inverted
once. This explicit angle-to-line policy does not claim pixel scrolling or OS
scroll-preference matching. Diagnostics expose successful wheel_events and last
line delta. The real offscreen fixture verifies a half-detent = 1.5 lines, ignored
pixel-only input, and marker position after wheel delivery. Physical devices and
older/foreign SDK runtime acceptance remain open.

| API | Parameters and behavior |
| --- | --- |
| [QWheelEvent::angleDelta](https://doc.qt.io/archives/qt-5.15/qwheelevent.html#angleDelta) | Borrowed event, no arguments: copy vertical angle units (120 per usual detent); horizontal-only remains unhandled. |
| [QWheelEvent::position](https://doc.qt.io/archives/qt-5.15/qwheelevent.html#position) | No arguments: event-time logical QPointF on SDK >=5.14; x/y inverse-scaled once. |
| [QWheelEvent::posF](https://doc.qt.io/archives/qt-5.15/qwheelevent-obsolete.html#posF) | Older Qt5 fractional position accessor; used only when position() is unavailable. |
| [QEvent::accept](https://doc.qt.io/archives/qt-5.15/qevent.html#accept) | No arguments: mark recognized vertical event consumed, no ownership transfer. |
| [QEvent::ignore](https://doc.qt.io/archives/qt-5.15/qevent.html#ignore) | No arguments: let Qt propagate unsupported pixel/horizontal/zero stream; no VM wheel callback. |
| [demo_wheel](../common/wheel.h) | Demo, world→widget affine, event x/y, line delta: synchronous rooted VM dispatch and first-error latch; no held-button mutation. [Contract](../../../doc/MOUSE.md#wheel-events). |

Русский: полщелчка колеса даёт 1.5 строки; пиксельную дельту пример не выдаёт за
строки. Позиция берётся из самого события, без дополнительного масштаба DPI.
Конструкторы QWheelEvent в тесте выбраны по версии SDK (современный с 5.12);
ядро FlashVM по-прежнему не зависит от Qt. Печатный результат содержит wheels
и last_delta, ноль означает отсутствие успешной доставки, а не ошибку SDK.
