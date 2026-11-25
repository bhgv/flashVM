# phytcpdial

[English](phytcpdial.md) · [Объявление](../../remote_phy/drivers/posix/tcp.h) · [Исходник](../../remote_phy/drivers/posix/tcp.c)

Подключите `<styx/phy/drivers/posix/tcp.h>` и `Styx::carrier` в хостовой
сборке Linux64.

```c
int phytcpdial(const char *host, const char *port, int timeout_ms);
```

`host` — непустое явное имя либо адрес IPv4/IPv6. `port` — десятичная строка
от 1 до 65535. `timeout_ms` — положительный бюджет подключения, общий для
всех разрешённых адресов. Разрешение имени может блокировать до начала отсчёта.

Успех возвращает собственный подключённый сокет с close-on-exec,
неблокирующим режимом и `TCP_NODELAY`. При ошибке возвращается `-1`,
временные сокеты закрываются. Вызывайте в потоке запуска. Закройте сокет
или передайте владение [`phyposixfd`](phyposixfd.ru.md), например с флагами
`PHY_STREAM | PHY_ORDERED | PHY_RELIABLE`. Обе стороны выбирают протокол
Styx или P9MX после открытия транспорта.

[Приём соединения](phytcplisten.ru.md) · [Команды FlashVM](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_PLAYER.ru.md#запуск-через-tcp)
