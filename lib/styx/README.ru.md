# Самостоятельная библиотека Styx / 9P2000

Copyright (c) 2026 bhgv. Исходные уведомления Inferno сохранены в
[vendor/inferno/NOTICE](vendor/inferno/NOTICE) и NOTICE отдельных компонентов.

Библиотека реализована на C и не требует C++ compiler/runtime. Другие языки допустимы для примеров, тестовых клиентов и инструментов; C++ consumer tests проверяют совместимость C API, не язык реализации.

[English](README.md) · [Справка API](docs/index.ru.md) · [Примеры](examples/README.ru.md)

Проект не зависит от DisVM, FlashVM и Unibus. В нём находятся исходные codec Inferno
и существующие нативные client/server adapters с их лицензиями. Зависимостей от
графики, шрифтов, guest heap и интерпретаторов нет. Скопированная папка собирается
самостоятельно, без соседних деревьев исходников.

## Выбор слоя

| CMake target | Назначение | Требования |
| --- | --- | --- |
| `Styx::codec` | Исходные encode/decode `Fcall`/`Dir` 9P2000 | C99, `stdint.h`, стандартные операции со строками/байтами; без OS, allocation и threads |
| `Styx::wire` | Необязательное S-record обрамление явного traversal-расширения | C89, буферы вызывающего; обычный 9P этим wrapper не пользуется |
| `Styx::nine` | Полная выбранная исходная POSIX-поддержка lib9 | Linux64 libc/libm/pthread |
| `Styx::server` | Исходная C libstyx и POSIX TCP backend | `Styx::nine` |
| `Styx::client` | Нативный multiplexed 9P client, удерживаемые fid, cancellation/heartbeat | lib9, carrier, traversal, pthread |
| `Styx::phy_server` | Исходный dispatcher со сменным PHY backend | Альтернатива POSIX backend; та же неграфическая поддержка |
| `Styx::carrier` | Read/write/wait/poll/connect-disconnect физического носителя | Callback-контракт и готовый POSIX descriptor driver |
| `Styx::walk` | Необязательная история/идентичности обхода запроса | Native allocation/pthread; явное расширение, не требование стандарта 9P |

`Styx::codec` и `Styx::nine` — альтернативные поставщики исходных codec-символов.
Не линкуйте их вместе и не смешивайте scalar facade codec-only с другим конфликтующим
набором нативных типов Inferno. Для нативных client/server используются их headers,
которые CMake-target предоставляет вместе с зависимостями.

## Минимальная сборка codec

```sh
cmake -S . -B build-small -DSTYX_HOSTED=OFF -DBUILD_TESTING=ON
cmake --build build-small
ctest --test-dir build-small --output-on-failure
```

Этому варианту не нужны Python, lib9, sockets, pthread и сборка VM. Исходные codec
работают с буферами вызывающего. `convM2S` может изменять входное сообщение, чтобы
добавить завершающие NUL в counted strings; декодированные строки/data заимствуют
этот буфер. Он должен жить до окончания обработки сообщения. Нативные структуры
`Fcall`/`Dir` не являются wire-представлением.

Для микроконтроллера приложение предоставляет framing, byte transport, учёт fid,
backend и исполнение вокруг codec. Готовая hosted libstyx не объявляется уже
проверенным freestanding MCU-сервером или универсальным 32-битным портом lib9.
Границы переноса перечислены в [руководстве](docs/porting.ru.md).

