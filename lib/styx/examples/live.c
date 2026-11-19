/* Copyright (c) 2026 bhgv
 * EN: Live window/control registry with monotonic object IDs; the example needs no actual GUI library.
 * RU: Живой реестр окон/контролов с монотонными ID объектов; пример не требует GUI-библиотеки.
 */
#include "catalog.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stdio.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stdlib.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <string.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <limits.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */

/* EN/RU: Published name and retained open-object lifetime are separate states.
 * Опубликованное имя и время жизни удержанного open-объекта — разные состояния.
 */
typedef struct Window /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
{ /* EN: Begin this scope. RU: Начало области. */
    uint64_t id;         /* Stable never-reused base Qid. */
    unsigned references; /* Actual original-server opened fids retaining this object. */
    int visible;         /* Included in fresh root walks/listings only while set. */
} Window; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */

static Window windows[16];     /* Bounded live/held object slots, reusable only after final hidden close. */
static uint64_t next_id = 100; /* Never reused; each window reserves three consecutive stable IDs. */

/* EN/RU: Initialize one live window; optional source is unused.
 * Инициализировать одно живое окно; необязательный source не используется.
 */


/* [standalone-contract:catalog_init]
 * EN: catalog_init: examples/live implementation and native state.
 * RU: catalog_init: реализация examples/live и нативное состояние.
 * source (const char *source): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int catalog_init(const char *source) /* EN: Invoke catalog_init with these explicit arguments. RU: Вызвать catalog_init с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    (void)source; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    windows[0].id = next_id; /* EN: Update windows[0].id for the current operation. RU: Обновить windows[0].id для текущей операции. */
    windows[0].visible = 1; /* EN: Update windows[0].visible for the current operation. RU: Обновить windows[0].visible для текущей операции. */
    next_id += 3; /* EN: Update next_id for the current operation. RU: Обновить next_id для текущей операции. */
    return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: Emit one copied row into a bounded result page after skipping start matches.
 * Скопировать строку в ограниченную страницу, пропустив start совпадений.
 */


