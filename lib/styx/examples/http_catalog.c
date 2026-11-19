/* Copyright (c) 2026 bhgv
 * EN: Query a paginated HTTP backend through a bounded helper process, without a shell or native HTTP dependency.
 * RU: Запрашивать постраничный HTTP backend ограниченным helper-процессом, без shell и нативной HTTP-зависимости.
 */
#define _POSIX_C_SOURCE 200809L /* EN: Select the declared compile-time interface/profile. RU: Выбрать объявленный compile-time интерфейс или профиль. */
#include "catalog.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stdio.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stdlib.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <string.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <unistd.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <sys/wait.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <errno.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
static const char *endpoint; /* Borrowed argv URL, never interpolated into a shell command. */

/* EN/RU: Retain a required backend URL for process lifetime; helper validates scheme and bounds responses.
 * Заимствовать обязательный URL backend до завершения процесса; helper проверяет схему и размер ответа.
 */


/* [standalone-contract:catalog_init]
 * EN: catalog_init: examples/http_catalog implementation and native state.
 * RU: catalog_init: реализация examples/http_catalog и нативное состояние.
 * source (const char *source): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int catalog_init(const char *source) /* EN: Invoke catalog_init with these explicit arguments. RU: Вызвать catalog_init с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    endpoint = source; /* EN: Update endpoint for the current operation. RU: Обновить endpoint для текущей операции. */
    return source && source[0]; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: Decode bounded hex into a terminated text field; reject embedded NUL or malformed bytes.
 * Декодировать ограниченный hex в текст с NUL-терминатором; отклонить внутренний NUL и неверные байты.
 */


/* [standalone-contract:text_field]
 * EN: text_field: examples/http_catalog implementation and native state.
 * RU: text_field: реализация examples/http_catalog и нативное состояние.
 * hex (const char *hex): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * output (char *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int text_field(const char *hex, char *output, size_t capacity) /* EN: Invoke text_field with these explicit arguments. RU: Вызвать text_field с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    if (!strcmp(hex, "-")) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        output[0] = 0; /* EN: Update output[0] for the current operation. RU: Обновить output[0] для текущей операции. */
        return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    size_t size = strlen(hex); /* EN: Measure the NUL-terminated demonstration text. RU: Измерить учебный текст до NUL. */
    if (size % 2 || size / 2 >= capacity) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    for (size_t index = 0; index < size; index += 2) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        char pair[3] = {hex[index], hex[index + 1], 0}; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        char *end; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        long value = strtol(pair, &end, 16); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        if (*end || value <= 0 || value > 255) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        output[index / 2] = value; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    output[size / 2] = 0; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN: Fetch one live stat/page through exec; bound every copied field and require successful helper completion.
 * Inputs follow catalog.h. Python helper enforces a 3-second HTTP timeout and 128KiB response bound.
 * RU: Получить живой stat/страницу через exec; ограничить все поля и требовать успешного завершения helper.
 * Контракт в catalog.h; Python helper задаёт HTTP timeout3с и лимит ответа128KiB.
 */


