# GTK3 frontend / Фронтенд GTK3

[main.c](main.c) is a C89 executable using GtkDrawingArea, GdkPixbuf and Cairo.
It presents the actual FlashVM RGBA marker frame, forwards primary mouse motion,
press/release, and cancels a hold on native focus loss. Resize scales painting
and pointer coordinates consistently in GTK logical pixels (do not multiply by
monitor scale again). Touch/keyboard/IME and movie/audio clocks are not implemented.

Русский: окно показывает зелёный квадрат, при нажатии — красный. Двигайте мышь,
изменяйте размер окна; маркер остаётся под указателем. Уход фокуса снимает
нажатие в последней позиции. `--smoke` завершает 120 успешных рисований.
Все строки исходника снабжены учебными комментариями.

## Build and run / Сборка и запуск

On Debian/Ubuntu install `libgtk-3-dev pkg-config` and a C/C++ compiler, CMake,
then from the FlashVM component directory:

```sh
cmake -S . -B build-gtk3 -DFVM_DEFS_FILE="$PWD/cmake/profiles/player-jit.defs" -DFVM_BUILD_EXAMPLES=ON -DFVM_EXAMPLE_GTK3=ON
cmake --build build-gtk3 --target flashvm_example_gtk3 -j2
./build-gtk3/examples/frontends/flashvm_example_gtk3
./build-gtk3/examples/frontends/flashvm_example_gtk3 --smoke
```

GTK3 >=3.22 and a real X11/Wayland display are required. For software CI use
`xvfb-run -a ... --smoke`; it checks SDK painting, not physical input/hardware.
Expected final output: `GTK3 frames=120 wheels=0 last_delta=0.000 status=0`. Native close exits normally
with the actual frame count. Missing display or initialization error is nonzero.
Linux is the exercised SDK target; foreign GTK3 ports/Android are not accepted.

## Native callbacks and lifetime / Обработчики

`GtkHost.demo` owns VM/frame, `area` borrows child, `cr` borrows only the active
paint callback, `smoke` selects finite run. `gtk_draw(widget,cr,host)` runs the
shared raster producer and sink; `gtk_pointer(widget,event,host)` copies the
recognized GDK variant and inverse scale; `gtk_focus` synthesizes UP;
`gtk_tick(host)` requests repaint; `gtk_closed` quits the loop without freeing VM.
Stop timer, destroy widgets, release retained native references, then close Demo.
Only opaque frames are used: generic straight-alpha images would need explicit
premultiplication when converting to Cairo; do not copy this assumption blindly.

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

Each row names individual arguments and the ownership/behavior used here.

