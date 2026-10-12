/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/os_ports/main.c.
 */

/* C89 embedding example: read native pointer state and attempt a TCP connect.
 * Run against a server you control; the connect call is BLOCKING.
 * This example intentionally makes no network connection during tests. */
/* Подключаем объявления используемых типов и функций. */
#include "input_port.h"
/* Подключаем объявления используемых типов и функций. */
#include "network_port.h"
/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>
/* Initialize the example, run its API operations and release owned resources. */
/* Точка входа: запускаем последовательность действий этого примера. */
int main(void)
/* Начинаем блок инструкций или описание объекта. */
{
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_PortMouseState mouse;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_port_mouse_poll(&mouse))
    /* Начинаем блок инструкций или описание объекта. */
    {
        /* Выводим результат в стандартный поток вывода. */
        printf("Pointer: %ld,%ld buttons=%u\n", mouse.x, mouse.y, mouse.buttons);
        /* Завершаем текущий блок или описание объекта. */
    }
    /* Обрабатываем альтернативный результат проверки. */
    else
    /* Начинаем блок инструкций или описание объекта. */
    {
        /* Выводим строку с завершающим переводом строки. */
        puts("Native pointer polling unavailable; use host input dispatch");
        /* Завершаем текущий блок или описание объекта. */
    }
    /* Example when a local TCP server is available:
     * FVM_PortSocket *s = fvm_port_tcp_connect("127.0.0.1", 8080);
     * if (s) { fvm_port_socket_send(s, "ping", 4); fvm_port_socket_close(s); }
     */
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
