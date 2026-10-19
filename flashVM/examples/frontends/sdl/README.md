# SDL2 frontend / Фронтенд SDL2

[main.c](main.c) is a runnable C89 desktop tutorial with comments on every
nonempty line. It uses public FlashVM APIs through [common/demo.h](../common/demo.h),
not private player source. A green square follows mouse movement or the first
finger; primary hold changes it red. Secondary fingers never replace the first.
The API call flow, VM ownership and individual FlashVM parameter contracts are
listed in the [shared frontend guide](../README.md#public-flashvm-api-used-by-the-shared-host).

Русский: готовый учебный хост SDL2 с реальным окном/текстурой и событиями
мыши/первого касания. `main.c` владеет SDL_Window/Renderer/Texture, общая часть —
контекстом VM и RGBA-буфером. Подробные параметры FlashVM API приведены в
[общем руководстве](../README.md#public-flashvm-api-used-by-the-shared-host).

## Build/run / Сборка и запуск

Install/provide an SDL2 ≥2.0.9 CMake SDK. From the component directory:

```sh
cmake -S . -B build-sdl -DFVM_BUILD_EXAMPLES=ON -DFVM_EXAMPLE_SDL=ON -DSDL2_DIR=/absolute/sdk/lib/cmake/SDL2
cmake --build build-sdl --target flashvm_example_sdl
./build-sdl/examples/frontends/flashvm_example_sdl
./build-sdl/examples/frontends/flashvm_example_sdl --smoke
```

GRAPHICS_AGG (default) or GRAPHICS_CAIRO is required. `--smoke` exits after
120 successful uploads and prints `SDL frames=120`. Unknown options fail.
For a real SDL software test without a display, prefix the smoke command with
`SDL_VIDEODRIVER=dummy`; this does not certify a native window or physical GPU.
Interactive mode ends on close. Configure `FVM_BUILD_TESTS=ON` to build the
`flashvm_example_sdl_input` regression; run its CTest entry with dummy video.

Нужен явный SDK SDL2; SDL2_DIR указывает на каталог SDL2Config.cmake.
Без `--smoke` пример интерактивный, с ним заканчивает после 120 кадров.
Dummy-video — только software-проверка, не приёмка телефона или GPU.

## Source and callback responsibilities / Что делает код

`sdl_present(frame, userdata)` borrows a checked fixed-size frame and the
SDLHost; uploads the entire byte span, clears letterbox bars, copies texture,
then presents. `sdl_event(host, demo, event)` borrows a copied SDL_Event, selects
the first touch by both device/finger IDs, maps to DOWN/MOVE/UP and delivers
immediately on the owner thread. On focus loss it cancels at the last valid
position. A held physical mouse blocks touch takeover; active touch blocks
physical mouse. No ignored secondary finger is promoted after primary release.
Touch-generated mouse and mouse-generated touch duplicates are suppressed.
No borrowed SDL_Event survives its callback. `main` owns setup, native event
loop, 16 ms coarse pacing and reverse partial-error cleanup.

SDL_RenderSetLogicalSize already applies window/device/letterbox mapping to
SDL events. Mouse coordinates are logical pixels; finger coordinates are
normalized to that logical stage and multiplied by 320/200 **once**. The
world→event affine is consequently identity in this tutorial. Do not add the
window scale again. Outside-world pointer state remains raw; only invisible
marker drawing is bounded. No cursor capture, wheel, keyboard/IME, audio,
SWF playback, automatic Flash click/focus or Activity lifecycle is claimed.

После SDL-нормализации нельзя повторно применять масштаб окна. Все вызовы
SDL/VM выполняет один поток. Сначала прекращается цикл событий, потом
освобождаются VM/буфер, texture, renderer, window, SDL. Для асинхронной загрузки
нужна собственная копия пикселей. Android-приложению нужны SDLActivity,
поверхность и foreground/background policy: этот desktop main не APK.

## SDL SDK API and parameters

These are external SDK APIs; links point to the SDL2 reference. Their ownership
is independent of FlashVM. Errors are reported with SDL_GetError and exit 1.

| API | Parameters and behavior |
| --- | --- |
| [SDL_SetMainReady](https://wiki.libsdl.org/SDL2/SDL_SetMainReady) | No arguments; pair with SDL_MAIN_HANDLED before explicit SDL initialization. |
| [SDL_SetHint](https://wiki.libsdl.org/SDL2/SDL_SetHint) | `name`, string `value`: request disabling touch↔mouse synthesis; SDK copies setting; explicit device-ID guards still enforce suppression. |
| [SDL_Init](https://wiki.libsdl.org/SDL2/SDL_Init) | VIDEO\|TIMER flags: initialize native subsystems on owner thread; nonzero means failure; pair with SDL_Quit. |
| [SDL_CreateWindow](https://wiki.libsdl.org/SDL2/SDL_CreateWindow) | borrowed title, native x/y, pixel width/height, RESIZABLE flags: acquire window; NULL fails. |
| [SDL_CreateRenderer](https://wiki.libsdl.org/SDL2/SDL_CreateRenderer) | window owner, driver=-1, flags=0: acquire native renderer with SDK-selected backend. |
| [SDL_RenderSetLogicalSize](https://wiki.libsdl.org/SDL2/SDL_RenderSetLogicalSize) | renderer, logical width=320/height=200: scale image and normalize event coordinates; negative status fails. |
| [SDL_CreateTexture](https://wiki.libsdl.org/SDL2/SDL_CreateTexture) | renderer, RGBA32 byte format, STREAMING access, fixed width/height: acquire writable upload destination, NULL fails. |
| [SDL_SetTextureBlendMode](https://wiki.libsdl.org/SDL2/SDL_SetTextureBlendMode) | texture, NONE: display already opaque tutorial frame without extra alpha blending; nonzero fails. |
| [SDL_UpdateTexture](https://wiki.libsdl.org/SDL2/SDL_UpdateTexture) | texture, NULL rectangle=whole image, borrowed pixels, pitch in bytes: synchronous upload/copy; source may be reused after return. |
| [SDL_RenderClear](https://wiki.libsdl.org/SDL2/SDL_RenderClear) | renderer: clear target including bars; nonzero fails. |
| [SDL_RenderCopy](https://wiki.libsdl.org/SDL2/SDL_RenderCopy) | renderer, texture, NULL source/destination: copy whole logical image into renderer target; nonzero fails. |
| [SDL_RenderPresent](https://wiki.libsdl.org/SDL2/SDL_RenderPresent) | renderer: present native target; returns void, upload count is not hardware scanout evidence. |
| [SDL_PollEvent](https://wiki.libsdl.org/SDL2/SDL_PollEvent) | writable event: copy/remove one queued event; zero means queue empty; consume in order on owner thread. |
| [SDL_Delay](https://wiki.libsdl.org/SDL2/SDL_Delay) | milliseconds=16: coarse yielding, no precise movie/audio synchronization. |
| [SDL_GetError](https://wiki.libsdl.org/SDL2/SDL_GetError) | No arguments: borrow latest SDK diagnostic until SDK changes it; do not free. |
| [SDL_DestroyTexture](https://wiki.libsdl.org/SDL2/SDL_DestroyTexture) | acquired texture: release before renderer; invalid afterward. |
| [SDL_DestroyRenderer](https://wiki.libsdl.org/SDL2/SDL_DestroyRenderer) | acquired renderer: release after texture, before window. |
| [SDL_DestroyWindow](https://wiki.libsdl.org/SDL2/SDL_DestroyWindow) | acquired window: release after renderer and after event loop stops. |
| [SDL_Quit](https://wiki.libsdl.org/SDL2/SDL_Quit) | No arguments: shut down this standalone application's initialized SDL subsystems; not appropriate for unloading one embedded view from a shared SDL application. |

SDLHost retains native resources and numeric contact identities only. Physical
mouse buttons use SDL_BUTTON_LEFT; SDL_TOUCH_MOUSEID identifies synthetic
mouse events, and SDL_MOUSE_TOUCHID is checked only when exposed by the SDK.
The example is SDK-compilable C89, with no STL or sibling-project dependency.

## Vertical wheel / Вертикальная прокрутка (v0.607)

sdl_event handles real SDL_MOUSEWHEEL only when no primary touch owns the stream;
SDL_TOUCH_MOUSEID wheel duplicates are ignored. y is the legacy detent count,
preciseY replaces it when SDK >=2.0.18. SDL_MOUSEWHEEL_FLIPPED negates once, then
multiply by the explicit 3-lines-per-detent policy. Horizontal/zero events do not
enter VM. For compatibility with baseline SDL2 2.0.9 event layouts, position is
**the last successfully accepted logical VM pointer**, not a claim of exact native
wheel event-time position. No SDL_GetMouseState physical/window coordinate poll
or second logical-size transform is performed. Handler/mapping errors are latched.
Exit diagnostics include wheels and last_delta; no wheel is fabricated by smoke.

| API/field | Parameters, behavior and ownership |
| --- | --- |
| [SDL_MouseWheelEvent](https://wiki.libsdl.org/SDL2/SDL_MouseWheelEvent) | Borrowed SDL event: y/preciseY vertical units, direction inversion flag and which source; native scalar copies only. preciseY is compiled only for SDK >=2.0.18. |
| [SDL_VERSION_ATLEAST](https://wiki.libsdl.org/SDL2/SDL_VERSION_ATLEAST) | Major/minor/patch compile-time test: guard access to newer event fields; does not certify execution with older runtimes. |
| [demo_wheel](../common/wheel.h) | Demo, identity logical transform, last-known world x/y, normalized line delta: actual synchronous VM dispatcher, native diagnostics and first-error retention. [Contract](../../../doc/MOUSE.md#wheel-events). |

Русский: SDL переводит вертикаль в строки и не дублирует основное касание.
Для старого ABI позиция колеса — последняя принятая логическая позиция VM;
новая привязка к нативным mouseX/mouseY не заявляется. Проверка на настоящем SDK
с dummy-video покрывает дробь, FLIPPED, дубли и активное касание; это не физический
тест колеса. Колесо необязательно для работы сенсорного фронтенда.

## Cursor visibility / Видимость курсора (v0.610)

Run with `--hide-cursor`, optionally with `--smoke`. After events and before each
frame, [sdl_cursor](main.c#L19)(host,demo) reads requested visibility and calls SDL_ShowCursor
only on change. Initialization saves SDL's global original state; every acquired
cleanup path restores it before SDL_Quit. Negative query/apply/restore status is
a real failure. This executable is a single-window native owner; do not reuse its
global cursor policy unchanged for independent multi-window contexts. The colored
marker is content, not SDL's cursor, and remains visible. The real SDK dummy-video
fixture checks enable/hide/repeat/show via SDL_QUERY; no physical cursor is inferred.

| API | Parameters, result and ownership |
| --- | --- |
| [SDL_ShowCursor](https://wiki.libsdl.org/SDL2/SDL_ShowCursor) | SDL_QUERY returns current global 0/1 without changing it; SDL_ENABLE/SDL_DISABLE apply requested visibility. Negative means failure with SDL_GetError; save original and restore only an acquired state. |
| [`fvm_input_get_cursor_visible`](../../../include/flashvm/mouse.h#L38) | Context, visible output: read logical requested 0/1 on owner thread, preserve first demo error. [Contract](../../../doc/MOUSE.md#cursor-visibility-request). |
| [`fvm_input_set_cursor_visible`](../../../include/flashvm/mouse.h#L43) | Context, 0 at startup, mandatory previous output: request hidden without SDK/callback; applied later by native owner. [Contract](../../../doc/MOUSE.md#cursor-visibility-request). |

Русский: с --hide-cursor скрывается системный SDL-курсор, а не маркер кадра.
Исходная глобальная видимость восстанавливается при закрытии. Успех SDK не
выдаётся за видимость аппаратного курсора. Режим физического устройства/ОС
и автоматическое владение при нескольких окнах этим не приняты.
```sh
./build-sdl/examples/frontends/flashvm_example_sdl --smoke --hide-cursor
```

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_get_cursor_visible` | function | [include/flashvm/mouse.h:38](../../../include/flashvm/mouse.h#L38) |
| `fvm_input_set_cursor_visible` | function | [include/flashvm/mouse.h:43](../../../include/flashvm/mouse.h#L43) |
<!-- source-links:end -->