| API | Parameters, behavior and ownership / Параметры и действие |
| --- | --- |
| [gtk_init_check](https://docs.gtk.org/gtk3/func.init_check.html) | NULL argc/argv: initialize GTK without consuming tutorial options; FALSE if no usable display; no successful headless fallback. |
| [gtk_window_new](https://docs.gtk.org/gtk3/ctor.Window.new.html) | GTK_WINDOW_TOPLEVEL: create native top-level widget, floating ownership is explicitly sunk. |
| [gtk_drawing_area_new](https://docs.gtk.org/gtk3/ctor.DrawingArea.new.html) | No arguments: create paint/input child; window owns it after container_add. |
| [gtk_widget_set_size_request](https://docs.gtk.org/gtk3/method.Widget.set_size_request.html) | area, minimum width/height in logical pixels: keep a usable drawing rectangle; not physical DPI pixels. |
| [gtk_widget_add_events](https://docs.gtk.org/gtk3/method.Widget.add_events.html) | area, GDK event mask: request motion, press and release delivery; does not itself dispatch VM events. |
| [gtk_container_add](https://docs.gtk.org/gtk3/method.Container.add.html) | window container, area: parent and acquire child; child survives until parent destruction. |
| [gtk_widget_get_allocated_width](https://docs.gtk.org/gtk3/method.Widget.get_allocated_width.html) | area: query current logical width; divide by world width to construct the forward input/paint scale. |
| [gtk_widget_get_allocated_height](https://docs.gtk.org/gtk3/method.Widget.get_allocated_height.html) | area: current logical height; zero extent produces a rejected singular input transform. |
| [gtk_widget_queue_draw](https://docs.gtk.org/gtk3/method.Widget.queue_draw.html) | area: request asynchronous repaint; draw callbacks may be coalesced; never retain cairo_t. |
| [gtk_widget_show_all](https://docs.gtk.org/gtk3/method.Widget.show_all.html) | window: realize/show parent and child before native loop starts painting. |
| [gtk_main](https://docs.gtk.org/gtk3/func.main.html) | No arguments: block in GTK main context; all timer/input/paint callbacks run on this owner thread. |
| [gtk_main_level](https://docs.gtk.org/gtk3/func.main_level.html) | No arguments: query active GTK loop nesting; guard destroy-signal quit when cleanup runs after loop exit. |
| [gtk_main_quit](https://docs.gtk.org/gtk3/func.main_quit.html) | No arguments: request exit from active GTK loop; actual cleanup happens after callback returns. |
| [gtk_widget_destroy](https://docs.gtk.org/gtk3/method.Widget.destroy.html) | window: destroy native widget/children; explicit retained reference keeps object safe through cleanup. |
| [g_object_ref_sink](https://docs.gtk.org/gobject/method.Object.ref_sink.html) | window: consume floating reference or acquire strong native reference; pair with g_object_unref. |
| [g_object_unref](https://docs.gtk.org/gobject/method.Object.unref.html) | image/window: release only owned native references, not Demo pixel memory. |
| [g_signal_connect](https://docs.gtk.org/gobject/func.signal_connect.html) | sender, signal name, G_CALLBACK(function), userdata: connect owner-thread callback; userdata borrowed until sender destruction. |
| [g_timeout_add](https://docs.gtk.org/glib/func.timeout_add.html) | interval in ms, gtk_tick, host: attach repeated native source; returns source ID retained for removal. |
| [g_source_remove](https://docs.gtk.org/glib/type_func.Source.remove.html) | timer ID: stop callbacks after gtk_main returns and before stack host cleanup. |
| [gdk_pixbuf_new_from_data](https://docs.gtk.org/gdk-pixbuf/ctor.Pixbuf.new_from_data.html) | borrowed pixels, RGB colorspace, alpha TRUE, 8 bits, width/height, byte rowstride, NULL destroy/userdata: descriptor never frees Demo storage. |
| [gdk_cairo_set_source_pixbuf](https://docs.gtk.org/gdk3/func.cairo_set_source_pixbuf.html) | borrowed Cairo context, image, x=0,y=0: select image source in scaled logical space. |
| [cairo_save](https://www.cairographics.org/manual/cairo-cairo-t.html#cairo-save) | cr: save source/clip/transform so no pixel source survives after restore. |
| [cairo_scale](https://www.cairographics.org/manual/cairo-Transformations.html#cairo-scale) | cr, width/320, height/200: same nonuniform logical scale inverted for pointer delivery. |
| [cairo_paint](https://www.cairographics.org/manual/cairo-cairo-t.html#cairo-paint) | cr: paint opaque source inside existing GTK damage clip; synchronous pixel consumption. |
| [cairo_restore](https://www.cairographics.org/manual/cairo-cairo-t.html#cairo-restore) | cr: restore the borrowed native state before returning to GTK. |
| [cairo_status](https://www.cairographics.org/manual/cairo-Error-handling.html#cairo-status) | cr: check native paint status; map non-success to [FVM_ERROR](../../../include/flashvm/flashvm.h#L43). |
| [GTK_CONTAINER](https://docs.gtk.org/gtk3/class.Container.html) | Checked SDK cast of window to its container interface; transfers no ownership. |
| [G_CALLBACK](https://docs.gtk.org/gobject/callback.Callback.html) | Convert exact signal-handler signature to generic native callback type; retains no function owner. |
## SDK fixture / Проверка адаптера

With FVM_BUILD_TESTS enabled, build `flashvm_example_gtk3_input` and run
`xvfb-run -a ctest --test-dir build-gtk3 -R '^flashvm_example_gtk3_input$' --output-on-failure`.
It uses real GTK allocations/painting and explicit callback records to check
fractional resize mapping, button hold, ignored double-click notification and
focus cancellation. It requires an actual SDK display (Xvfb is software only),
not a mock Gtk header. `--smoke` additionally exercises the native timer/main loop.
Русский: тест проверяет перевод событий в VM и реальное рисование SDK; физическая
мышь/тачскрин и чужие ОС этим не проверяются.
## Vertical wheel / Вертикальная прокрутка (v0.607)

The drawing area requests GDK_SCROLL_MASK|GDK_SMOOTH_SCROLL_MASK and connects
scroll-event to gtk_scroll(widget,event,host). UP/DOWN are ±3 lines; smooth Y
is multiplied by -3 (explicit tutorial units, not an inferred pixel distance).
Zero/stop/horizontal-only events return FALSE without VM entry. Event x/y are
logical widget pixels, inverse-scaled like pointer input. demo_wheel preserves
fractions, latches real errors and never changes held contact. Successful counter
and last delta are printed at exit. The GTK fixture checks +3 and -0.75, fractional
position, and ignored horizontal/stop input. No physical wheel acceptance follows.

| API | Parameters and behavior |
| --- | --- |
| [gdk_event_get_scroll_deltas](https://docs.gtk.org/gdk3/method.Event.get_scroll_deltas.html) | Borrowed GdkEvent, writable double dx/dy outputs: TRUE only for smooth information; use dy only after success; ignore horizontal-only/zero. |
| [demo_wheel](../common/wheel.h) | Demo, world→widget transform, event x/y, signed normalized lines: synchronous public Mouse wheel dispatch, first-error latch, native scalar diagnostics; full [API contract](../../../doc/MOUSE.md#wheel-events). |

Русский: колесо не требует изменения Mouse API в GTK и не меняет нажатие кнопки.
Маска разрешает и дискретные, и smooth события. Умножение smooth-единиц на три —
явная политика учебного хоста; настройка скорости прокрутки ОС не считывается.
## Cursor visibility / Видимость курсора (v0.610)

`--hide-cursor` requests hidden startup state; `--smoke --hide-cursor` performs
120 paints. [gtk_cursor](main.c#L16)(host) reads the context on draw, obtains the realized area
window, lazily creates one owned GDK_BLANK_CURSOR for that display and applies it.
Showing passes NULL to restore parent inheritance. Main destroys native widgets
before unref of its owned blank descriptor and Demo teardown. No global display
cursor is changed. GTK acquisition/window/query failures latch a real status;
set_cursor returns void, so success records issuance rather than compositor
acknowledgement. The marker remains visible as ordinary frame content.

| API | Parameters and ownership |
| --- | --- |
| [gtk_widget_get_window](https://docs.gtk.org/gtk3/method.Widget.get_window.html) | Realized drawing area: borrow GdkWindow; NULL is unusable and stops the tutorial. Never destroy/unref this borrowed window separately. |
| [gtk_widget_get_display](https://docs.gtk.org/gtk3/method.Widget.get_display.html) | Drawing area: borrow its display for same-display cursor creation, no ownership transfer. |
| [gdk_cursor_new_for_display](https://docs.gtk.org/gdk3/ctor.Cursor.new_for_display.html) | Borrowed display, GDK_BLANK_CURSOR: acquire one native blank cursor or fail; release owned reference after window teardown with g_object_unref. |
| [gdk_window_set_cursor](https://docs.gtk.org/gdk3/method.Window.set_cursor.html) | Borrowed window, same-display blank descriptor or NULL: set local override/restore parent inheritance, retains native reference; void API does not confirm hardware state. |
| [`fvm_input_get_cursor_visible`](../../../include/flashvm/mouse.h#L38) | Context, int output: query logical request before painting, without script/GC/OS work. [Contract](../../../doc/MOUSE.md#cursor-visibility-request). |
| [`fvm_input_set_cursor_visible`](../../../include/flashvm/mouse.h#L43) | Context, 0, previous output: request hidden at startup before native realization. [Contract](../../../doc/MOUSE.md#cursor-visibility-request). |

Русский: тест с настоящим GTK/Xvfb проверяет blank descriptor, повторное
применение и возврат наследуемого курсора. Время жизни нативной ссылки отделено
от VM; после остановки callbacks сначала разрушается окно, затем снимается ссылка.
Курсор физического устройства, сенсорный экран и другие ОС не считаются принятыми.
```sh
./build-gtk3/examples/frontends/flashvm_example_gtk3 --smoke --hide-cursor
```

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../../../include/flashvm/flashvm.h#L43) |
| `fvm_input_get_cursor_visible` | function | [include/flashvm/mouse.h:38](../../../include/flashvm/mouse.h#L38) |
| `fvm_input_set_cursor_visible` | function | [include/flashvm/mouse.h:43](../../../include/flashvm/mouse.h#L43) |
<!-- source-links:end -->