/* [standalone-contract:catalog_query]
 * EN: Query one live stat or a bounded immediate-child page without traversing the whole namespace.
 * RU: Запросить живой stat либо ограниченную страницу непосредственных детей без обхода всего namespace.
 * listing (int listing): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * id (uint64_t id): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * start (size_t start): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * out (CatalogRow *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * count (size_t *count): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int catalog_query(int listing, uint64_t id, size_t start, CatalogRow *out, size_t capacity, size_t *count) /* EN: Ask the backend only for the selected object/page. RU: Запросить у backend выбранный объект или страницу. */
{ /* EN: Begin this scope. RU: Начало области. */
    int descriptors[2]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    char number[32], offset[32], limit[32]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    *count = 0; /* EN: Update *count for the current operation. RU: Обновить *count для текущей операции. */
    if (!capacity || capacity > CATALOG_PAGE || pipe(descriptors)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    snprintf(number, sizeof(number), "%llu", (unsigned long long)id); /* EN: Format within the supplied output capacity. RU: Форматировать в указанной ёмкости. */
    snprintf(offset, sizeof(offset), "%zu", start); /* EN: Format within the supplied output capacity. RU: Форматировать в указанной ёмкости. */
    snprintf(limit, sizeof(limit), "%zu", capacity); /* EN: Format within the supplied output capacity. RU: Форматировать в указанной ёмкости. */
    pid_t child = fork(); /* EN: Invoke fork with these explicit arguments. RU: Вызвать fork с этими явными аргументами. */
    if (!child) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        close(descriptors[0]); /* EN: Release this owned native descriptor. RU: Освободить этот native descriptor. */
        if (dup2(descriptors[1], STDOUT_FILENO) < 0) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            _exit(126); /* EN: Invoke _exit with these explicit arguments. RU: Вызвать _exit с этими явными аргументами. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        close(descriptors[1]); /* EN: Release this owned native descriptor. RU: Освободить этот native descriptor. */
        execl(CATALOG_PYTHON, /* EN: Invoke execl with these explicit arguments. RU: Вызвать execl с этими явными аргументами. */
              CATALOG_PYTHON, /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
              CATALOG_HELPER, /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
              endpoint, /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
              listing ? "list" : "stat", /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
              number, /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
              offset, /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
              limit, /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
              (char *)NULL); /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
        _exit(127); /* EN: Invoke _exit with these explicit arguments. RU: Вызвать _exit с этими явными аргументами. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    close(descriptors[1]); /* EN: Release this owned native descriptor. RU: Освободить этот native descriptor. */
    if (child < 0) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        close(descriptors[0]); /* EN: Release this owned native descriptor. RU: Освободить этот native descriptor. */
        return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    FILE *stream = fdopen(descriptors[0], "r"); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    char line[4600]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    int valid = stream != NULL; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if (!stream) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        close(descriptors[0]); /* EN: Release this owned native descriptor. RU: Освободить этот native descriptor. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    while (stream && fgets(line, sizeof(line), stream)) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        unsigned long long object, parent; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        unsigned long mode; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        char name[256], value[4096], trailing; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        if (*count == capacity || /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            sscanf(line, "%llu %llu %lo %255s %4095s %c", &object, &parent, &mode, name, value, &trailing) != 5) /* EN: Invoke sscanf with these explicit arguments. RU: Вызвать sscanf с этими явными аргументами. */
        { /* EN: Begin this scope. RU: Начало области. */
            valid = 0; /* EN: Update valid for the current operation. RU: Обновить valid для текущей операции. */
            continue; /* EN: Advance to the next iteration. RU: Перейти к следующей итерации. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        CatalogRow *row = &out[(*count)++]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        memset(row, 0, sizeof(*row)); /* EN: Clear the destination before filling its fields. RU: Очистить приёмник до заполнения полей. */
        row->id = object; /* EN: Update row->id for the current operation. RU: Обновить row->id для текущей операции. */
        row->parent = parent; /* EN: Update row->parent for the current operation. RU: Обновить row->parent для текущей операции. */
        row->mode = mode; /* EN: Update row->mode for the current operation. RU: Обновить row->mode для текущей операции. */
        valid = valid && text_field(name, row->name, sizeof(row->name)) && /* EN: Update valid for the current operation. RU: Обновить valid для текущей операции. */
                text_field(value, row->value, sizeof(row->value)); /* EN: Invoke text_field with these explicit arguments. RU: Вызвать text_field с этими явными аргументами. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    if (stream) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        valid = valid && !ferror(stream); /* EN: Update valid for the current operation. RU: Обновить valid для текущей операции. */
        fclose(stream); /* EN: Invoke fclose with these explicit arguments. RU: Вызвать fclose с этими явными аргументами. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    int status; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    pid_t joined; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    do /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    { /* EN: Begin this scope. RU: Начало области. */
        joined = waitpid(child, &status, 0); /* EN: Update joined for the current operation. RU: Обновить joined для текущей операции. */
    } while (joined < 0 && errno == EINTR); /* EN: Invoke while with these explicit arguments. RU: Вызвать while с этими явными аргументами. */
    return valid && joined == child && WIFEXITED(status) && WEXITSTATUS(status) == 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: Readonly HTTP projection refuses writes; no implicit HTTP mutation/retry.
 * HTTP-проекция только для чтения; без неявных HTTP-изменений и повторов.
 */


/* [standalone-contract:catalog_write]
 * EN: catalog_write: examples/http_catalog implementation and native state.
 * RU: catalog_write: реализация examples/http_catalog и нативное состояние.
 * id (uint64_t id): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * input (const char *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * offset (uint64_t offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * Return: long; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: long; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
long catalog_write(uint64_t id, const char *input, size_t size, uint64_t offset) /* EN: Invoke catalog_write with these explicit arguments. RU: Вызвать catalog_write с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    (void)id; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    (void)input; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    (void)size; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    (void)offset; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    return -1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: This backend keeps scalar logical identities without an extra per-open resource; accept the validated ID.
 * Этот backend использует scalar logical ID без дополнительного open-ресурса; принять проверенный ID.
 */


/* [standalone-contract:catalog_pin]
 * EN: catalog_pin: examples/http_catalog implementation and native state.
 * RU: catalog_pin: реализация examples/http_catalog и нативное состояние.
 * id (uint64_t id): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int catalog_pin(uint64_t id) /* EN: Retain the opened object independently of its published name. RU: Удержать открытый объект независимо от имени. */
{ /* EN: Begin this scope. RU: Начало области. */
    (void)id; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: No extra open resource was acquired by catalog_pin; id is intentionally unused.
 * catalog_pin не получал дополнительный ресурс; id намеренно не используется.
 */


/* [standalone-contract:catalog_unpin]
 * EN: catalog_unpin: examples/http_catalog implementation and native state.
 * RU: catalog_unpin: реализация examples/http_catalog и нативное состояние.
 * id (uint64_t id): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void catalog_unpin(uint64_t id) /* EN: Release the opened-object hold. RU: Снять удержание открытого объекта. */
{ /* EN: Begin this scope. RU: Начало области. */
    (void)id; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
