# Clutter3D frontend from bhgv's Android/Linux fork

[main.c](main.c) is a C89 texture/event tutorial for
[bhgv/Inferno-OS_Android-Clutter3d](https://github.com/bhgv/Inferno-OS_Android-Clutter3d),
pinned to 2a616b264f0045a3ba00ce77aa1930ca905956ea. The **inferno-os client is
excluded**: the host uses the Clutter/Cogl library sources and actor/event API,
not client emulation, xmouse_btn, libdraw or libinterp. The SDK preparation and
any compatibility fixes are tracked in [external/clutter3d](../../../external/clutter3d/README.md).
A system Clutter SDK is only an auxiliary compatibility test, not the requested
fork's source or proof that its Android backend works.

Русский: пример использует библиотечную часть указанного репозитория;
`inferno-os` не переносится и не собирается. `main.c` — самостоятельный
учебный Linux-launcher, каждая строка объяснена. Общая VM/кадровая часть —
[common/demo.h](../common/demo.h). Отдельный Android-host сохраняет владение
своим stage, Activity и GLES-контекстом, как описано ниже. Полные параметры
FlashVM API: [общее руководство](../README.md#public-flashvm-api-used-by-the-shared-host).

## Separate patch for the library / Отдельный патч библиотеки

[Standalone git am patch](clutter3d-library-fixes-v0.1.patch),
[application instructions / инструкция](LIBRARY_PATCH.md). Apply it in the
library Git, not FlashVM. It contains the same configure and subpixel fixes
previously applied by the prepared SDK route. The inferno-os client is untouched.

## Build and run / Сборка и запуск

Provide the explicit fork SDK described in the external guide. Point its pkg-config
files ahead of system Clutter/Cogl; do not mix fork headers with system binaries.
Enable GRAPHICS_AGG or GRAPHICS_CAIRO and examples:

```sh
export PKG_CONFIG_PATH=/absolute/clutter3d-sdk/lib/pkgconfig
cmake -S . -B build-clutter -DFVM_BUILD_EXAMPLES=ON -DFVM_EXAMPLE_CLUTTER3D=ON -DFVM_CLUTTER3D_SDK=/absolute/clutter3d-sdk
cmake --build build-clutter --target flashvm_example_clutter3d
./build-clutter/examples/frontends/flashvm_example_clutter3d
./build-clutter/examples/frontends/flashvm_example_clutter3d --smoke
./build-clutter/examples/frontends/flashvm_example_clutter3d --touch-only
```

The target acquires an external `clutter-1.0` SDK ≥1.12 with its transitive
Cogl/GLib/Pango/etc. dependencies. The pinned fork reports 1.14.1. `--smoke`
ends after 120 successful frame uploads and prints `Clutter frames=120`.
`--touch-only` ignores every mouse event; use it when the containing shell also
synthesizes mouse from touch. It may be combined with `--smoke`. Without smoke,
close the native stage to exit. On Linux use a working X11/GLX or EGL/GLES
backend from that SDK; Xvfb/Mesa software runs are not physical GLES acceptance.

Для SDK задайте его pkg-config-путь явно. Режим `--touch-only` нужен оболочке,
которая порождает mouse-события из касаний: пример не должен получать их дважды.
На desktop без этого режима работают обычная мышь и первичное касание.

## Code responsibilities / Что делает код

| Function | Parameters and operation |
| --- | --- |
| [clutter_present](main.c#L19) | Borrowed `frame`, `userdata=ClutterHost*`: upload full straight RGBA rows into the actor texture with actual byte stride; print/free GError; retain no VM pixels. |
| [clutter_size](main.c#L37) | Borrowed `stage`, allocation `box`, `flags`, host userdata: allocate texture to current stage size. Embedded shells can replace this policy with their own actor layout. |
| [clutter_input](main.c#L62) | Borrowed stage, native event, host userdata: select primary sequence, map stage coordinates through actor projection, remove allocation scale, then dispatch world DOWN/MOVE/UP on owner thread. |
| [clutter_deactivate](main.c#L54) | Borrowed stage/userdata: handle the native deactivate signal, synthesize UP at the last valid VM position and clear contact identity. |
| [clutter_destroy](main.c#L45) | Borrowed stage/userdata: mark children invalid, clear borrowed texture, quit only an active launcher-owned main loop. |
| [clutter_tick](main.c#L119) | Borrowed host userdata: generate/submit one frame, latch real errors, quit finite smoke mode; returning FALSE removes source and clears its ID. |
| main | `argc,argv`: parse smoke/touch-only, initialize SDK, VM and actor owners, connect signals, run single event loop, stop sources/signals and release in reverse order. |

ClutterHost owns the VM/frame and one retained stage reference. Stage owns its
child texture; the host borrows that child only until destroy. The GLib source
borrows stack userdata until removed; no callback may survive main cleanup.
Touch sequence is a borrowed identity, never dereferenced or used after end.
First BEGIN wins, other sequences never promote automatically. Touch cancel or
stage deactivation uses a synthetic UP at the previous valid world position.
Unprojectable/edge-on actor geometry cancels safely instead of delivering fake
coordinates. In mouse-enabled mode no mouse event can steal an active touch.
For touch-synthesizing shells use touch-only mode to suppress even post-END
mouse duplicates; keyboard/IME and unrelated events propagate. Vertical wheel
is handled under the v0.613 policy below.

Coordinates are **not** assumed equal to native stage pixels. First
clutter_actor_transform_stage_point converts actual stage event coordinates into
the texture's projected local allocation space (including actor transforms).
Then [fvm_host_pointer_to_world](../../../include/flashvm/host.h#L26) removes local width/320 and height/200 once.
Do not invert the actor projection or device scale again. Texture scaling is
presentation only; no authored MovieClip matrix is changed. The common renderer
uses VM world position, draws green/red marker pixels, then returns its borrowed
frame only after upload. No automatic Flash click/focus or SWF timeline is implied.

## Android shell integration / Подключение к Android-оболочке

The requested repository's app/jni/clutter_main.c enters through
clutter_android_main, connects application `ready`, and runs
clutter_android_application_run. Reuse that native lifecycle; **do not call this
example's desktop main or initialize a second Clutter application**.
In the shell's ready callback, create an independent Demo/texture under a
shell-owned actor, install the input/frame callbacks on the Clutter owner
thread, and keep userdata alive until disconnection. Use the shell's actor
allocation instead of clutter_size's full-stage policy. Route only events for
this view; a shared shell must not globally consume other views' touches.

On pause/window loss stop the frame source, cancel the selected contact and
stop VM delivery. On surface/context recreation recreate/re-upload texture
resources under the shell's GLES policy; do not reuse destroyed actors. On
shutdown remove timer and signal handlers before destroying Demo/context/pixels.
The shell owns Activity/native_app_glue, ANativeWindow, EGL/GLES and permissions.
Build the fork's Android backend with its explicit NDK/GLib-Android/Cogl SDK;
no paths to a sibling inferno-os are needed by this frontend. The desktop
launcher demonstrates callback bodies, not a tested Android application module.

Русский: сохраните Android entrypoint и цикл вашей оболочки. В `ready` создайте
Demo и texture, передайте живой userdata callbacks; в общей оболочке фильтруйте
события по своей view. При pause/surface loss прекратите таймер и доставку,
отмените контакт; при восстановлении контекста создайте текстуры заново.
Сначала отключите источники/сигналы, затем уничтожайте VM/буфер. Activity,
ANativeWindow, EGL/GLES и жизненный цикл принадлежат оболочке. Это не готовый APK
и не подтверждение работы на телефоне.

## External SDK API / API библиотеки

Links below point to the **requested fork's own public headers**, whose comments
and declarations define this integration. GLib ownership functions link to its
upstream API. First-party FlashVM parameters are in the shared guide above.

| API | Parameters, result and ownership |
| --- | --- |
| [clutter_init](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-main.h) | writable argc/argv: initialize the chosen native window/GLES backend on owner thread; require CLUTTER_INIT_SUCCESS. |
| [clutter_stage_new](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-stage.h) | No arguments: create an independent floating stage actor; explicitly retain it with g_object_ref_sink. |
| [clutter_texture_new](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/deprecated/clutter-texture.h) | No arguments: create floating texture actor; clutter_actor_add_child transfers child ownership to stage. |
| [clutter_texture_set_from_rgb_data](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/deprecated/clutter-texture.h) | texture, borrowed data, has_alpha=TRUE, width/height pixels, rowstride bytes, bytes-per-pixel=4, flags=NONE, writable GError*: upload straight RGBA; returns gboolean and consumes no source ownership. |
| [clutter_actor_add_child](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-actor.h) | parent, child: add texture to scene graph; parent adopts floating child reference. |
| [clutter_actor_set_size](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-actor.h) | actor, width/height in allocation pixels: set presentation allocation, not VM world transform. |
| [clutter_actor_get_width](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-actor.h) | actor: read current allocation width for world→local event scale; zero extent makes conversion invalid. |
| [clutter_actor_get_height](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-actor.h) | actor: read current allocation height for world→local event scale; no ownership transfer. |
| [clutter_stage_set_title](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-stage.h) | stage, borrowed title text: set native title; no client integration. |
| [clutter_actor_set_reactive](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-actor.h) | actor, TRUE: request event participation on this dedicated stage. |
| [clutter_actor_show](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-actor.h) | actor: make stage visible; actual presentation remains the native backend responsibility. |
| [clutter_event_type](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-event.h) | borrowed event: read native type, no event retention. |
| [clutter_event_get_event_sequence](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-event.h) | borrowed touch event: borrow sequence identity until end/cancel; compare identity only, never free. |
| [clutter_event_get_button](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-event.h) | borrowed button event: read native button number; only primary one maps to this tutorial pointer. |
| [clutter_event_get_coords](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-event.h) | borrowed event, writable float x/y: read stage coordinates, not actor-local/world coordinates. |
| [clutter_actor_transform_stage_point](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-actor.h) | actor, stage x/y, writable local x/y: invert projected actor transform; FALSE means unmappable and outputs must not be dispatched. |
| [clutter_actor_destroy](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-actor.h) | actor: destroy native scene/children and emit destroy; retained stage reference is released separately. |
| [clutter_main](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-main.h) | No arguments: run owner-thread event/timer loop; no VM calls from workers. |
| [clutter_main_quit](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/sources/clutter-android/clutter/clutter-main.h) | No arguments: request current loop exit; call only while this launcher owns an active loop. |
| [g_object_ref_sink](https://docs.gtk.org/gobject/method.Object.ref_sink.html) | stage object: sink floating reference or retain nonfloating object, balance with one unref. |
| [g_object_unref](https://docs.gtk.org/gobject/method.Object.unref.html) | retained stage: release explicit owner after destruction and callback shutdown. |
| [g_signal_connect](https://docs.gtk.org/gobject/func.signal_connect.html) | instance, detailed signal name, typed G_CALLBACK, userdata: borrow userdata through disconnection/object destruction; no ownership transfer. |
| [g_timeout_add](https://docs.gtk.org/glib/func.timeout_add.html) | interval in milliseconds, callback, userdata: attach recurring source to default main context, return owned source ID. |
| [g_source_remove](https://docs.gtk.org/glib/type_func.Source.remove.html) | live source ID: stop timer before releasing borrowed userdata; never remove ID already cleared by callback. |
| [g_error_free](https://docs.gtk.org/glib/method.Error.free.html) | allocated GError: release diagnostic and message once after printing; no VM object ownership. |

ClutterTexture is deprecated in newer system Clutter but intentionally matches
the requested fork/shell API. Modern system-SDK deprecation warnings do not
justify silently replacing the user-selected library. The SDK and its upstream
licenses stay external and separate from FlashVM's first-party MIT code.
## EGL/GLES2 and subpixel SDK (v0.601)

The [EGL/GLES2 profile](../../../external/clutter3d/README.md#linux-eglgles2-profile-v0601)
builds the selected fork with desktop GL/GLX disabled and EGL-Xlib/GLES2 enabled.
The same tutorial code uses it without an SDL dependency. Select a separate SDK
prefix/CMake build directory; the runtime renderer is egl_xlib and driver gles2.
The SDK includes the actor inverse fix preserving fractional event coordinates
and allocation dimensions. The real SDK regression now projects a point through
a 25-degree Y-rotated actor with fractional allocation and verifies the inverse
returns the original world point. CMake checks both fork providers and the fix
hash; it refuses mixed Clutter/Cogl SDKs or a pre-fix marker.

Русский: профиль EGL/GLES2 и исправление дробных координат описаны в SDK-guide.
Код примера тот же; дополнительного frontend на SDL здесь нет. Для смены
рендерера нужны отдельные каталоги исходников/SDK/сборки. Проверка охватывает
3D-поворот и дробный allocation; нативный Android всё ещё отдельная приёмка.
## v0.613 wheel and cursor / Колесо и курсор

Vertical scroll now follows the same inverse projected actor mapping as primary
pointer input. UP/DOWN produce +3/-3 lines; SMOOTH uses `-dy * 3`, preserving
fractions. This is an explicit tutorial policy, not the OS user's scroll setting.
Horizontal-only/zero events propagate to the shell. `--touch-only` and an active
primary touch suppress wheel delivery; ordinary wheel does not change held-button
or touch ownership. Unmappable projected positions propagate without cancelling
a held contact. A real VM delivery error is latched for loop shutdown.

The owned stage applies the VM cursor request before upload using
`clutter_stage_show_cursor/hide_cursor`; repeated requests are cached.
`--hide-cursor` sets the initial VM request and combines with `--smoke`.
The colored frame marker stays visible. A touch-only native backend may have no
physical cursor. Void stage setters prove request issuance/property changes,
not compositor or hardware acknowledgement. The launcher destroys its own stage;
an embedding Android shell must retain its own stage/lifecycle ownership policy.

Русский: колесо преобразуется через ту же обратную 3D-проекцию и масштаб актора,
что и указатель; положительная прокрутка направлена вверх. При активном touch
и в режиме `--touch-only` колесо не доставляется. Горизонтальные/нулевые события
передаются оболочке. Видимость системного курсора применяется отдельно от
цветного маркера перед загрузкой кадра. `--hide-cursor` меняет начальный запрос
VM, не создавая мышь на телефоне. Встраиваемая оболочка продолжает владеть своим
stage, Activity и GLES-контекстом.

| API | Parameters and ownership / Параметры и владение |
| --- | --- |
| `clutter_event_get_scroll_direction(event)` | Borrowed scroll event; distinguish UP, DOWN, SMOOTH and horizontal / заимствованное событие, вид прокрутки |
| `clutter_event_get_scroll_delta(event, &dx, &dy)` | Borrowed event and writable double outputs; normalize only vertical dy / выходные double, используется вертикальный dy |
| `clutter_event_get_coords(event, &x, &y)` | Event-time stage coordinates; feed inverse actor projection / координаты stage в момент события |
| `clutter_actor_transform_stage_point(actor, x, y, &lx, &ly)` | Borrowed live texture actor; returns false for unusable projection / обратная проекция в локальные координаты |
| `clutter_stage_show_cursor(stage)` / `clutter_stage_hide_cursor(stage)` | Borrowed owned-live stage, native owner thread, no returned acknowledgement / живой stage на потоке владельца, без подтверждения устройства |
| `fvm_input_get_cursor_visible(context, &visible)` | Allocation-free logical request query / чтение логического запроса без выделения памяти |
| `demo_wheel(demo, transform, x, y, delta)` | Copied positions/line delta, borrowed transform, synchronous dispatch / синхронная передача позиции и строк прокрутки |

Public fork declarations:
[event API](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/master/sources/clutter-android/clutter/clutter-event.h),
[stage API](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/master/sources/clutter-android/clutter/clutter-stage.h).
No new library patch or inferno-os client dependency is needed for these APIs.
The supplied separate library patch remains unchanged. Auxiliary system SDK
checks do not replace execution of the requested fork's Android backend.
### Current acceptance / Текущие проверки

The focused `flashvm_clutter_wheel_cursor` fixture and ordinary/hidden/touch-only
120-frame smoke runs pass on a real Linux system Clutter 1.26 SDK under Xvfb.
The same adapter sources pass ASan/UBSan with leak detection disabled; the VM and
external SDK libraries in this run are ordinary builds, so this is adapter
instrumentation only. The unchanged strict fractional-projection acceptance
(`flashvm_example_clutter3d_input`) still fails on this unpatched system SDK at
the known subpixel inverse case. It remains enabled for the prepared patched
fork; it was not weakened to make an auxiliary system run look equivalent.

Русский: проверка новых колеса/курсора отделена от строгой проверки дробной
обратной проекции. Последняя на системном Clutter без прежнего патча не проходит;
это не объявляется свежей приёмкой пользовательского форка или Android.

[clutter_event_get_scroll_direction](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/master/sources/clutter-android/clutter/clutter-event.h) — read borrowed event data / чтение заимствованных данных события.

[clutter_event_get_scroll_delta](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/master/sources/clutter-android/clutter/clutter-event.h) — read borrowed event data / чтение заимствованных данных события.

[clutter_stage_show_cursor](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/master/sources/clutter-android/clutter/clutter-stage.h) — stage-local native cursor request / запрос курсора своего stage.

[clutter_stage_hide_cursor](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/master/sources/clutter-android/clutter/clutter-stage.h) — stage-local native cursor request / запрос курсора своего stage.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_host_pointer_to_world` | function | [include/flashvm/host.h:26](../../../include/flashvm/host.h#L26) |
| `fvm_input_get_cursor_visible` | function | [include/flashvm/mouse.h:41](../../../include/flashvm/mouse.h#L41) |
<!-- source-links:end -->
