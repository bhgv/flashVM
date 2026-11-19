/* Copyright (c) 2026 bhgv
 * EN: Allocation-free tag-zero binding and fragmented nested client records, valid under strict C89.
 * RU: Назначение через тег0 и фрагментированные вложенные записи клиентов без allocation, строго C89.
 */
#include "flashvm/sstream.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include "flashvm/sbindings.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stdio.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <string.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */

/* EN/RU: Return nonzero on any real codec/namespace mismatch; no transport or VM is required.
 * Вернуть ненулевой код при несовпадении codec/пространств тегов; транспорт и VM не требуются.
 */


/* [standalone-contract:main]
 * EN: Validate the program's command-line contract, own its resources and return a process exit status.
 * RU: Проверить контракт командной строки, владеть ресурсами программы и вернуть код завершения.
 * Parameters: none. Параметры: нет.
 * Return: int; direct result constants in this body: 0, 1, 2, 3, 4. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1, 2, 3, 4. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int main(void) /* EN: Invoke main with these explicit arguments. RU: Вызвать main с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    FVM_SBinding assignment; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    FVM_SBinding slots[2]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    FVM_SBinding found; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    FVM_SBindings bindings; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    FVM_SStream stream; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    FVM_SRecord outer; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    FVM_SRecord inner; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    unsigned char metadata[128], wire[256], nested[128], storage[256]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    size_t size, used, nested_size, index, consumed; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    FVM_SResult result; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    memset(&assignment, 0, sizeof(assignment)); /* EN: Clear the destination before filling its fields. RU: Очистить приёмник до заполнения полей. */
    assignment.tag = 7; /* EN: Update assignment.tag for the current operation. RU: Обновить assignment.tag для текущей операции. */
    assignment.version = 1; /* EN: Update assignment.version for the current operation. RU: Обновить assignment.version для текущей операции. */
    assignment.type_size = 17; /* EN: Update assignment.type_size for the current operation. RU: Обновить assignment.type_size для текущей операции. */
    strcpy(assignment.type, "example.sensor.v1"); /* EN: Invoke strcpy with these explicit arguments. RU: Вызвать strcpy с этими явными аргументами. */
    if (fvm_sbindings_init(&bindings, slots, 2) != FVM_S_OK || /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        fvm_sbindings_payload_write(&assignment, 1, metadata, sizeof(metadata), &size) != FVM_S_OK || /* EN: Invoke fvm_sbindings_payload_write with these explicit arguments. RU: Вызвать fvm_sbindings_payload_write с этими явными аргументами. */
        fvm_srecord_write(0, metadata, size, wire, sizeof(wire), &used) != FVM_S_OK || /* EN: Encode a checked S-record in caller storage. RU: Кодировать проверяемую S-запись в буфер вызывающего. */
        fvm_sbindings_apply(&bindings, wire, used, &size) != FVM_S_OK) /* EN: Invoke fvm_sbindings_apply with these explicit arguments. RU: Вызвать fvm_sbindings_apply с этими явными аргументами. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    if (fvm_srecord_write(1, "21.5", 4, nested, sizeof(nested), &nested_size) != FVM_S_OK || /* EN: Encode a checked S-record in caller storage. RU: Кодировать проверяемую S-запись в буфер вызывающего. */
        fvm_srecord_write(7, nested, nested_size, wire, sizeof(wire), &size) != FVM_S_OK || /* EN: Encode a checked S-record in caller storage. RU: Кодировать проверяемую S-запись в буфер вызывающего. */
        fvm_sstream_init(&stream, storage, sizeof(storage), 256) != FVM_S_OK) /* EN: Invoke fvm_sstream_init with these explicit arguments. RU: Вызвать fvm_sstream_init с этими явными аргументами. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 2; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    result = FVM_S_MORE; /* EN: Update result for the current operation. RU: Обновить result для текущей операции. */
    for (index = 0; index < size; index++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        result = fvm_sstream_push(&stream, wire + index, 1, &outer, &consumed); /* EN: Feed a fragment and preserve the exact consumed count. RU: Подать фрагмент, сохранив точное consumed. */
        if (consumed != 1 || (result != FVM_S_MORE && result != FVM_S_OK)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            return 3; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    if (result != FVM_S_OK || outer.tag != 7 || fvm_sbindings_lookup(&bindings, outer.tag, &found) != FVM_S_OK || /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        strcmp(found.type, "example.sensor.v1") || /* EN: Invoke strcmp with these explicit arguments. RU: Вызвать strcmp с этими явными аргументами. */
        fvm_srecord_read(outer.payload, outer.size, 128, &inner, &consumed) != FVM_S_OK || /* EN: Validate one complete S-record before using its borrowed payload. RU: Проверить S-запись до использования заимствованного payload. */
        inner.encoded_size != outer.size || inner.tag != 1 || inner.size != 4 || memcmp(inner.payload, "21.5", 4)) /* EN: Compare the exact byte span, including embedded zero bytes. RU: Сравнить точный диапазон, включая нулевые байты. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 4; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    puts("PASS tag0 binding, fragmented outer client7, inner application tag1 and exact payload"); /* EN: Invoke puts with these explicit arguments. RU: Вызвать puts с этими явными аргументами. */
    return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
