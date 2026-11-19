/* Copyright (c) 2026 bhgv
 * EN: Exact opaque byte transfer and complete serialized S-record output, independent of Styx libraries.
 * RU: Точная передача непрозрачных байт и сериализация целых S-записей, без библиотек Styx.
 */
#include "bridge.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include "flashvm/srecord.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <unistd.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <errno.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
static pthread_mutex_t output_lock = /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    PTHREAD_MUTEX_INITIALIZER; /* Complete-record output only; never locks broker RX/client queues. */

/* EN/RU: See bridge.h; partial progress stays on this descriptor, EINTR repeats only the remaining byte range.
 * См. bridge.h; частичный прогресс остаётся на этом descriptor, EINTR повторяет только оставшиеся байты.
 */


/* [standalone-contract:bridge_exact]
 * EN: bridge_exact: bridge/io implementation and native state.
 * RU: bridge_exact: реализация bridge/io и нативное состояние.
 * descriptor (int descriptor): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * bytes (void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * writing (int writing): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int bridge_exact(int descriptor, void *bytes, size_t size, int writing) /* EN: Invoke bridge_exact with these explicit arguments. RU: Вызвать bridge_exact с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    unsigned char *cursor = bytes; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    while (size) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        ssize_t amount = writing ? write(descriptor, cursor, size) : read(descriptor, cursor, size); /* EN: Invoke write with these explicit arguments. RU: Вызвать write с этими явными аргументами. */
        if (amount < 0 && errno == EINTR) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            continue; /* EN: Advance to the next iteration. RU: Перейти к следующей итерации. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        if (amount <= 0) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        cursor += amount; /* EN: Update cursor for the current operation. RU: Обновить cursor для текущей операции. */
        size -= amount; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: See bridge.h; original S-record codec supplies canonical integers and checksums.
 * См. bridge.h; исходный codec S-записей задаёт канонические целые и контрольные суммы.
 */


/* [standalone-contract:bridge_record]
 * EN: bridge_record: bridge/io implementation and native state.
 * RU: bridge_record: реализация bridge/io и нативное состояние.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int bridge_record(unsigned long tag, const void *bytes, size_t size) /* EN: Invoke bridge_record with these explicit arguments. RU: Вызвать bridge_record с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    unsigned char output[BRIDGE_LIMIT + 256]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    size_t used; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if (fvm_srecord_write(tag, bytes, size, output, sizeof(output), &used) != FVM_S_OK) /* EN: Encode a checked S-record in caller storage. RU: Кодировать проверяемую S-запись в буфер вызывающего. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    pthread_mutex_lock(&output_lock); /* EN: Acquire this state lock before modifying shared data. RU: Взять этот lock до изменения общих данных. */
    int result = bridge_exact(STDOUT_FILENO, output, used, 1); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    pthread_mutex_unlock(&output_lock); /* EN: Publish the protected updates and release this state lock. RU: Опубликовать защищённые изменения и снять lock. */
    return result; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: See bridge.h; application errors live inside the client envelope, never under outer tag0.
 * См. bridge.h; ошибки приложения находятся внутри его конверта, не под внешним tag0.
 */


/* [standalone-contract:bridge_reply]
 * EN: bridge_reply: bridge/io implementation and native state.
 * RU: bridge_reply: реализация bridge/io и нативное состояние.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * inner (unsigned long inner): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int bridge_reply(unsigned long tag, unsigned long inner, const void *bytes, size_t size) /* EN: Invoke bridge_reply with these explicit arguments. RU: Вызвать bridge_reply с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    unsigned char output[BRIDGE_LIMIT + 128]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    size_t used; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    return fvm_srecord_write(inner, bytes, size, output, sizeof(output), &used) == FVM_S_OK && /* EN: Encode a checked S-record in caller storage. RU: Кодировать проверяемую S-запись в буфер вызывающего. */
           bridge_record(tag, output, used); /* EN: Invoke bridge_record with these explicit arguments. RU: Вызвать bridge_record с этими явными аргументами. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: See bridge.h; unaligned native loads and host endian assumptions are avoided.
 * См. bridge.h; без невыровненных native loads и предположений о host endian.
 */


/* [standalone-contract:bridge_length]
 * EN: bridge_length: bridge/io implementation and native state.
 * RU: bridge_length: реализация bridge/io и нативное состояние.
 * bytes (const unsigned char *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: unsigned; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: unsigned; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
unsigned bridge_length(const unsigned char *bytes) /* EN: Invoke bridge_length with these explicit arguments. RU: Вызвать bridge_length с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    return (unsigned)bytes[0] | ((unsigned)bytes[1] << 8) | ((unsigned)bytes[2] << 16) | ((unsigned)bytes[3] << 24); /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
