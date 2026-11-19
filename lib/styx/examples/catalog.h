/* Copyright (c) 2026 bhgv
 * EN: Bounded immediate-child backend contract; stable scalar IDs replace prebuilt trees.
 * RU: Контракт backend для ограниченных страниц непосредственных детей; стабильные ID вместо готового дерева.
 */
#ifndef EXAMPLE_CATALOG_H /* EN: Select the declared compile-time interface/profile. RU: Выбрать объявленный compile-time интерфейс или профиль. */
#define EXAMPLE_CATALOG_H /* EN: Select the declared compile-time interface/profile. RU: Выбрать объявленный compile-time интерфейс или профиль. */
#include <stdint.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stddef.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#define CATALOG_PAGE 16U         /* Maximum immediate children fetched by one backend query. */
#define CATALOG_DIR 0x80000000UL /* Standard 9P directory mode bit. */

/* EN/RU: Caller-owned copied metadata and bounded demonstration text, never a native wire structure.
 * Собственные скопированные metadata и ограниченный текст примера; не нативная wire-структура.
 */
typedef struct CatalogRow /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
{ /* EN: Begin this scope. RU: Начало области. */
    uint64_t id;        /* Stable backend object identity; root is zero. */
    uint64_t parent;    /* Stable parent identity; root's parent is root. */
    unsigned long mode; /* Standard directory and rwx bits. */
    char name[128];     /* One UTF-8 component, no slash or NUL inside. */
    char value[2048];   /* Demonstration UTF-8 contents; production backends may stream by offset. */
} CatalogRow; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */

/* EN: Initialize backend from optional source; 1/0 success, no ownership transfer of source.
 * RU: Инициализировать backend из необязательного source; 1/0, строка остаётся у вызывающего.
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
int catalog_init(const char *source); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */


/* EN: Fetch one stat row or a page of immediate children; return 1/0, fill count on success.
 * id/start and output are borrowed through return. stat requires capacity>=1; list returns count<=capacity.
 * No recursive traversal or whole-tree materialization. IDs must not alias different live objects.
 * RU: Получить stat либо страницу непосредственных детей; 1/0, count задаётся при успехе.
 * id/start/output заимствуются до возврата; stat требует capacity>=1, list выдаёт не больше capacity.
 * Без рекурсивного обхода и материализации дерева; ID разных живых объектов не должны совпадать.
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
int catalog_query(int listing, uint64_t id, size_t start, CatalogRow *output, size_t capacity, size_t *count); /* EN: Ask the backend only for the selected object/page. RU: Запросить у backend выбранный объект или страницу. */


/* EN: Apply one bounded control write; return accepted bytes or -1, borrowing input through return.
 * RU: Выполнить ограниченную запись управления; вернуть принятое число байт либо -1, input заимствован.
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
long catalog_write(uint64_t id, const char *input, size_t size, uint64_t offset); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */


/* EN: Pin one already validated open object,1/0; root/control backends may need no extra state.
 * A successful open has one matching unpin even after its name is hidden.
 * RU: Удержать проверенный открываемый объект,1/0; root/control backend может не требовать состояния.
 * Успешный open получает один unpin даже после скрытия имени.
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
int catalog_pin(uint64_t id); /* EN: Retain the opened object independently of its published name. RU: Удержать открытый объект независимо от имени. */


/* EN/RU: Release exactly one successful open's native pin; no name lookup or reopening.
 * Освободить ровно одно удержание успешного open; без поиска имени и переоткрытия.
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
void catalog_unpin(uint64_t id); /* EN: Release the opened-object hold. RU: Снять удержание открытого объекта. */
#endif /* EN: Select the declared compile-time interface/profile. RU: Выбрать объявленный compile-time интерфейс или профиль. */