## Нативные client/server и примеры

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$PWD/sdk"
```

Для hosted предусмотрен выбор Linux amd64, arm64 и riscv64 исходников; реально
исполнение проверяется на доступном Linux amd64. Python3 выбирает список исходников
и запускает проверки; host-правки formatter/decimal уже применены при экспорте; в client/server он не линкуется. HTTP-пример
пользуется helper стандартной библиотеки Python; остальным серверам HTTP/SQL-пакеты
не нужны.

Использование из другого CMake-проекта:

```cmake
find_package(Styx CONFIG REQUIRED)
add_executable(device main.c)
target_link_libraries(device PRIVATE Styx::codec)
```

Укажите установленную `sdk` в `CMAKE_PREFIX_PATH`. Либо добавьте скопированную папку
через `add_subdirectory` и используйте тот же target. Установленному потребителю не
нужны исходный репозиторий или его bootstrap-инструменты.

## Примеры файловых серверов

| Пример | Что показывает |
| --- | --- |
| [flat](examples/flat/README.ru.md) | Фиксированные `status` и `temperature` непосредственно в корне |
| [tree](examples/tree/README.ru.md) | Фиксированные каталоги, включая `devices/sensor/temperature` |
| [database](examples/database/README.ru.md) | Текущие ключи и значения явно переданной TSV-БД |
| [http](examples/http/README.ru.md) | Ленивые HTTP-страницы страна/город/район/улица/дом/комната |
| [live](examples/live/README.ru.md) | Меняющийся реестр окон/контролов с добавлением и удалением узлов |

Общий [контракт backend](examples/catalog.h) передаёт стабильные ID, не готовое
дерево. `walk` запрашивает компонент; `read` каталога возвращает целые `Dir` только
этого каталога; `read` файла работает с уже открытым fid. Перед каждым read/write
повторный `Twalk` не выполняется. Удалённый объект не должен перенаправлять старый
fid на другой объект, позднее созданный под тем же именем.

## Доступ из Inferno и Unix

Запуск: `./build/examples/styx-tree 5640`. Первая stdout-строка содержит реальный
порт; аргумент0 выбирает свободный порт. Нативный клиент:

```sh
./build/examples/styx-cat 5640 devices/sensor/temperature
```

В Inferno можно dial TCP-сервер и монтировать обычное Styx-соединение штатными
средствами. Для Unix-клиента нужно выбрать **9P2000**, не предполагая расширений
9P2000.L или 9P2000.u. Команда kernel mount и доступные transport зависят от OS;
проверка native/oracle клиента не означает проверки всех ядер и вариантов auth.

Примеры выдают условные данные и не реализуют аутентификацию/TLS. Исходный POSIX
listener привязывает заданный TCP-порт; интерфейс и политику реального сервиса
выбирают через PHY backend. Переданный `uname` не является аутентификацией.
Упрощённые проверки mode примеров не заменяют production user/group/credential policy.

## Backend, потоки и время жизни

Catalog получает только страницы непосредственных детей. HTTP-fixture вычисляет
миллиарды условных узлов, но не содержит реальных географических сведений. Его
JSON-сервис/adapter заменяется настоящим интернет-источником или БД. Offset каталога
измеряется байтами записей; изменение списка может требовать возврата к offset0.
Пример пересматривает предыдущие страницы для поиска offset: это простой алгоритм,
а не заявление о постоянной стоимости перехода к произвольной странице.

Пример сервера вызывает backend синхронно из исходного dispatcher. Медленный HTTP
задерживает именно этот пример; асинхронный production-provider использует deferred
read/owner API сервера. У HTTP-helper socket timeout3с и отдельный process alarm5с.
Ответы, поля и страницы ограничены; дерево целиком не загружается.

Нативный клиент удерживает открытый fid и соединение. Clunk/free поглощают их
владельцев. Ошибка/неопределённая запись не разрешает автоматический повтор эффекта.
Обычный raw9P отделён от opt-in S-record traversal-профиля: этот профиль явно
включается на обоих концах.

## Просмотр справки

```sh
python3 view_docs.py --port 8910 --no-browser
```

Нужна только стандартная библиотека Python3. Viewer обслуживает loopback read-only,
показывает оба языка, страницы функций, примеры и исходники с адресуемыми строками.
Ссылки на help основного проекта внешние и не нужны для сборки. См. [API](docs/index.ru.md),
[протокол](docs/protocol.ru.md), [перенос](docs/porting.ru.md).

Исходные публичные имена сохранены. Этот каталог не является полной Inferno OS.
Лицензии исходных компонентов продолжают действовать; [LICENSE.bhgv](LICENSE.bhgv)
относится к собственным дополнениям.

## Введение и руководство

[What and why](docs/introduction.ru.md) · [Writing a service](docs/tutorial.ru.md)

Пошаговое подключение из C/C++ и выбор версии/компонентов: [SDK](docs/embedding.ru.md).

Новый пример настоящего 9P без TCP и fd носителя: [Memory PHY](examples/memory_phy/README.ru.md).

## Идентификация исходного выпуска

Архив инструмента выпуска содержит `RELEASE.json` с точным commit исходников,
SHA-256 и режимом каждого файла. Отдельный `SHA256SUMS` описывает сжатые архивы.
Перед распаковкой проверьте хеш архива. Для сборки распакованных исходников Git
не нужен; настройки сборки остаются описанными выше.
