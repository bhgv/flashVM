/* Copyright (c) 2026 bhgv
 * EN: Two-client stdin/stdout Unibus gateway to opaque loopback 9P sockets; no Styx or VM linkage.
 * RU: Двухклиентский Unibus-шлюз stdin/stdout к непрозрачным loopback 9P sockets; без линковки Styx или VM.
 */
#define _POSIX_C_SOURCE 200809L /* EN: Select the declared compile-time interface/profile. RU: Выбрать объявленный compile-time интерфейс или профиль. */
#include "bridge.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include "flashvm/sstream.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include "flashvm/sbindings.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stdio.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stdlib.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <string.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <unistd.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <signal.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <sys/socket.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <sys/time.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <netinet/in.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */

/* EN: Connect one independent client with finite per-I/O socket timeout; return owned fd/-1.
 * RU: Подключить независимого клиента с конечным socket timeout для I/O; вернуть свой fd/-1.
 */


/* [standalone-contract:connect_backend]
 * EN: connect_backend: bridge/main implementation and native state.
 * RU: connect_backend: реализация bridge/main и нативное состояние.
 * port (unsigned port): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int connect_backend(unsigned port) /* EN: Invoke connect_backend with these explicit arguments. RU: Вызвать connect_backend с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    int descriptor = socket(AF_INET, SOCK_STREAM, 0); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    struct sockaddr_in address; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    struct timeval timeout = {3, 0}; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    memset(&address, 0, sizeof(address)); /* EN: Clear the destination before filling its fields. RU: Очистить приёмник до заполнения полей. */
    address.sin_family = AF_INET; /* EN: Update address.sin_family for the current operation. RU: Обновить address.sin_family для текущей операции. */
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* EN: Update address.sin_addr.s_addr for the current operation. RU: Обновить address.sin_addr.s_addr для текущей операции. */
    address.sin_port = htons(port); /* EN: Update address.sin_port for the current operation. RU: Обновить address.sin_port для текущей операции. */
    if (descriptor < 0) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return -1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    if (setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) || /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        setsockopt(descriptor, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) || /* EN: Invoke setsockopt with these explicit arguments. RU: Вызвать setsockopt с этими явными аргументами. */
        connect(descriptor, (struct sockaddr *)&address, sizeof(address))) /* EN: Invoke connect with these explicit arguments. RU: Вызвать connect с этими явными аргументами. */
    { /* EN: Begin this scope. RU: Начало области. */
        close(descriptor); /* EN: Release this owned native descriptor. RU: Освободить этот native descriptor. */
        return -1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    return descriptor; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: Dispatch only verified nested records to predeclared clients; tag0 stays inside this broker.
 * Выдавать только проверенные вложенные записи заранее объявленным клиентам; tag0 остаётся внутри брокера.
 */


/* [standalone-contract:dispatch]
 * EN: dispatch: bridge/main implementation and native state.
 * RU: dispatch: реализация bridge/main и нативное состояние.
 * record (FVM_SRecord record): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * bindings (FVM_SBindings *bindings): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * workers (Worker *workers): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int dispatch(FVM_SRecord record, FVM_SBindings *bindings, Worker *workers) /* EN: Invoke dispatch with these explicit arguments. RU: Вызвать dispatch с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    size_t used; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if (!record.tag) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        unsigned char encoded[1024]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        size_t size; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        return fvm_srecord_write(0, record.payload, record.size, encoded, sizeof(encoded), &size) == FVM_S_OK && /* EN: Encode a checked S-record in caller storage. RU: Кодировать проверяемую S-запись в буфер вызывающего. */
               fvm_sbindings_apply(bindings, encoded, size, &used) == FVM_S_OK; /* EN: Invoke fvm_sbindings_apply with these explicit arguments. RU: Вызвать fvm_sbindings_apply с этими явными аргументами. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    FVM_SBinding assignment; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    FVM_SRecord request; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if ((record.tag != 7 && record.tag != 8) || fvm_sbindings_lookup(bindings, record.tag, &assignment) != FVM_S_OK || /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        strcmp(assignment.type, "example.9p.v1") || assignment.version != 1 || /* EN: Invoke strcmp with these explicit arguments. RU: Вызвать strcmp с этими явными аргументами. */
        fvm_srecord_read(record.payload, record.size, BRIDGE_LIMIT + 32, &request, &used) != FVM_S_OK || /* EN: Validate one complete S-record before using its borrowed payload. RU: Проверить S-запись до использования заимствованного payload. */
        used != record.size || request.tag != 1 || request.size < 7 || request.size > BRIDGE_LIMIT || /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
        bridge_length(request.payload) != request.size) /* EN: Invoke bridge_length with these explicit arguments. RU: Вызвать bridge_length с этими явными аргументами. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    if (!bridge_enqueue(&workers[record.tag - 7], request.payload, request.size)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return bridge_reply(record.tag, 2, "client queue unavailable", 24); /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN: Read S-records on the broker owner, enqueue by client and join workers on EOF/failure.
 * argv1 is a numeric loopback backend port. stdout is binary only; tag0 declares the two directional assignments.
 * RU: Читать S-записи владельцем брокера, ставить по клиентам и выполнять join по EOF/ошибке.
 * argv1 — числовой loopback-порт backend. stdout только бинарный; tag0 объявляет два назначения направления.
 */