/* [standalone-contract:emit]
 * EN: emit: examples/live implementation and native state.
 * RU: emit: реализация examples/live и нативное состояние.
 * row (CatalogRow row): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * start (size_t start): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * seen (size_t *seen): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * out (CatalogRow *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * count (size_t *count): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void emit(CatalogRow row, size_t start, size_t *seen, CatalogRow *out, size_t capacity, size_t *count) /* EN: Invoke emit with these explicit arguments. RU: Вызвать emit с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    if ((*seen)++ >= start && *count < capacity) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        out[(*count)++] = row; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: Query only current registry entries; removed IDs never resolve to newly created windows.
 * Запросить только текущие записи реестра; удалённые ID не разрешаются в новые окна.
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
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int catalog_query(int listing, uint64_t id, size_t start, CatalogRow *out, size_t capacity, size_t *count) /* EN: Ask the backend only for the selected object/page. RU: Запросить у backend выбранный объект или страницу. */
{ /* EN: Begin this scope. RU: Начало области. */
    CatalogRow row = {0, 0, CATALOG_DIR | 0555, "/", ""}; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    size_t seen = 0; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    *count = 0; /* EN: Update *count for the current operation. RU: Обновить *count для текущей операции. */
    if (!listing && !id) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        emit(row, start, &seen, out, capacity, count); /* EN: Invoke emit with these explicit arguments. RU: Вызвать emit с этими явными аргументами. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    row.id = 1; /* EN: Update row.id for the current operation. RU: Обновить row.id для текущей операции. */
    row.mode = 0222; /* EN: Update row.mode for the current operation. RU: Обновить row.mode для текущей операции. */
    strcpy(row.name, "ctl"); /* EN: Invoke strcpy with these explicit arguments. RU: Вызвать strcpy с этими явными аргументами. */
    if ((listing && !id) || (!listing && id == 1)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        emit(row, start, &seen, out, capacity, count); /* EN: Invoke emit with these explicit arguments. RU: Вызвать emit с этими явными аргументами. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    for (size_t index = 0; index < 16; index++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        if (!windows[index].id) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            continue; /* EN: Advance to the next iteration. RU: Перейти к следующей итерации. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        for (unsigned part = 0; part < 3; part++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
        { /* EN: Begin this scope. RU: Начало области. */
            memset(&row, 0, sizeof(row)); /* EN: Clear the destination before filling its fields. RU: Очистить приёмник до заполнения полей. */
            row.id = windows[index].id + part; /* EN: Update row.id for the current operation. RU: Обновить row.id для текущей операции. */
            row.parent = part ? windows[index].id : 0; /* EN: Update row.parent for the current operation. RU: Обновить row.parent для текущей операции. */
            row.mode = part ? 0444 : CATALOG_DIR | 0555; /* EN: Update row.mode for the current operation. RU: Обновить row.mode для текущей операции. */
            if (!part) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            { /* EN: Begin this scope. RU: Начало области. */
                snprintf(row.name, sizeof(row.name), "window-%llu", (unsigned long long)row.id); /* EN: Format within the supplied output capacity. RU: Форматировать в указанной ёмкости. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
            else /* EN: Handle the alternative to the preceding condition. RU: Обработать альтернативу предыдущего условия. */
            { /* EN: Begin this scope. RU: Начало области. */
                strcpy(row.name, part == 1 ? "title" : "button"); /* EN: Invoke strcpy with these explicit arguments. RU: Вызвать strcpy с этими явными аргументами. */
                strcpy(row.value, part == 1 ? "Live window" : "enabled"); /* EN: Invoke strcpy with these explicit arguments. RU: Вызвать strcpy с этими явными аргументами. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
            if ((!listing && row.id == id) || (listing && row.parent == id && (id || windows[index].visible))) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            { /* EN: Begin this scope. RU: Начало области. */
                emit(row, start, &seen, out, capacity, count); /* EN: Invoke emit with these explicit arguments. RU: Вызвать emit с этими явными аргументами. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN: ctl accepts add or remove <stable-ID>; acknowledge exactly one bounded command at offset0.
 * RU: ctl принимает add либо remove <стабильный-ID>; подтвердить одну ограниченную команду при offset0.
 */


/* [standalone-contract:catalog_write]
 * EN: catalog_write: examples/live implementation and native state.
 * RU: catalog_write: реализация examples/live и нативное состояние.
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
    char command[80]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if (id != 1 || offset || !size || size >= sizeof(command) || memchr(input, 0, size)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return -1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    memcpy(command, input, size); /* EN: Copy the explicitly bounded byte span. RU: Скопировать явно ограниченный диапазон байтов. */
    command[size] = 0; /* EN: Update command[size] for the current operation. RU: Обновить command[size] для текущей операции. */
    command[strcspn(command, "\r\n")] = 0; /* EN: Invoke strcspn with these explicit arguments. RU: Вызвать strcspn с этими явными аргументами. */
    if (!strcmp(command, "add") && next_id <= UINT64_MAX - 3) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        for (size_t index = 0; index < 16; index++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
        { /* EN: Begin this scope. RU: Начало области. */
            if (!windows[index].id) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            { /* EN: Begin this scope. RU: Начало области. */
                windows[index].id = next_id; /* EN: Update windows[index].id for the current operation. RU: Обновить windows[index].id для текущей операции. */
                windows[index].visible = 1; /* EN: Update windows[index].visible for the current operation. RU: Обновить windows[index].visible для текущей операции. */
                next_id += 3; /* EN: Update next_id for the current operation. RU: Обновить next_id для текущей операции. */
                return size; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    if (!strncmp(command, "remove ", 7)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        char *end; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        unsigned long long removed = strtoull(command + 7, &end, 10); /* EN: Parse a numeric object identity and inspect the end pointer. RU: Разобрать числовой ID и проверить end pointer. */
        if (!end[0] && removed) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            for (size_t index = 0; index < 16; index++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
            { /* EN: Begin this scope. RU: Начало области. */
                if (windows[index].id == removed && windows[index].visible) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
                { /* EN: Begin this scope. RU: Начало области. */
                    windows[index].visible = 0; /* EN: Update windows[index].visible for the current operation. RU: Обновить windows[index].visible для текущей операции. */
                    if (!windows[index].references) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
                    { /* EN: Begin this scope. RU: Начало области. */
                        windows[index].id = 0; /* EN: Update windows[index].id for the current operation. RU: Обновить windows[index].id для текущей операции. */
                    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
                    return size; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
                } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    return -1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN: Retain one active object's actual opened fid; reject refcount exhaustion, never republish a hidden name.
 * RU: Удержать настоящий opened fid объекта; отказать при переполнении счётчика, не публиковать скрытое имя заново.
 */


/* [standalone-contract:catalog_pin]
 * EN: catalog_pin: examples/live implementation and native state.
 * RU: catalog_pin: реализация examples/live и нативное состояние.
 * id (uint64_t id): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int catalog_pin(uint64_t id) /* EN: Retain the opened object independently of its published name. RU: Удержать открытый объект независимо от имени. */
{ /* EN: Begin this scope. RU: Начало области. */
    if (id < 2) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    for (size_t index = 0; index < 16; index++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        Window *window = &windows[index]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        if (window->id && id >= window->id && id - window->id < 3 && window->references != UINT_MAX) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            window->references++; /* EN: Update window->references for the current operation. RU: Обновить window->references для текущей операции. */
            return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN: Consume one successful pin; an unpublished window's storage is reusable only after its final opened fid closes.
 * RU: Поглотить одно успешное удержание; место скрытого окна переиспользуется только после закрытия последнего fid.
 */


/* [standalone-contract:catalog_unpin]
 * EN: catalog_unpin: examples/live implementation and native state.
 * RU: catalog_unpin: реализация examples/live и нативное состояние.
 * id (uint64_t id): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void catalog_unpin(uint64_t id) /* EN: Release the opened-object hold. RU: Снять удержание открытого объекта. */
{ /* EN: Begin this scope. RU: Начало области. */
    for (size_t index = 0; index < 16; index++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        Window *window = &windows[index]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        if (window->id && id >= window->id && id - window->id < 3 && window->references) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            window->references--; /* EN: Update window->references for the current operation. RU: Обновить window->references для текущей операции. */
            if (!window->visible && !window->references) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            { /* EN: Begin this scope. RU: Начало области. */
                window->id = 0; /* EN: Update window->id for the current operation. RU: Обновить window->id для текущей операции. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
            return; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
