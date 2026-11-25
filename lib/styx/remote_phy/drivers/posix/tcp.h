/* Copyright (c) 2026 bhgv
 * EN: Open native TCP carriers before assigning any protocol to them.
 * RU: Открытие TCP-носителей до назначения протокола.
 */
#ifndef REMOTE_PHY_POSIX_TCP_H
#define REMOTE_PHY_POSIX_TCP_H
#ifdef __cplusplus
extern "C" {
#endif


/* EN: Dial an explicit host and decimal port 1..65535. After resolving the
 * host, all addresses share timeout_ms. Return an owned nonblocking CLOEXEC
 * socket or -1 without leaving open sockets. Call on a launcher thread.
 * RU: Подключиться к хосту и десятичному порту 1..65535. После разрешения
 * имени у всех адресов общий timeout_ms. Вернуть свой неблокирующий сокет
 * CLOEXEC или -1 без оставленных сокетов. Вызывать в потоке запуска. */
int phytcpdial(const char *host, const char *port, int timeout_ms);


/* EN: Bind the first available address of explicit host. Port 0 asks the OS
 * to pick a port. Return an owned blocking CLOEXEC listener and store its
 * actual port; -1 on failure, without modifying bound_port.
 * RU: Привязать первый доступный адрес заданного хоста. Порт 0 выбирает ОС.
 * Вернуть свой блокирующий слушатель CLOEXEC и записать порт. При ошибке
 * вернуть -1 и не менять bound_port. */
int phytcplisten(const char *host, const char *port, unsigned *bound_port);


/* EN: Borrow the blocking listener for one accept, retrying interruptions.
 * Return an owned nonblocking CLOEXEC stream or -1. Caller keeps the listener.
 * RU: Заимствовать блокирующий слушатель на один accept с повтором при
 * прерывании. Вернуть свой неблокирующий поток CLOEXEC или -1. */
int phytcpaccept(int listener);

#ifdef __cplusplus
}
#endif
#endif
