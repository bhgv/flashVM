/* Copyright (c) 2026 bhgv
 * EN: Actual native client reads a remote file through retained fids, without VM or exported host paths.
 * RU: Настоящий нативный клиент читает удалённый файл через удерживаемые fid, без VM и экспорта host-путей.
 */
#include "client.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stdio.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stdlib.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <unistd.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <sys/socket.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <netinet/in.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <string.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */

/* EN: argv1 is numeric loopback port, argv2 is exported relative path; copy the complete remote file to stdout.
 * mountfd consumes the connected descriptor even on failure. clunk consumes fid; shutdown/free retire the client.
 * RU: argv1 — числовой loopback-порт, argv2 — относительный путь экспорта; скопировать файл в stdout.
 * mountfd принимает descriptor даже при отказе; clunk поглощает fid, shutdown/free освобождают клиента.
 */


/* [standalone-contract:main]
 * EN: Validate the program's command-line contract, own its resources and return a process exit status.
 * RU: Проверить контракт командной строки, владеть ресурсами программы и вернуть код завершения.
 * argc (int argc): Number of command-line entries, including the program name.
 * RU: Число аргументов командной строки вместе с именем программы.
 * argv (char **argv): Process-lifetime argument strings; accepted options are described by the example README.
 * RU: Строки аргументов, живущие до конца процесса; варианты описаны в README примера.
 * Return: int; direct result constants in this body: 1, 2. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1, 2. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int main(int argc, char **argv) /* EN: Invoke main with these explicit arguments. RU: Вызвать main с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    char *end; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    unsigned long port = argc == 3 ? strtoul(argv[1], &end, 10) : 0; /* EN: Parse the numeric command-line value and inspect the end pointer. RU: Разобрать числовой аргумент и проверить end pointer. */
    if (!port || port > 65535 || *end) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    int descriptor = socket(AF_INET, SOCK_STREAM, 0); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    struct sockaddr_in address; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    memset(&address, 0, sizeof(address)); /* EN: Clear the destination before filling its fields. RU: Очистить приёмник до заполнения полей. */
    address.sin_family = AF_INET; /* EN: Update address.sin_family for the current operation. RU: Обновить address.sin_family для текущей операции. */
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* EN: Update address.sin_addr.s_addr for the current operation. RU: Обновить address.sin_addr.s_addr для текущей операции. */
    address.sin_port = htons(port); /* EN: Update address.sin_port for the current operation. RU: Обновить address.sin_port для текущей операции. */
    if (descriptor < 0) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    if (connect(descriptor, (struct sockaddr *)&address, sizeof(address))) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        close(descriptor); /* EN: Release this owned native descriptor. RU: Освободить этот native descriptor. */
        return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    StyxClient *client = styxclientmountfd(descriptor, 8192, "demo", "", 5000, 8); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if (!client) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 2; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    StyxFid *fid = styxclientwalk(client, argv[2]); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    int valid = fid && styxclientopen(fid, 0); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    unsigned long long offset = 0; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    while (valid) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        unsigned char bytes[1024]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        StyxTransfer result; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        styxclientread(fid, offset, bytes, sizeof(bytes), &result); /* EN: Invoke styxclientread with these explicit arguments. RU: Вызвать styxclientread с этими явными аргументами. */
        if (!result.success || !result.confirmed || result.uncertain) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            valid = 0; /* EN: Update valid for the current operation. RU: Обновить valid для текущей операции. */
            break; /* EN: Leave this loop. RU: Выйти из цикла. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        if (!result.count) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            break; /* EN: Leave this loop. RU: Выйти из цикла. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        if (fwrite(bytes, 1, result.count, stdout) != result.count) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            valid = 0; /* EN: Update valid for the current operation. RU: Обновить valid для текущей операции. */
            break; /* EN: Leave this loop. RU: Выйти из цикла. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        offset += result.count; /* EN: Update offset for the current operation. RU: Обновить offset для текущей операции. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    styxclientclunk(fid); /* EN: Invoke styxclientclunk with these explicit arguments. RU: Вызвать styxclientclunk с этими явными аргументами. */
    styxclientshutdown(client); /* EN: Invoke styxclientshutdown with these explicit arguments. RU: Вызвать styxclientshutdown с этими явными аргументами. */
    styxclientfree(client); /* EN: Invoke styxclientfree with these explicit arguments. RU: Вызвать styxclientfree с этими явными аргументами. */
    return valid ? 0 : 3; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
