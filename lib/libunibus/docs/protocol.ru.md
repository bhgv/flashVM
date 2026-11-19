# Протокол и время жизни

Copyright (c) 2026 bhgv

[Guide](../README.ru.md) · [API](index.ru.md) · [Examples](../examples/README.ru.md)

Unibus использует существующий canonical S-record profile. Внешним tag0 владеет служебный обработчик; клиентские конверты содержат собственные вложенные записи. Клиенты могут повторять одинаковые внутренние ID. Prefix номера — width byte с CRC5 и1..8 unsigned байтов. Счётчик принадлежит тегу/направлению и сбрасывается только по подтверждённой границе, без поколения в каждом пакете.

## Владение

Буфер codec принадлежит вызывающему. Decoded view не живёт дольше буфера. Успешный open/fid и native context имеют явные ссылки; снятие имени не переоткрывает ресурс. Cleanup/free допустимы только после завершения заимствующих calls. Native structs/pointers не сериализуются напрямую.

## Ошибки и частичный I/O

Различать malformed input, временное отсутствие, EOF, подтверждённый prefix и uncertain effects. Не повторять неопределённый write автоматически. CRC не является authentication. Размеры проверяются до выдачи данных приложению; codec сам по себе не даёт прав. Исчерпание локального буфера/allocator — явная ошибка, не ложный EOF.

## Потоки и scope

Отдельный обслуживающий владелец принимает записи; очередь потребителя не является всем физическим каналом. Потоки библиотека не останавливает ради исследования. Для каждого изменяемого caller-owned codec context нужна собственная сериализация; независимые contexts могут исполняться параллельно. Нативные очереди имеют свои mutex/condition. Пример Styx backend синхронный, это прямо ограничивает его dispatcher.

## Основной help

- [Original native libraries](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md).
- [Ordered Unibus transport](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md).
- [Inferno Styxservers reference](https://powerman.name/Inferno/man/2/styxservers.html).
