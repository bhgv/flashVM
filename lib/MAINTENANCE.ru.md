# Состояние сопровождения самостоятельных библиотек

Copyright (c) 2026 bhgv

[English](MAINTENANCE.md) · [Библиотеки](README.ru.md) · [Приёмка](VALIDATION.ru.md)

## Завершённый объём

Styx и libunibus выделены в отдельные исходные пакеты на C, без зависимости от
DisVM/FlashVM и C++ runtime. Проверяются hosted/minimal сборки, перемещённые SDK,
независимые offline viewers и двуязычные API references. Связь с canonical source
проверяется для 224 файлов Styx и 37 Unibus; лицензии и provenance сохранены.

Авторские исходники примеров прошли ограниченный ручной разбор контрактов:
все C/header/Python-файлы под обоими `examples/` получили описание назначения
файла и функций. Это общий catalog server/client, fixed/TSV/live/HTTP backends,
native memory carrier, nested records, native queues, gateway workers и
независимые test peers. В этой проверенной группе нет шаблонных пояснений синтаксиса.
`python3 -m lib.tools.example_lines` охватывает её целиком, но marker/lexical check
не доказывает полноту и точность содержания.

[Общий разбор Styx](styx/examples/common.ru.md) описывает request flow, byte offsets,
provisional outputs и различия времени жизни backend. Отдельные EN/RU guides
описывают команды, параметры и ограничения. TSV-рецепт исполняется из текста
обоих README при проверке документации. API call-site links пересозданы и
проверяются по фактическим строкам исходников.

## Оставшийся унаследованный долг

Этот проход **не** объявляет все комментарии экспортируемых реализаций прошедшими
ручную проверку. Исходные lib9/libstyx и ранее аннотированные codec/client/queue/walk
units ещё требуют выборочной проверки контрактов при изменении соответствующего
кода. Часть унаследованных пояснений краткая или механическая; сверка токенов и
тесты сохраняют поведение, но не доказывают качество этих описаний.

Это сопровождение, а не незавершённое выделение библиотек или недостающий пример.
Для возврата к DisVM не требуется добавлять новые демонстрационные возможности.
Конкретный найденный дефект следует воспроизводить затронутой регрессией, не
превращая этот проход по документации в разработку новой подсистемы.

## Явные границы

- Hosted execution проверен на Linux amd64. Выбор исходников других архитектур
  не равен приёмке на физических устройствах.
- Memory PHY показывает интерфейс, а не готовый проверенный SPI/CAN/radio driver.
  Reliability/order adaptation остаётся ответственностью physical adapter.
- Catalog examples — ограниченные учебные серверы. HTTP/TSV синхронно блокируют
  своего server owner; это не production asynchronous source schedulers.
- Gateway показывает negotiated nested records и изоляцию клиентов, но не
  упаковывает все engine-broker features, remote execution и migration.
- Mode bits и projected names не заменяют provider authentication policy.

Эти границы относятся к соответствующей интеграционной работе, а не к
бесконечному расширению набора примеров самостоятельных библиотек.
