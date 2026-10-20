# Запуск плеера из C89 через структуру

[English](README.md) · [Каталог](../../README.md#example-player-configured_host) ·
[Подробный контракт API](../../../doc/PLAYER_CONFIGURATION.ru.md).

Исполняемый пример встраивает общий проигрыватель через структуру без сборки
его командной строки. Подходит для native launcher или команды preview в IDE,
которая может отдать основной UI-поток до завершения воспроизведения.
Каждая непустая строка [main.c](main.c) пояснена. Компилятор ActionScript,
линкер и JIT для этого примера не требуются.

## Сборка и запуск

Нужны C89/C++98 toolchain, CMake, графический профиль FlashVM, SDL2 SDK 2.0.9+
с CMake и необязательный desktop frontend. Из корня репозитория:

```sh
cmake -S flashVM -B build-player -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_EXAMPLES=ON -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" -DSDL2_DIR=/path/to/native-sdl/lib/cmake/SDL2
cmake --build build-player --target flashvm_example_configured_host --parallel
./build-player/frontend/flashvm_example_configured_host flashVM/examples/player/muted_stream/muted-stream.swf
```

Окно сцены ненадолго открывается и закрывается после двух interpreter tick;
код возврата — 0. Тестовый фильм содержит root stream controls, поэтому mute
задан явно. Аудиоустройство не открывается. Для проверки без дисплея добавьте
перед командой SDL_VIDEODRIVER=dummy и SDL_RENDER_DRIVER=software. Это проверка
протокола, не физического экрана. Сборка для другой ОС/CPU с целевым SDL описана
в [руководстве frontend](../../../doc/DESKTOP_FRONTEND.ru.md); каталоги сборки и
метаданные разных целей нельзя смешивать. Запуск на чужой платформе не заявляется.

## API, владение и очистка

| API | Назначение и владение |
| --- | --- |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Инициализирует структуру [FVM_DesktopOptions](../../../frontend/common/options.h#L29) приложения, включая 100 процентов авторской частоты. Null возвращает 0; ресурсы не создаются |
| [fvm_desktop_run](../../../frontend/common/player.h#L34) | Проверяет весь плейлист, затем синхронно загружает, играет и освобождает ресурсы. Массив и неизменные пути заимствуются до возврата. Коды 0/1/2 описаны в контракте |
| [FVM_DESKTOP_EXIT_ARGUMENT](../../../frontend/common/player.h#L12) | Ошибка аргументов/конфигурации до запуска |

Пример выбирает interpreter, mute, два tick и явный FPS, оставляя percent по
умолчанию. Он линкует flashvm_player_host — ту же статическую реализацию, что
и CLI. Цель подключает FlashVM/SDL и метаданные версии соответствующей сборки;
OS entrypoint в библиотеку не включён. Структура и строки принадлежат приложению.
Runner владеет временной VM, [GC:] корнями и native-ресурсами; управляемые объекты
наружу не выходят. Статический текст ошибки освобождать нельзя. Запуск — на
основном UI-потоке, без рекурсии и параллельных вызовов. Финальный [fvm_desktop_shutdown](../../../frontend/common/player.h#L49)
принадлежит встраивающему приложению.

Ограничения: синхронное воспроизведение, текущая диагностика через стандартные
потоки, нет widget API, маршрутизации событий чужих окон и decoder/clock
потокового звука; полная совместимость SWF8 не заявляется.
[Владение SDL](../../../doc/PLAYER_HOST_OWNERSHIP.ru.md).

[Интерфейс хоста](../../../frontend/common/player.h) ·
[Параметры](../../../frontend/common/options.h) ·
[Реализация](../../../frontend/common/player.c) ·
[Тестовый фильм](../muted_stream/README.ru.md)
