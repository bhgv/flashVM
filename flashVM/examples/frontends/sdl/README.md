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
