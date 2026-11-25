# phytcpaccept

[English](phytcpaccept.md) · [Объявление](../../remote_phy/drivers/posix/tcp.h) · [Исходник](../../remote_phy/drivers/posix/tcp.c)

Подключите `<styx/phy/drivers/posix/tcp.h>` и `Styx::carrier` в хостовой сборке Linux64.

```c
int phytcpaccept(int listener);
```

`listener` — заимствованный блокирующий слушатель TCP, например из
[`phytcplisten`](phytcplisten.ru.md). Функция ждёт одно соединение и повторяет
accept при прерывании. Вызывайте в потоке запуска. Возвращается собственный
неблокирующий подключённый сокет с close-on-exec и `TCP_NODELAY` либо `-1`
при ошибке. Слушатель остаётся у вызывающего. Его можно использовать снова
или закрыть. Поток можно передать в [`phyposixfd`](phyposixfd.ru.md).
