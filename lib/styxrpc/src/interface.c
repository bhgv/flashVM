/* Copyright (c) 2026 bhgv
 * EN: Emit a bounded Limbo source module from known RPC types and ADT field names.
 * RU: Вывод ограниченного модуля Limbo из типов RPC и имён полей ADT.
 */
#include "styxrpc/interface.h"
#include "load_profile.h"
#include <string.h>
#include <stdio.h>

#define IDENTIFIER_LIMIT 63U /* Public source identifier limit. */
#define ADT_LIMIT 16U        /* Bounded declarations per interface. */

typedef struct Output
{
    char *bytes;      /* NULL for a measurement pass. */
    size_t length;    /* Written or measured bytes, excluding NUL. */
    const StyxRpcAdt *adts; /* Read-only names for reference types. */
    size_t count;     /* Number of ADT bindings. */
} Output;


/* EN: Check an ASCII Limbo identifier and reject language keywords.
 * RU: Проверить идентификатор ASCII Limbo и исключить ключевые слова. */
static int identifier(const char *value)
{
    static const char alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";
    static const char all[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789";
    static const char *const reserved[] = {
        "adt", "alt", "array", "big", "break", "byte", "case", "chan", "con", "continue",
        "cyclic", "do", "dynamic", "else", "exception", "exit", "fixed", "fn", "for", "hd", "if", "implement",
        "import", "include", "int", "len", "list", "load", "module", "nil", "of", "or",
        "pick", "raise", "raises", "real", "ref", "return", "self", "spawn", "string", "tagof",
        "tl", "to", "type", "while", "table", "hash", "hash_table", "hashtable"
    };
    if (!value || !*value || !strchr(alpha, *value) || strspn(value, all) != strlen(value) ||
        strlen(value) > IDENTIFIER_LIMIT)
    {
        return 0;
    }
    for (size_t i = 0; i < sizeof(reserved) / sizeof(*reserved); ++i)
    {
        if (!strcmp(value, reserved[i]))
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Append known bounded text, or measure it without writing.
 * RU: Добавить ограниченный текст либо измерить его без записи. */
static void add(Output *out, const char *text)
{
    size_t size = strlen(text);
    if (out->bytes)
    {
        memcpy(out->bytes + out->length, text, size);
    }
    out->length += size;
}


/* EN: Write a single wire type as Limbo source, consuming exactly that token.
 * RU: Записать один сетевой тип как Limbo, поглощая ровно его токен. */
static int render_type(Output *out, const char **cursor)
{
    const char *start = *cursor;
    const char *end = styxshape_end(start);
    if (!end)
    {
        return 0;
    }
    *cursor = end;
    switch (*start)
    {
    case 'i':
        add(out, "int");
        return 1;
    case 'b':
        add(out, "big");
        return 1;
    case 'y':
        add(out, "byte");
        return 1;
    case 'f':
        add(out, "real");
        return 1;
    case 's':
        add(out, "string");
        return 1;
    case 'a':
    case 'l':
    case 'c':
        add(out, *start == 'a' ? "array of " : *start == 'l' ? "list of " : "chan of ");
        start += 2;
        return render_type(out, &start);
    case '(':
        add(out, "(");
        ++start;
        while (start < end - 1)
        {
            if (!render_type(out, &start))
            {
                return 0;
            }
            if (start < end - 1)
            {
                add(out, ", ");
            }
        }
        add(out, ")");
        return 1;
    case 'r':
        for (size_t i = 0; i < out->count; ++i)
        {
            if (strlen(out->adts[i].shape) == (size_t)(end - start) &&
                !memcmp(out->adts[i].shape, start, (size_t)(end - start)))
            {
                add(out, "ref ");
                add(out, out->adts[i].name);
                return 1;
            }
        }
        return 0;
    default:
        return 0;
    }
}


/* EN: Validate ADT definitions and render their named fields in order.
 * RU: Проверить объявления ADT и вывести именованные поля по порядку. */
static int render_adt(Output *out, const StyxRpcAdt *adt)
{
    const char *shape = adt->shape + 2;
    add(out, "    ");
    add(out, adt->name);
    add(out, ": adt {\n");
    for (size_t i = 0; i < adt->count; ++i)
    {
        if (!identifier(adt->fields[i]))
        {
            return 0;
        }
        for (size_t previous = 0; previous < i; ++previous)
        {
            if (!strcmp(adt->fields[previous], adt->fields[i]))
            {
                return 0;
            }
        }
        add(out, "        ");
        add(out, adt->fields[i]);
        add(out, ": ");
        if (!render_type(out, &shape))
        {
            return 0;
        }
        add(out, ";\n");
    }
    add(out, "    };\n");
    return !strcmp(shape, "]");
}


/* EN: Render one verified function schema with sign-neutral argument names.
 * RU: Вывести одну проверенную форму функции с нейтральными для сигнатуры именами. */
static int render_function(Output *out, const StyxRpcDeclaration *function)
{
    StyxCallShape shape;
    if (!function->schema || !identifier(function->name))
    {
        return 0;
    }
    memcpy(shape.arguments, function->schema->arguments, sizeof(shape.arguments));
    memcpy(shape.result, function->schema->result, sizeof(shape.result));
    if (!styxshapevalid(&shape))
    {
        return 0;
    }
    add(out, "    ");
    add(out, function->name);
    add(out, ": fn(");
    const char *argument = shape.arguments + 1;
    unsigned index = 0;
    while (*argument != ')')
    {
        char name[sizeof("arg4294967295: ")];
        if (index)
        {
            add(out, ", ");
        }
        snprintf(name, sizeof(name), "arg%u: ", index++);
        add(out, name);
        if (!render_type(out, &argument))
        {
            return 0;
        }
    }
    add(out, ")");
    const char *result = shape.result + 1;
    if (*result != ')')
    {
        add(out, ": ");
        if (!render_type(out, &result))
        {
            return 0;
        }
    }
    add(out, ";\n");
    return 1;
}


/* EN: Measure or render a full source module after checking names and type definitions.
 * RU: Измерить или вывести полный исходник после проверки имён и типов. */
static int render(Output *out, const char *module, const StyxRpcDeclaration *functions, size_t count)
{
    add(out, module);
    add(out, ": module {\n");
    for (size_t i = 0; i < out->count; ++i)
    {
        if (!render_adt(out, &out->adts[i]))
        {
            return 0;
        }
    }
    for (size_t i = 0; i < count; ++i)
    {
        if (!render_function(out, &functions[i]))
        {
            return 0;
        }
        for (size_t j = 0; j < i; ++j)
        {
            if (!strcmp(functions[i].name, functions[j].name))
            {
                return 0;
            }
        }
        for (size_t j = 0; j < out->count; ++j)
        {
            if (!strcmp(functions[i].name, out->adts[j].name))
            {
                return 0;
            }
        }
    }
    add(out, "};\n");
    return 1;
}


/* EN: Validate all ADT bindings before type rendering may look them up.
 * RU: Проверить все привязки ADT до поиска при выводе типов. */
static int valid_adts(const StyxRpcAdt *adts, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        const StyxRpcAdt *adt = &adts[i];
        const char *end = adt->shape && strlen(adt->shape) <= STYX_SHAPE_MAX ?
            styxshape_end(adt->shape) : NULL;
        if (!end || *end || adt->shape[0] != 'r' || !identifier(adt->name) || !adt->count || adt->count > STYX_SHAPE_MAX || !adt->fields)
        {
            return 0;
        }
        for (size_t j = 0; j < i; ++j)
        {
            if (!strcmp(adt->shape, adts[j].shape) || !strcmp(adt->name, adts[j].name))
            {
                return 0;
            }
        }
    }
    return 1;
}


/* EN/RU: See interface.h. / См. interface.h. */
StyxRpcStatus styxrpc_module_source(const char *module, const StyxRpcDeclaration *functions, size_t count,
    const StyxRpcAdt *adts, size_t adt_count, char *bytes, size_t capacity, size_t *used)
{
    if (!used)
    {
        return STYXRPC_INVALID;
    }
    *used = 0;
    if (!identifier(module) || !functions || !count || count > STYXRPC_MAX_EXPORTS ||
        adt_count > ADT_LIMIT || (adt_count && !adts) || (!bytes && capacity) || !valid_adts(adts, adt_count))
    {
        return STYXRPC_INVALID;
    }
    Output measured = {.adts = adts, .count = adt_count};
    if (!render(&measured, module, functions, count))
    {
        return STYXRPC_INVALID;
    }
    if (bytes)
    {
        if (capacity <= measured.length)
        {
            return STYXRPC_INVALID;
        }
        Output output = {.bytes = bytes, .adts = adts, .count = adt_count};
        if (!render(&output, module, functions, count))
        {
            return STYXRPC_INVALID;
        }
        bytes[output.length] = '\0';
    }
    *used = measured.length;
    return STYXRPC_OK;
}
