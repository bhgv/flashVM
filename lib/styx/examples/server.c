/* Copyright (c) 2026 bhgv
 * EN: Original libstyx callback server over a replaceable, lazily queried catalog backend.
 * RU: Callback-сервер исходной libstyx над заменяемым лениво запрашиваемым catalog backend.
 */
#include <lib9.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <styx.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <styxserver.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <sys/socket.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <netinet/in.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <signal.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include "catalog.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
static volatile sig_atomic_t stopping; /* Signal handler publishes shutdown only; callbacks own all backend work. */
static CatalogRow metadata; /* Original synchronous dispatcher packs stat strings before the next callback. */

/* EN/RU: Request orderly main-loop exit; signal number is unused, no allocation or backend call.
 * Запросить выход основного цикла; номер сигнала не используется, без allocation/backend вызовов.
 */


/* [standalone-contract:stop_server]
 * EN: stop_server: examples/server implementation and native state.
 * RU: stop_server: реализация examples/server и нативное состояние.
 * number (int number): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void stop_server(int number) /* EN: Invoke stop_server with these explicit arguments. RU: Вызвать stop_server с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    (void)number; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    stopping = 1; /* EN: Update stopping for the current operation. RU: Обновить stopping для текущей операции. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN: Fill a borrowed standard Dir from copied row data; string pointers remain borrowed until packing.
 * RU: Заполнить стандартный Dir из строки backend; строки заимствованы до упаковки.
 */


/* [standalone-contract:attributes]
 * EN: attributes: examples/server implementation and native state.
 * RU: attributes: реализация examples/server и нативное состояние.
 * row (const CatalogRow *row): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * out (Dir *out): Native metadata/string references; encode fields rather than transmitting the structure layout.
 * RU: Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void attributes(const CatalogRow *row, Dir *out) /* EN: Invoke attributes with these explicit arguments. RU: Вызвать attributes с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    memset(out, 0, sizeof(*out)); /* EN: Clear the destination before filling its fields. RU: Очистить приёмник до заполнения полей. */
    out->qid = styxqid(row->id, (row->mode & CATALOG_DIR) != 0); /* EN: Update out->qid for the current operation. RU: Обновить out->qid для текущей операции. */
    out->mode = row->mode; /* EN: Update out->mode for the current operation. RU: Обновить out->mode для текущей операции. */
    out->name = (char *)row->name; /* EN: Update out->name for the current operation. RU: Обновить out->name для текущей операции. */
    out->uid = "demo"; /* EN: Update out->uid for the current operation. RU: Обновить out->uid для текущей операции. */
    out->gid = "demo"; /* EN: Update out->gid for the current operation. RU: Обновить out->gid для текущей операции. */
    out->muid = ""; /* EN: Update out->muid for the current operation. RU: Обновить out->muid для текущей операции. */
    out->length = row->mode & CATALOG_DIR ? 0 : strlen(row->value); /* EN: Measure the NUL-terminated demonstration text. RU: Измерить учебный текст до NUL. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN: Resolve one component only, querying live backend rows; qid changes only on success.
 * RU: Разрешить один компонент по живым данным backend; менять qid только при успехе.
 */


