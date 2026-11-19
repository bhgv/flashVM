/* Copyright (c) 2026 bhgv
 * EN: Live key/value file as a small database backend: fresh keys/values per query, no startup tree snapshot.
 * RU: Живой файл ключ/значение как малая БД: новые ключи и значения при каждом запросе, без снимка дерева на старте.
 */
#include "catalog.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stdio.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stdlib.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <string.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
static const char *database; /* Borrowed argv path, valid until server shutdown. */

/* EN/RU: Require an explicit source path; retain only the process-lifetime argv pointer.
 * Требовать явный путь источника; заимствовать только argv до завершения процесса.
 */


/* [standalone-contract:catalog_init]
 * EN: catalog_init: examples/database implementation and native state.
 * RU: catalog_init: реализация examples/database и нативное состояние.
 * source (const char *source): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int catalog_init(const char *source) /* EN: Invoke catalog_init with these explicit arguments. RU: Вызвать catalog_init с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    database = source; /* EN: Update database for the current operation. RU: Обновить database для текущей операции. */
    return source && source[0]; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN: Derive a stable demo ID from a key; collisions are rejected when loading the small dataset.
 * RU: Вычислить стабильный ID ключа; коллизии отклоняются при чтении небольшого набора данных.
 */


/* [standalone-contract:identity]
 * EN: identity: examples/database implementation and native state.
 * RU: identity: реализация examples/database и нативное состояние.
 * name (const char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * Return: uint64_t; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: uint64_t; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static uint64_t identity(const char *name) /* EN: Invoke identity with these explicit arguments. RU: Вызвать identity с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    uint64_t result = UINT64_C(14695981039346656037); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    while (*name) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        result = (result ^ (unsigned char)*name++) * UINT64_C(1099511628211); /* EN: Update result for the current operation. RU: Обновить result для текущей операции. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    return result ? result : 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: Reread a bounded flat database; reject malformed/duplicate/colliding keys, return copied rows.
 * Перечитать ограниченную плоскую БД; отклонить неверные/повторные ключи и коллизии, вернуть скопированные строки.
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
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int catalog_query(int listing, uint64_t id, size_t start, CatalogRow *out, size_t capacity, size_t *count) /* EN: Ask the backend only for the selected object/page. RU: Запросить у backend выбранный объект или страницу. */
{ /* EN: Begin this scope. RU: Начало области. */
    FILE *file = fopen(database, "r"); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    CatalogRow rows[128]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    size_t used = 0; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    char line[2200]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    *count = 0; /* EN: Update *count for the current operation. RU: Обновить *count для текущей операции. */
    if (!file) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    while (fgets(line, sizeof(line), file)) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        char *tab = strchr(line, '\t'); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        size_t length = strlen(line); /* EN: Measure the NUL-terminated demonstration text. RU: Измерить учебный текст до NUL. */
        if (!tab || used == 128 || length == sizeof(line) - 1 || tab == line || /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            (size_t)(tab - line) >= sizeof(rows[0].name)) /* EN: Invoke sizeof with these explicit arguments. RU: Вызвать sizeof с этими явными аргументами. */
        { /* EN: Begin this scope. RU: Начало области. */
            fclose(file); /* EN: Invoke fclose with these explicit arguments. RU: Вызвать fclose с этими явными аргументами. */
            return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        *tab++ = 0; /* EN: Update *tab for the current operation. RU: Обновить *tab для текущей операции. */
        tab[strcspn(tab, "\r\n")] = 0; /* EN: Invoke strcspn with these explicit arguments. RU: Вызвать strcspn с этими явными аргументами. */
        if (strchr(line, '/') || !strcmp(line, ".") || !strcmp(line, "..") || strlen(tab) >= sizeof(rows[0].value)) /* EN: Measure the NUL-terminated demonstration text. RU: Измерить учебный текст до NUL. */
        { /* EN: Begin this scope. RU: Начало области. */
            fclose(file); /* EN: Invoke fclose with these explicit arguments. RU: Вызвать fclose с этими явными аргументами. */
            return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        memset(&rows[used], 0, sizeof(rows[used])); /* EN: Clear the destination before filling its fields. RU: Очистить приёмник до заполнения полей. */
        rows[used].id = identity(line); /* EN: Update rows[used].id for the current operation. RU: Обновить rows[used].id для текущей операции. */
        rows[used].mode = 0444; /* EN: Update rows[used].mode for the current operation. RU: Обновить rows[used].mode для текущей операции. */
        strcpy(rows[used].name, line); /* EN: Invoke strcpy with these explicit arguments. RU: Вызвать strcpy с этими явными аргументами. */
        strcpy(rows[used].value, tab); /* EN: Invoke strcpy with these explicit arguments. RU: Вызвать strcpy с этими явными аргументами. */
        for (size_t previous = 0; previous < used; previous++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
        { /* EN: Begin this scope. RU: Начало области. */
            if (rows[previous].id == rows[used].id) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            { /* EN: Begin this scope. RU: Начало области. */
                fclose(file); /* EN: Invoke fclose with these explicit arguments. RU: Вызвать fclose с этими явными аргументами. */
                return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        used++; /* EN: Update used for the current operation. RU: Обновить used для текущей операции. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    int valid = !ferror(file); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    fclose(file); /* EN: Invoke fclose with these explicit arguments. RU: Вызвать fclose с этими явными аргументами. */
    if (!valid) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    if (!listing && !id && capacity) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        CatalogRow root = {0, 0, CATALOG_DIR | 0555, "/", ""}; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        out[0] = root; /* EN: Update out[0] for the current operation. RU: Обновить out[0] для текущей операции. */
        *count = 1; /* EN: Update *count for the current operation. RU: Обновить *count для текущей операции. */
        return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    size_t seen = 0; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    for (size_t index = 0; index < used; index++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        if ((listing && !id) || (!listing && rows[index].id == id)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            if (seen++ >= start && *count < capacity) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            { /* EN: Begin this scope. RU: Начало области. */
                out[(*count)++] = rows[index]; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: This database projection is readonly; update the backend file atomically outside the server.
 * Эта проекция БД только для чтения; файл backend обновляется атомарно вне сервера.
 */


/* [standalone-contract:catalog_write]
 * EN: catalog_write: examples/database implementation and native state.
 * RU: catalog_write: реализация examples/database и нативное состояние.
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
 * EN: catalog_pin: examples/database implementation and native state.
 * RU: catalog_pin: реализация examples/database и нативное состояние.
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
 * EN: catalog_unpin: examples/database implementation and native state.
 * RU: catalog_unpin: реализация examples/database и нативное состояние.
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