/* [standalone-contract:main]
 * EN: Validate the program's command-line contract, own its resources and return a process exit status.
 * RU: Проверить контракт командной строки, владеть ресурсами программы и вернуть код завершения.
 * argc (int argc): Number of command-line entries, including the program name.
 * RU: Число аргументов командной строки вместе с именем программы.
 * argv (char **argv): Process-lifetime argument strings; accepted options are described by the example README.
 * RU: Строки аргументов, живущие до конца процесса; варианты описаны в README примера.
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int main(int argc, char **argv) /* EN: Invoke main with these explicit arguments. RU: Вызвать main с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    char *end; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    unsigned long port = argc == 2 ? strtoul(argv[1], &end, 10) : 0; /* EN: Parse the numeric command-line value and inspect the end pointer. RU: Разобрать числовой аргумент и проверить end pointer. */
    if (!port || port > 65535 || *end) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    signal(SIGPIPE, SIG_IGN); /* EN: Invoke signal with these explicit arguments. RU: Вызвать signal с этими явными аргументами. */
    Worker workers[2] = {0}; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    unsigned started = 0; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    int valid = 1; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    for (unsigned index = 0; index < 2; index++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        Worker *worker = &workers[index]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        worker->tag = index + 7; /* EN: Update worker->tag for the current operation. RU: Обновить worker->tag для текущей операции. */
        worker->socket = connect_backend(port); /* EN: Update worker->socket for the current operation. RU: Обновить worker->socket для текущей операции. */
        if (worker->socket < 0 || pthread_mutex_init(&worker->lock, NULL)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            valid = 0; /* EN: Update valid for the current operation. RU: Обновить valid для текущей операции. */
            if (worker->socket >= 0) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            { /* EN: Begin this scope. RU: Начало области. */
                close(worker->socket); /* EN: Release this owned native descriptor. RU: Освободить этот native descriptor. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
            break; /* EN: Leave this loop. RU: Выйти из цикла. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        if (pthread_cond_init(&worker->ready, NULL)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            pthread_mutex_destroy(&worker->lock); /* EN: Invoke pthread_mutex_destroy with these explicit arguments. RU: Вызвать pthread_mutex_destroy с этими явными аргументами. */
            close(worker->socket); /* EN: Release this owned native descriptor. RU: Освободить этот native descriptor. */
            valid = 0; /* EN: Update valid for the current operation. RU: Обновить valid для текущей операции. */
            break; /* EN: Leave this loop. RU: Выйти из цикла. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        if (pthread_create(&worker->thread, NULL, bridge_worker, worker)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            pthread_cond_destroy(&worker->ready); /* EN: Invoke pthread_cond_destroy with these explicit arguments. RU: Вызвать pthread_cond_destroy с этими явными аргументами. */
            pthread_mutex_destroy(&worker->lock); /* EN: Invoke pthread_mutex_destroy with these explicit arguments. RU: Вызвать pthread_mutex_destroy с этими явными аргументами. */
            close(worker->socket); /* EN: Release this owned native descriptor. RU: Освободить этот native descriptor. */
            valid = 0; /* EN: Update valid for the current operation. RU: Обновить valid для текущей операции. */
            break; /* EN: Leave this loop. RU: Выйти из цикла. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        started++; /* EN: Update started for the current operation. RU: Обновить started для текущей операции. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    FVM_SBinding assignments[2] = {{7, 1, 13, "example.9p.v1"}, {8, 1, 13, "example.9p.v1"}}; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    unsigned char metadata[256]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    size_t size; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if (!valid || fvm_sbindings_payload_write(assignments, 2, metadata, sizeof(metadata), &size) != FVM_S_OK || /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        !bridge_record(0, metadata, size)) /* EN: Invoke bridge_record with these explicit arguments. RU: Вызвать bridge_record с этими явными аргументами. */
    { /* EN: Begin this scope. RU: Начало области. */
        valid = 0; /* EN: Update valid for the current operation. RU: Обновить valid для текущей операции. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    unsigned char storage[BRIDGE_LIMIT + 256]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    FVM_SStream stream; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    FVM_SBinding slots[2]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    FVM_SBindings bindings; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    valid = valid && fvm_sstream_init(&stream, storage, sizeof(storage), sizeof(storage)) == FVM_S_OK && /* EN: Update valid for the current operation. RU: Обновить valid для текущей операции. */
            fvm_sbindings_init(&bindings, slots, 2) == FVM_S_OK; /* EN: Invoke fvm_sbindings_init with these explicit arguments. RU: Вызвать fvm_sbindings_init с этими явными аргументами. */
    int incomplete = 0; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    while (valid) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        int byte = getchar(); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        if (byte == EOF) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            valid = !incomplete && !ferror(stdin); /* EN: Update valid for the current operation. RU: Обновить valid для текущей операции. */
            break; /* EN: Leave this loop. RU: Выйти из цикла. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        incomplete = 1; /* EN: Update incomplete for the current operation. RU: Обновить incomplete для текущей операции. */
        unsigned char input = byte; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        FVM_SRecord record; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        size_t consumed; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        FVM_SResult result = fvm_sstream_push(&stream, &input, 1, &record, &consumed); /* EN: Feed a fragment and preserve the exact consumed count. RU: Подать фрагмент, сохранив точное consumed. */
        if (result == FVM_S_OK) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            valid = dispatch(record, &bindings, workers) && fvm_sstream_reset(&stream) == FVM_S_OK; /* EN: Release the current borrowed record before accepting another. RU: Освободить текущее borrowed представление до следующей записи. */
            incomplete = 0; /* EN: Update incomplete for the current operation. RU: Обновить incomplete для текущей операции. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        else if (result != FVM_S_MORE) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            valid = 0; /* EN: Update valid for the current operation. RU: Обновить valid для текущей операции. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    for (unsigned index = 0; index < started; index++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        pthread_mutex_lock(&workers[index].lock); /* EN: Acquire this state lock before modifying shared data. RU: Взять этот lock до изменения общих данных. */
        workers[index].ending = 1; /* EN: Update workers[index].ending for the current operation. RU: Обновить workers[index].ending для текущей операции. */
        pthread_cond_signal(&workers[index].ready); /* EN: Notify a waiting consumer of this queue. RU: Уведомить ожидающего потребителя этой очереди. */
        pthread_mutex_unlock(&workers[index].lock); /* EN: Publish the protected updates and release this state lock. RU: Опубликовать защищённые изменения и снять lock. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    for (unsigned index = 0; index < started; index++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        if (pthread_join(workers[index].thread, NULL)) /* EN: Wait for the owned worker to finish before reclaiming its state. RU: Дождаться своего worker до освобождения его состояния. */
        { /* EN: Begin this scope. RU: Начало области. */
            valid = 0; /* EN: Update valid for the current operation. RU: Обновить valid для текущей операции. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        close(workers[index].socket); /* EN: Release this owned native descriptor. RU: Освободить этот native descriptor. */
        pthread_cond_destroy(&workers[index].ready); /* EN: Invoke pthread_cond_destroy with these explicit arguments. RU: Вызвать pthread_cond_destroy с этими явными аргументами. */
        pthread_mutex_destroy(&workers[index].lock); /* EN: Invoke pthread_mutex_destroy with these explicit arguments. RU: Вызвать pthread_mutex_destroy с этими явными аргументами. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    return valid ? 0 : 2; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