/* [standalone-contract:walk_node]
 * EN: walk_node: examples/server implementation and native state.
 * RU: walk_node: реализация examples/server и нативное состояние.
 * qid (Qid *qid): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * name (char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *walk_node(Qid *qid, char *name) /* EN: Invoke walk_node with these explicit arguments. RU: Вызвать walk_node с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    CatalogRow rows[CATALOG_PAGE]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    size_t count; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if (!catalog_query(0, qid->path, 0, rows, 1, &count) || count != 1) /* EN: Ask the backend only for the selected object/page. RU: Запросить у backend выбранный объект или страницу. */
    { /* EN: Begin this scope. RU: Начало области. */
        return Enonexist; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    if (!(rows[0].mode & CATALOG_DIR)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return "not a directory"; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    if (!strcmp(name, "..")) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        if (!catalog_query(0, rows[0].parent, 0, rows, 1, &count) || count != 1) /* EN: Ask the backend only for the selected object/page. RU: Запросить у backend выбранный объект или страницу. */
        { /* EN: Begin this scope. RU: Начало области. */
            return Enonexist; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        *qid = styxqid(rows[0].id, (rows[0].mode & CATALOG_DIR) != 0); /* EN: Update *qid for the current operation. RU: Обновить *qid для текущей операции. */
        return nil; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    for (size_t start = 0;; start += count) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        if (!catalog_query(1, qid->path, start, rows, CATALOG_PAGE, &count)) /* EN: Ask the backend only for the selected object/page. RU: Запросить у backend выбранный объект или страницу. */
        { /* EN: Begin this scope. RU: Начало области. */
            return "backend query failed"; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        if (!count) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            return Enonexist; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        for (size_t index = 0; index < count; index++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
        { /* EN: Begin this scope. RU: Начало области. */
            if (!strcmp(rows[index].name, name)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            { /* EN: Begin this scope. RU: Начало области. */
                *qid = styxqid(rows[index].id, (rows[index].mode & CATALOG_DIR) != 0); /* EN: Update *qid for the current operation. RU: Обновить *qid для текущей операции. */
                return nil; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: Query current metadata and keep its string storage live through original server packing.
 * Запросить текущие metadata, удержав строки до упаковки исходным сервером.
 */


/* [standalone-contract:stat_node]
 * EN: stat_node: examples/server implementation and native state.
 * RU: stat_node: реализация examples/server и нативное состояние.
 * qid (Qid qid): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * out (Dir *out): Native metadata/string references; encode fields rather than transmitting the structure layout.
 * RU: Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *stat_node(Qid qid, Dir *out) /* EN: Invoke stat_node with these explicit arguments. RU: Вызвать stat_node с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    size_t count; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if (!catalog_query(0, qid.path, 0, &metadata, 1, &count) || count != 1) /* EN: Ask the backend only for the selected object/page. RU: Запросить у backend выбранный объект или страницу. */
    { /* EN: Begin this scope. RU: Начало области. */
        return Enonexist; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    attributes(&metadata, out); /* EN: Invoke attributes with these explicit arguments. RU: Вызвать attributes с этими явными аргументами. */
    return nil; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: Validate mode against actual backend metadata; no creation, truncation or permission inference from the name.
 * Проверить mode по metadata backend; без create/truncate и вывода прав из имени.
 */


/* [standalone-contract:open_node]
 * EN: open_node: examples/server implementation and native state.
 * RU: open_node: реализация examples/server и нативное состояние.
 * qid (Qid *qid): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *open_node(Qid *qid, int mode) /* EN: Invoke open_node with these explicit arguments. RU: Вызвать open_node с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    Dir info; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    char *error = stat_node(*qid, &info); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if (error) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return error; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    if (mode == OREAD && (info.mode & 0444)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return catalog_pin(qid->path) ? nil : "cannot retain opened object"; /* EN: Retain the opened object independently of its published name. RU: Удержать открытый объект независимо от имени. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    return mode == OWRITE && !(info.mode & CATALOG_DIR) && (info.mode & 0222) && catalog_pin(qid->path) ? nil : Eperm; /* EN: Retain the opened object independently of its published name. RU: Удержать открытый объект независимо от имени. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: Release the exact opened backend ID when original libstyx retires its fid, independent of namespace names.
 * Освободить точный ID открытого backend при retirement fid исходной libstyx, независимо от имён namespace.
 */


/* [standalone-contract:close_node]
 * EN: close_node: examples/server implementation and native state.
 * RU: close_node: реализация examples/server и нативное состояние.
 * qid (Qid qid): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *close_node(Qid qid, int mode) /* EN: Invoke close_node with these explicit arguments. RU: Вызвать close_node с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    (void)mode; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    catalog_unpin(qid.path); /* EN: Release the opened-object hold. RU: Снять удержание открытого объекта. */
    return nil; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN: Read text by offset or encode complete Dir records from immediate-child pages; never serialize a whole tree.
 * Directory offsets count encoded bytes, not child indices. Reject offsets inside records and undersized first buffers.
 * RU: Читать текст по offset либо целые Dir из страниц детей; никогда не сериализовать дерево целиком.
 * Offset каталога измеряется байтами записей; середина записи и слишком маленький первый буфер отклоняются.
 */


/* [standalone-contract:read_node]
 * EN: read_node: examples/server implementation and native state.
 * RU: read_node: реализация examples/server и нативное состояние.
 * qid (Qid qid): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * bytes (char *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (ulong *size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * offset (vlong offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *read_node(Qid qid, char *bytes, ulong *size, vlong offset) /* EN: Invoke read_node with these explicit arguments. RU: Вызвать read_node с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    CatalogRow rows[CATALOG_PAGE]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    size_t count; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if (offset < 0 || !catalog_query(0, qid.path, 0, rows, 1, &count) || count != 1) /* EN: Ask the backend only for the selected object/page. RU: Запросить у backend выбранный объект или страницу. */
    { /* EN: Begin this scope. RU: Начало области. */
        return Enonexist; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    if (!(rows[0].mode & CATALOG_DIR)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        size_t length = strlen(rows[0].value); /* EN: Measure the NUL-terminated demonstration text. RU: Измерить учебный текст до NUL. */
        size_t available = (uvlong)offset < length ? length - (size_t)offset : 0; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        if (*size > available) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            *size = available; /* EN: Update *size for the current operation. RU: Обновить *size для текущей операции. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        if (*size) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            memcpy(bytes, rows[0].value + (size_t)offset, *size); /* EN: Copy the explicitly bounded byte span. RU: Скопировать явно ограниченный диапазон байтов. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        return nil; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    uvlong position = 0; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    ulong written = 0; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    for (size_t start = 0;; start += count) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        if (!catalog_query(1, qid.path, start, rows, CATALOG_PAGE, &count)) /* EN: Ask the backend only for the selected object/page. RU: Запросить у backend выбранный объект или страницу. */
        { /* EN: Begin this scope. RU: Начало области. */
            return "backend query failed"; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        if (!count) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            *size = written; /* EN: Update *size for the current operation. RU: Обновить *size для текущей операции. */
            return position < (uvlong)offset ? "directory changed; rewind to zero" : nil; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        for (size_t index = 0; index < count; index++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
        { /* EN: Begin this scope. RU: Начало области. */
            Dir info; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
            attributes(&rows[index], &info); /* EN: Invoke attributes with these explicit arguments. RU: Вызвать attributes с этими явными аргументами. */
            uint required = sizeD2M(&info); /* EN: Measure the complete encoded directory entry. RU: Измерить целую кодированную запись каталога. */
            if (position >= (uvlong)offset) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            { /* EN: Begin this scope. RU: Начало области. */
                if (required > *size - written) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
                { /* EN: Begin this scope. RU: Начало области. */
                    *size = written; /* EN: Update *size for the current operation. RU: Обновить *size для текущей операции. */
                    return written ? nil : "directory buffer too small"; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
                } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
                written += convD2M(&info, (uchar *)bytes + written, required); /* EN: Pack a standard counted directory record. RU: Упаковать стандартную counted запись каталога. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
            else if (position + required > (uvlong)offset) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            { /* EN: Begin this scope. RU: Начало области. */
                return "directory changed; rewind to zero"; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
            position += required; /* EN: Update position for the current operation. RU: Обновить position для текущей операции. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: Delegate one write to the backend; preserve its exact acknowledgement, no replay or implicit reopen.
 * Передать одну запись backend, сохранить точное подтверждение, без повтора и переоткрытия.
 */


/* [standalone-contract:write_node]
 * EN: write_node: examples/server implementation and native state.
 * RU: write_node: реализация examples/server и нативное состояние.
 * qid (Qid qid): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * bytes (char *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (ulong *size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * offset (vlong offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *write_node(Qid qid, char *bytes, ulong *size, vlong offset) /* EN: Invoke write_node with these explicit arguments. RU: Вызвать write_node с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    long accepted = offset < 0 ? -1 : catalog_write(qid.path, bytes, *size, offset); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if (accepted < 0 || (ulong)accepted > *size) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return Eperm; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    *size = accepted; /* EN: Update *size for the current operation. RU: Обновить *size для текущей операции. */
    return nil; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN: Start original TCP server; argv1 is port (default0), argv2 optional backend source. Print actual port for clients.
 * Stop on SIGINT/SIGTERM, release original server resources; return nonzero on initialization/backend failure.
 * RU: Запустить исходный TCP-сервер; argv1 — порт (по умолчанию0), argv2 — источник backend. Вывести фактический порт.
 * По SIGINT/SIGTERM освободить сервер; при ошибке инициализации/backend вернуть ненулевой код.
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
    Styxserver server; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    Styxops ops; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    struct sockaddr_in address; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    socklen_t length = sizeof(address); /* EN: Invoke sizeof with these explicit arguments. RU: Вызвать sizeof с этими явными аргументами. */
    struct sigaction action; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    memset(&ops, 0, sizeof(ops)); /* EN: Clear the destination before filling its fields. RU: Очистить приёмник до заполнения полей. */
    memset(&action, 0, sizeof(action)); /* EN: Clear the destination before filling its fields. RU: Очистить приёмник до заполнения полей. */
    action.sa_handler = stop_server; /* EN: Update action.sa_handler for the current operation. RU: Обновить action.sa_handler для текущей операции. */
    sigemptyset(&action.sa_mask); /* EN: Invoke sigemptyset with these explicit arguments. RU: Вызвать sigemptyset с этими явными аргументами. */
    sigaction(SIGINT, &action, NULL); /* EN: Invoke sigaction with these explicit arguments. RU: Вызвать sigaction с этими явными аргументами. */
    sigaction(SIGTERM, &action, NULL); /* EN: Invoke sigaction with these explicit arguments. RU: Вызвать sigaction с этими явными аргументами. */
    ops.walk = walk_node; /* EN: Update ops.walk for the current operation. RU: Обновить ops.walk для текущей операции. */
    ops.open = open_node; /* EN: Update ops.open for the current operation. RU: Обновить ops.open для текущей операции. */
    ops.stat = stat_node; /* EN: Update ops.stat for the current operation. RU: Обновить ops.stat для текущей операции. */
    ops.read = read_node; /* EN: Update ops.read for the current operation. RU: Обновить ops.read для текущей операции. */
    ops.write = write_node; /* EN: Update ops.write for the current operation. RU: Обновить ops.write для текущей операции. */
    ops.close = close_node; /* EN: Update ops.close for the current operation. RU: Обновить ops.close для текущей операции. */
    if (!catalog_init(argc > 2 ? argv[2] : NULL)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        fprint(2, "backend initialization failed\n"); /* EN: Invoke fprint with these explicit arguments. RU: Вызвать fprint с этими явными аргументами. */
        return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    char *error = styxinit(&server, &ops, argc > 1 ? argv[1] : "0", 0555, 0); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if (error || getsockname(server.connfd, (struct sockaddr *)&address, &length)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        fprint(2, "server startup failed: %s\n", error ? error : "socket"); /* EN: Invoke fprint with these explicit arguments. RU: Вызвать fprint с этими явными аргументами. */
        return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    print("%d\n", ntohs(address.sin_port)); /* EN: Invoke print with these explicit arguments. RU: Вызвать print с этими явными аргументами. */
    while (!stopping) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        if (styxwait(&server) || (!stopping && styxprocess(&server))) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            if (!stopping) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            { /* EN: Begin this scope. RU: Начало области. */
                styxend(&server); /* EN: Invoke styxend with these explicit arguments. RU: Вызвать styxend с этими явными аргументами. */
                return 2; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    return styxend(&server) ? 3 : 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
