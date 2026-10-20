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
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Общие значения по умолчанию; структура принадлежит приложению |
| [fvm_desktop_run_memory](../../../frontend/common/player.h#L45) | Заимствует двоичные байты и имя до возврата; точный sizeof, не strlen |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L52) | Хост завершает свой полный сеанс SDL после освобождения ресурсов плеера |

[main.c](main.c) выбирает mute, поскольку фильм содержит root stream controls.
[movie.h](movie.h) принадлежит исполняемому файлу, освобождать его нельзя.
[GC:] Runner владеет и освобождает корни VM; управляемые значения не выходят наружу.
Запуск — на основном UI-потоке без параллельных/рекурсивных вызовов. Диагностика
пока идёт через стандартные потоки. Ограничения импортера и рендера сохраняются:
нет streaming decoder, widget API и заявления о полной совместимости Flash.

[Интерфейс](../../../frontend/common/player.h) ·
[Реализация](../../../frontend/common/player.c) · [Тестовый фильм](../muted_stream/README.ru.md)

## Статический или динамический ресурсный модуль

[module.h](module.h), [module.c](module.c) и [module_host.c](module_host.c) также
пояснены построчно. Библиотека владеет слинкованным фильмом и экспортирует C89/C++98
embedded_movie_play. Хосту не нужны путь к фильму и внутренние заголовки VM.
Он вызывает функцию на UI-потоке и владеет финальным [fvm_desktop_shutdown](../../../frontend/common/player.h#L52): библиотека не
должна завершать SDL другого приложения. Указатели хоста и управляемые объекты
после возврата не сохраняются. Возвращается обычный статус плеера. Это пример
интерфейса, а не стабильный универсальный ABI VM.

```sh
cmake --build build-player --target flashvm_example_module_host
./build-player/frontend/flashvm_example_module_host
```

По умолчанию flashvm_example_embedded_module и flashvm_player_host — архивы.
FVM_PLAYER_HOST_SHARED=ON делает обе цели динамическими библиотеками. Настройте
отдельный каталог сборки с этой опцией, графическим профилем и целевым SDL SDK
со статическими PIC-библиотеками для ELF/Mach-O. Полные команды есть в
[руководстве источников](../../../doc/PLAYER_SOURCES.ru.md). Windows использует
import/export macro в module.h; статический вариант получает
EXAMPLE_EMBEDDED_STATIC транзитивно от CMake. Установка и поиск библиотек
принадлежат приложению. Нельзя выгружать модуль во время воспроизведения.
Хост по-прежнему закрывается после двух tick с кодом 0 без файла ресурса.
Android Activity этим desktop-примером не предоставляется.

Ресурсный модуль экспортирует embedded_movie_play и embedded_movie_shutdown.
Второй завершает тот же SDK instance, который использовала библиотека;
module_host.c не включает SDL/VM. Активные владельцы подсистем сохраняются,
cleanup возвращает ошибку. Явное завершение может очистить hints/TLS сеанса,
поэтому вызывается в конце использования этого сеанса приложением.
