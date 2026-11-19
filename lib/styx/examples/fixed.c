/* Copyright (c) 2026 bhgv
 * EN: Fixed one-level or three-level catalogs selected at compile time, sharing the same original Styx callbacks.
 * RU: Фиксированные каталоги в один либо три уровня, выбранные при сборке, с общими callbacks исходной Styx.
 */
#include "catalog.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <string.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#ifdef CATALOG_TREE /* EN: Select the declared compile-time interface/profile. RU: Выбрать объявленный compile-time интерфейс или профиль. */
static const CatalogRow rows[] = { /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    {0, 0, CATALOG_DIR | 0555, "/", ""}, /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    {10, 0, CATALOG_DIR | 0555, "devices", ""}, /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    {11, 10, CATALOG_DIR | 0555, "sensor", ""}, /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    {12, 11, 0444, "temperature", "21.5\n"}, /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    {20, 0, CATALOG_DIR | 0555, "system", ""}, /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    {21, 20, 0444, "info", "standalone Styx example\n"}}; /* Immutable fixed hierarchy, no OS device dependencies. */
#else /* EN: Select the declared compile-time interface/profile. RU: Выбрать объявленный compile-time интерфейс или профиль. */
static const CatalogRow rows[] = {{0, 0, CATALOG_DIR | 0555, "/", ""}, /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
                                  {1, 0, 0444, "status", "ready\n"}, /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
                                  {2, 0, 0444, "temperature", "21.5\n"}}; /* Fixed root children. */
#endif /* EN: Select the declared compile-time interface/profile. RU: Выбрать объявленный compile-time интерфейс или профиль. */


/* EN/RU: Fixed catalogs need no source; return success without retaining the borrowed string.
 * Фиксированному каталогу не нужен source; вернуть успех, не удерживая строку.
 */


/* [standalone-contract:catalog_init]
 * EN: catalog_init: examples/fixed implementation and native state.
 * RU: catalog_init: реализация examples/fixed и нативное состояние.
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
    return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: Copy one stat or a bounded page of immediate children; arguments follow catalog.h.
 * Скопировать stat либо ограниченную страницу детей; контракт аргументов в catalog.h.
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
    size_t seen = 0; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    *count = 0; /* EN: Update *count for the current operation. RU: Обновить *count для текущей операции. */
    for (size_t index = 0; index < sizeof(rows) / sizeof(rows[0]); index++) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        if ((!listing && rows[index].id == id) || (listing && rows[index].id && rows[index].parent == id)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            if (seen++ >= start && *count < capacity) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            { /* EN: Begin this scope. RU: Начало области. */
                out[(*count)++] = rows[index]; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: Readonly catalog refuses every write; borrowed arguments are unused.
 * Каталог только для чтения отклоняет запись; заимствованные аргументы не используются.
 */


/* [standalone-contract:catalog_write]
 * EN: catalog_write: examples/fixed implementation and native state.
 * RU: catalog_write: реализация examples/fixed и нативное состояние.
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
 * EN: catalog_pin: examples/fixed implementation and native state.
 * RU: catalog_pin: реализация examples/fixed и нативное состояние.
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
 * EN: catalog_unpin: examples/fixed implementation and native state.
 * RU: catalog_unpin: реализация examples/fixed и нативное состояние.
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
