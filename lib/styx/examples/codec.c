/* Copyright (c) 2026 bhgv
 * EN: OS-free original 9P codec acceptance with high64 offsets/qids, counted strings and truncation.
 * RU: Проверка исходного 9P codec без OS с high64 offset/qid, counted strings и усечением.
 */
#include "styx/codec.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <string.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */

/* EN/RU: Return nonzero on any original-codec mismatch; no allocator, network, formatter or interpreter calls.
 * Вернуть ненулевой код при несовпадении codec; без allocator, сети, formatter и интерпретатора.
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
    Fcall input; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    Fcall output; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    unsigned char bytes[512]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    memset(&input, 0, sizeof(input)); /* EN: Clear the destination before filling its fields. RU: Очистить приёмник до заполнения полей. */
    input.type = Tread; /* EN: Update input.type for the current operation. RU: Обновить input.type для текущей операции. */
    input.tag = 17; /* EN: Update input.tag for the current operation. RU: Обновить input.tag для текущей операции. */
    input.fid = 0xfedcba98U; /* EN: Update input.fid for the current operation. RU: Обновить input.fid для текущей операции. */
    input.offset = UINT64_C(0xfedcba9876543210); /* EN: Update input.offset for the current operation. RU: Обновить input.offset для текущей операции. */
    input.count = 127; /* EN: Update input.count for the current operation. RU: Обновить input.count для текущей операции. */
    uint size = convS2M(&input, bytes, sizeof(bytes)); /* EN: Invoke convS2M with these explicit arguments. RU: Вызвать convS2M с этими явными аргументами. */
    if (size != 23 || bytes[4] != Tread || GBIT64(bytes + 11) != input.offset) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    for (uint truncated = 0; truncated < size; truncated++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        if (convM2S(bytes, truncated, &output)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            return 2; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    if (convM2S(bytes, size, &output) != size || output.offset != input.offset || output.fid != input.fid) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 3; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    Dir row; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    Dir decoded; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    char strings[512]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    memset(&row, 0, sizeof(row)); /* EN: Clear the destination before filling its fields. RU: Очистить приёмник до заполнения полей. */
    row.qid.path = UINT64_C(0xf123456789abcdef); /* EN: Update row.qid.path for the current operation. RU: Обновить row.qid.path для текущей операции. */
    row.mode = 0444; /* EN: Update row.mode for the current operation. RU: Обновить row.mode для текущей операции. */
    row.name = "temperature"; /* EN: Update row.name for the current operation. RU: Обновить row.name для текущей операции. */
    row.uid = "device"; /* EN: Update row.uid for the current operation. RU: Обновить row.uid для текущей операции. */
    row.gid = "device"; /* EN: Update row.gid for the current operation. RU: Обновить row.gid для текущей операции. */
    row.muid = ""; /* EN: Update row.muid for the current operation. RU: Обновить row.muid для текущей операции. */
    size = convD2M(&row, bytes, sizeof(bytes)); /* EN: Pack a standard counted directory record. RU: Упаковать стандартную counted запись каталога. */
    if (!size || statcheck(bytes, size) || convM2D(bytes, size, &decoded, strings) != size || /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        decoded.qid.path != row.qid.path || strcmp(decoded.name, row.name)) /* EN: Invoke strcmp with these explicit arguments. RU: Вызвать strcmp с этими явными аргументами. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 4; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
