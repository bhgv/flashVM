# Очистка клипа, отмена загрузок и новая загрузка

[English](README.md) · [Каталог](../../README.md#example-player-unload_loading) ·
[Точный контракт](../../../doc/MOVIE_UNLOAD.ru.md).

Исполняемый C89-хост запускает SWF8: child/grandchild/peer, медленный запрос формы,
очистка child следующим кадром, отмена старого запроса, удаление ожидающего запроса
grandchild и новая загрузка формы в прежний экземпляр child. Несвязанный peer
сохраняет место в FIFO. Скрипт проверяет идентичность, собственное свойство,
удаление имени ребёнка, отсутствие poison из отменённого ответа и новые значения.

Файлы: построчно прокомментированный [C](main.c), [AS-эскиз](start.as),
самостоятельный [генератор](make_movies.py), готовый [SWF](start.swf),
формы [slow](slow.txt)/[never](never.txt)/[peer](peer.txt)/[fresh](fresh.txt),
локальный [сервер с задержкой](serve.py). AS — пояснение кадров; SWF непосредственно
создаётся генератором без импорта тестов и без незавершённого backend компилятора.

## Сборка и запуск

Нужны C89/C++98, CMake, Python3, SDL2 SDK целевой платформы и worker-провайдер
сокетов либо libcurl. Полные команды configure/build, генерации, сервера, C-хоста,
CLI и CTest приведены в [English](README.md#native-build-and-run). Они запускаются
из корня репозитория; компонента VM также собирается отдельно через `-S flashVM`.
Сервер слушает только 127.0.0.1:8000. Цель: `flashvm_example_unload_loading`.

Ожидаются пустая сцена и код 0; запросы start, peer, fresh в указанном порядке,
возможно с отменённым slow между ними. `/never.txt` не запрашивается. Отмена может
произойти до прихода slow на сервер. Сервер задерживает slow на 250 мс; клиент
закрывает устаревший сокет. Неверный fresh вызывает ошибку ограниченной скриптовой
проверки. 1000 fps и 600 кадров — границы loopback-примера, не обещание для Интернета.

## C API, владение и очистка

UI, VM, доставка и очистка выполняются в потоке владельца. Синхронный вызов плеера
заимствует stack descriptors, URL и userdata до возврата. Worker работает только
с native bytes; цели закреплены владельцем. Join предшествует снятию pin. Скрипт
ставит очистку в очередь, публикация выполняется после возврата из VM.

| C API | Назначение, владение и блокирование; руководство |
| --- | --- |
| [fvm_desktop_background_supported](../../../frontend/common/background.h#L49) | Проверка worker без I/O; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Инициализация структуры владельца; PLAYER_CONFIGURATION.md. |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Подготовка структуры провайдера без запуска worker; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L65) | Native adapter с заимствованной неизменной политикой и cooperative stop; SOCKET_TRANSPORT.md. |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L35) | Альтернативный curl adapter с проверкой безопасной global initialization; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_run_source_async](../../../frontend/common/background.h#L40) | Заимствует структуры до синхронного возврата; доставка child/form в безопасных точках; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L51) | Завершает SDK после возврата плеера и join; PLAYER_HOST_OWNERSHIP.md. |

Контракты: [worker](../../../doc/BACKGROUND_TRANSFER.ru.md),
[параметры](../../../doc/PLAYER_CONFIGURATION.ru.md),
[сокеты](../../../doc/SOCKET_TRANSPORT.ru.md),
[владение SDK](../../../doc/PLAYER_HOST_OWNERSHIP.ru.md).

## Другая платформа/процессор и ограничения

Настройка target toolchain и SDL2 SDK:
[сборка](../../../doc/DESKTOP_FRONTEND.ru.md),
[транспорт](../../../doc/SOCKET_TRANSPORT.ru.md). Используется та же цель
`flashvm_example_unload_loading`; Windows добавляет `.exe`. Кросс-линковка для
Linux ARM64/RISC-V64 и Windows x86_64 не означает исполнения на целевых системах.
Для curl отключается SOCKET_HTTP и включается FVM_PLAYER_HTTP с thread-safe target
SDK. HTTPS требует target TLS SDK и доверия сертификатам; этот сервер использует HTTP.

Синтетические onUnload/MovieClipLoader events не реализованы. Корень/_level0
поддержан с v0.696 по контракту очистки.
Независимое аудио и непрерываемый DNS остаются обязанностью host/provider. При
close/error выполняется join, принудительного уничтожения worker нет. Dummy SDL
подтверждает протокол и состояние, а не физическое окно или приёмку другой ОС.

## Стартовые байты внутри программы с v0.695

```sh
./build-unload/frontend/flashvm_example_unload_loading --embedded http://127.0.0.1:8000/start.swf
```

Генератор также создаёт [movie.inc](movie.inc), неизменяемый массив SWF, линкуемый
с C-программой. Источник [FVM_DESKTOP_SOURCE_MEMORY](../../../frontend/common/source.h#L5) заимствует массив; размер берётся
через sizeof, не strlen. URL задаёт происхождение, стартовый ресурс не скачивается.
Относительные slow/peer/fresh используют его сетевой каталог, в том числе после
очистки содержимого. Явно заданный origin не подменяется cwd. Временный SWF-файл
не создаётся; массив живёт до синхронного возврата плеера. Один C-host тест проверяет
оба способа старта и отсутствие HTTP `/start.swf` для встроенного массива.

## Корневой вариант и звук с v0.697

Генератор создаёт [root_start.swf](root_start.swf), [root_movie.inc](root_movie.inc)
и пояснительный [AS](root_start.as). Сохранённый onEnterFrame очищает `_level0`,
проверяет собственное свойство/пустой timeline и ждёт относительную форму fresh.
Отменяются child и root запросы; peer здесь не сохраняется, поскольку всё дерево
принадлежит корню. В SWF есть looping PCM. C-пример явно включает mute.

Команды URL/embedded и CLI со звуком приведены в
[English](README.md#root-and-audio-variant-since-v0697). CLI со звуком требует
FVM_ENABLE_SDL_AUDIO=ON при configure и рабочего SDL audio backend. Итоговые stats:
sound_events=1, audio_device=0. Закрывается старое аудиоустройство, не окно/контекст.
SDL_AUDIODRIVER=dummy проверяет cleanup worker/device без доказательства слышимости.
Без audio support используется --mute. C-host тест проверяет четыре сочетания
child/root × URL/embedded. `--root` выбирает встроенный root payload; в URL-режиме
по-прежнему скачивается явно переданный URL.

С v0.699 скрипты child slow-form и root fresh-form передают прямой объект MovieClip
в глобальную loadVariables. Цель сохраняет точную идентичность без target.toString;
объектная, строковая и method формы используют общую ограниченную owner-очередь.

Генератор также создаёт [numbered_start.swf](numbered_start.swf) и
[numbered_next.swf](numbered_next.swf); пояснение — [numbered.as](numbered.as).
Тот же C-хост запускается с `http://127.0.0.1:8000/numbered_start.swf` (URL mode).
Он отменяет старые child/root запросы через unloadMovieNum(0), загружает новый
корень через loadMovieNum(..., "0", "GET") и ждёт относительный fresh.txt.
Ожидаются только numbered_start.swf, numbered_next.swf, fresh.txt; never.txt и
never.swf не запрашиваются. Остальные уровни и POST не поддерживаются.
