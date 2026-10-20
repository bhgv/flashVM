# SWF, слинкованный с программой

[English](README.md) · [Каталог](../../README.md#example-player-embedded_movie) ·
[Контракт входа из памяти](../../../doc/PLAYER_SOURCES.ru.md).

Полный C89-пример проигрывает неизменный массив SWF8, включённый при линковке.
Файл SWF и аргументы командной строки при запуске не нужны. Подходит для
стартового ролика приложения, preview-ресурса или кода будущего библиотечного
модуля. Каждая непустая строка C/заголовка пояснена. movie.h содержит точные
байты тестового фильма muted_stream.

Нужны C89/C++98, CMake, SDL2 SDK 2.0.9+ с CMake, графический DEFS-профиль,
FVM_BUILD_DESKTOP_FRONTEND=ON и FVM_BUILD_EXAMPLES=ON. Настройка описана в
[руководстве конфигурации](../../../doc/PLAYER_CONFIGURATION.ru.md), затем:

```sh
cmake --build build-player --target flashvm_example_embedded_movie
./build-player/frontend/flashvm_example_embedded_movie
```

Небольшое окно закрывается после двух muted interpreter tick, код возврата 0.
Без дисплея задайте SDL_VIDEODRIVER=dummy и SDL_RENDER_DRIVER=software: это
проверка протокола, не физического экрана. Для другой ОС/CPU нужны отдельные
целевой SDL, toolchain и каталог сборки согласно руководству frontend.

| API | Назначение |
| --- | --- |
| fvm_desktop_options_init | Общие значения по умолчанию; структура принадлежит приложению |
| fvm_desktop_run_memory | Заимствует двоичные байты и имя до возврата; точный sizeof, не strlen |
| SDL_Quit | Хост завершает свой полный сеанс SDL после освобождения ресурсов плеера |

[main.c](main.c) выбирает mute, поскольку фильм содержит root stream controls.
[movie.h](movie.h) принадлежит исполняемому файлу, освобождать его нельзя.
[GC:] Runner владеет и освобождает корни VM; управляемые значения не выходят наружу.
Запуск — на основном UI-потоке без параллельных/рекурсивных вызовов. Диагностика
пока идёт через стандартные потоки. Ограничения импортера и рендера сохраняются:
нет streaming decoder, widget API и заявления о полной совместимости Flash.

[Интерфейс](../../../frontend/common/player.h) ·
[Реализация](../../../frontend/common/player.c) · [Тестовый фильм](../muted_stream/README.ru.md)
