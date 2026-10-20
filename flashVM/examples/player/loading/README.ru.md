# Встроенный старт, дочерний фильм и loadVariables

[English](README.md) · [Каталог](../../README.md#example-player-loading) ·
[Контракт загрузки](../../../doc/PLAYER_SOURCES.ru.md).

Исполняемый C89-пример содержит стартовый SWF в программе и загружает дочерний
фильм плюс два form-ресурса. SWF проверяет полученные строки и выбрасывает ошибку
при неверном значении. Каждая непустая строка C/заголовка/AS пояснена.
start.as и nested/child.as — пояснённые исходные эскизы с разделением на кадры;
make_movies.py независимо генерирует соответствующий SWF-байткод, не полагаясь
на незавершённые backend компиляторов. start.as нельзя исполнять одним линейным
кадром: проверки должны идти после отложенной загрузки.

## Файлы и порядок

main.c линкует startup.h; стартовый SWF через файловый вход не открывается.
Кадр 1 создаёт child, запрашивает в него nested/child.swf и root-vars.txt в root.
Оба запроса выполняются после возврата кадра. Кадр 2 проверяет greeting и
продвигает новый child; его this.loadVariables запрашивает nested/child-vars.txt.
Кадр 3 проверяет child.answer. Дочерний фильм останавливается после одного запроса.
Хост закрывается после трёх tick. root-vars.txt использует plus как пробел,
child-vars.txt — percent escapes. start.swf можно запускать файлом или по HTTP.

## Нативная и кросс-сборка

Включите frontend/examples, C89/C++98 toolchain, CMake, графический профиль DEFS
и целевой SDL SDK 2.0.9+ согласно [сборке](../../../doc/DESKTOP_FRONTEND.ru.md).

```sh
cmake --build build-player --target flashvm_example_loading
cp -R flashVM/examples/player/loading/nested build-player/frontend/
cp flashVM/examples/player/loading/root-vars.txt build-player/frontend/
./build-player/frontend/flashvm_example_loading
```

По умолчанию база встроенного фильма определяется SDL как каталог исполняемого
файла, независимо от process CWD. Если ограниченная среда не даёт определить его,
передайте полный синтетический путь источника: файла может не быть, но каталог
должен содержать ресурсы. Этот явный origin использует поставленные файлы и CTest:

```sh
./build-player/frontend/flashvm_example_loading "$PWD/flashVM/examples/player/loading/embedded.swf"
./build-player/frontend/flashvm_player --once --mute --loads 8 flashVM/examples/player/loading/start.swf
```

Нормальное завершение — код 0; проверки root/child не срабатывают. Перегенерация:
python3 flashVM/examples/player/loading/make_movies.py. Для HTTP включите
FVM_PLAYER_HTTP=ON и целевой libcurl SDK, затем:

```sh
python3 -m http.server 8000 --directory flashVM/examples/player/loading
# В другом терминале:
./build-player/frontend/flashvm_player --once --mute --loads 8 http://127.0.0.1:8000/start.swf
```

Кросс-сборкам нужны отдельные toolchain/каталоги/целевой SDL и curl при включении.
Для shared-host ELF/Mach-O зависимости должны быть PIC. Настройка — в
[руководстве источников/модулей](../../../doc/PLAYER_SOURCES.ru.md). Кросс-линковка
не является приёмкой запуска. Dummy-тесты используют SDL_VIDEODRIVER=dummy и
SDL_RENDER_DRIVER=software.

## API и владение

| API | Назначение |
| --- | --- |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Инициализация скалярных параметров приложения |
| [fvm_desktop_run_memory](../../../frontend/common/player.h#L46) | Заимствование точных байтов и имени старта до возврата |
| [FVM_DesktopOptions.max_loads](../../../frontend/common/options.h#L35) | Бюджет старта и всех запросов фильмов/переменных |
| loadMovie / MovieClip.loadVariables | Очередь скопированных запросов без сети внутри callback |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L53) | Завершение собственного глобального сеанса после очистки плеера |

[GC:] Runner удерживает queued targets до доставки, сохраняет объект и положение
дочернего клипа при замене, освобождает корни запросов при ошибке/закрытии.
Управляемые указатели наружу не выходят. Запуск — на UI-потоке без рекурсии и
конкурентных вызовов. Родитель сохраняется. Form-поля присваиваются через публичные
сеттеры по порядку; выполненные эффекты сеттеров не откатываются. Эскизы AS не
означают полного backend языка или совместимости загрузки Flash. POST,
progressive loading и root streaming sound дочернего фильма не входят в проверенный
поднабор.

[API хоста](../../../frontend/common/player.h) ·
[API загрузки ядра](../../../include/flashvm/movieclip.h) ·
[Реализация хоста](../../../frontend/common/player.c) ·
[Реализация ядра](../../../src/standard/movieclip_loading.cpp)
