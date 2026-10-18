# Linux desktop launcher / Запуск в Linux

Source / Исходник: [main.c](main.c). The launcher calls the shared player on the
process main thread; SDL2 selects the native window system. No SDL_main shim is
linked. `SDL_MAIN_HANDLED` and `SDL_SetMainReady` are used by the adapter.

For Debian/Ubuntu / Для Debian/Ubuntu:

```sh
sudo apt-get install build-essential cmake libsdl2-dev
# From flashVM / Из flashVM:
cmake -S . -B build-desktop -DFVM_BUILD_DESKTOP_FRONTEND=ON
cmake --build build-desktop --target flashvm_player --parallel
./build-desktop/frontend/flashvm_player --no-jit --quality best --fps 24 movie.swf
```

Use your normal X11/Wayland desktop session. `-h`/`--help` needs no display.
SWF paths are argv bytes; a UTF-8 locale/terminal supplies UTF-8 names. The player
uses SDL's UTF-8 file adapter. Native display tests are opt-in with
`-DFVM_DESKTOP_WINDOW_TESTS=ON`; run CTest within a display session or Xvfb.
Xvfb software presentation is not physical GPU or Wayland acceptance.

Используйте обычный сеанс X11/Wayland. Для справки display не нужен. Терминал с
UTF-8 передаёт имена файлов в этой кодировке. Оконные тесты включаются явно;
для них нужен рабочий display или Xvfb. Проверка Xvfb/software не удостоверяет
физический GPU или Wayland. [Руководство](../../doc/DESKTOP_FRONTEND.ru.md).
