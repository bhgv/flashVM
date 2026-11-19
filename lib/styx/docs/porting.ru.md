# Перенос и встраивание

Copyright (c) 2026 bhgv

[Guide](../README.ru.md) · [API](index.ru.md)

| Layer | Requirements | Граница |
| --- | --- | --- |
| Styx::codec | C99, exact uint32/uint64,8-bit bytes, byte/string primitives | Original 9P codec; no allocator/OS/threads |
| Unibus::wire | C89,8-bit bytes, caller buffers, portable32-bit limbs | Existing framing/state machines; no allocator/OS/threads |
| Hosted Styx | Linux64 libc/libm/pthread | amd64 acceptance; arm64/riscv64 source selection needs target testing |
| Unibus::queues | C99, malloc/free, pthread | Native waiting/reordering, not an interpreter scheduler |

Для MCU сначала собрать минимальный codec target. Приложение предоставляет физические read/write/wait/poll, память, session/fid ownership и callback policy. Не менять endian/ширину wire-полей под native типы. DMA/ISR callbacks не должны вызывать блокирующие hosted functions; передавать ownership обычному service worker. SPI/I2C/CAN/радио могут требовать отдельного packet/reliability adapter перед ordered-stream протоколом.

Полная hosted lib9 не объявляется уже проверенным freestanding/32-bit портом. Скопированная библиотека не требует VM, GUI или соседнего репозитория. Просмотрщик/справка необязательны при линковке. Установленный CMake SDK имеет namespace target и собственные include paths.

## Исходники и лицензии

SOURCES.jsonl фиксирует исходный путь и hash подготовленного алгоритма до оформления. Проверка экспорта сравнивает исполнимые токены после раскрытия readable_parts; комментарии/отступы и добавленные braces не считаются изменением алгоритма. Это не заменяет поведенческие проверки. Исходные лицензии Inferno сохранены отдельно от MIT дополнений bhgv.
