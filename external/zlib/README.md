# zlib 1.3.1

Source: https://zlib.net/fossils/zlib-1.3.1.tar.gz
SHA-256: `9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23`
License: `upstream/LICENSE` (zlib license). Upstream files are unmodified.
Full source release is included. FlashVM compiles the memory compression,
inflation and checksum components, excluding filesystem/gzip APIs.
No downloaded binaries or system zlib are required. C89 checked on Linux.

Version 1.3.2 was evaluated but its default offset declarations require
`long long`, conflicting with strict C89 in this build; 1.3.1 is pinned instead.
This pin is a compatibility decision, not a security certification.

Полный исходный выпуск zlib 1.3.1 включён без изменений с лицензией.
Используются компоненты для работы с памятью; файловые gzip API не собираются.
Выбор 1.3.1 обусловлен проверенной сборкой на строгом C89.
