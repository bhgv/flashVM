/* Copyright (c) 2026 bhgv
 * EN: Select the managed call profile and walk the bounded schema grammar.
 * RU: Выбрать профиль типизированного вызова и пройти ограниченную схему типов.
 */
#include "managed_values.h"
#include "styxrpc/channel_call.h"
#include <string.h>

#define FVM_STYXRPC_MAX_DEPTH 6U /* Bound schema recursion as in the Dis codec. */


/* EN: Return the end of one supported type or NULL, within the terminated schema.
 * RU: Вернуть конец одного допустимого типа или NULL в ограниченной схеме. */
static const char *type_end(const char *type, unsigned depth)
{
    const char *next;
    if (depth >= FVM_STYXRPC_MAX_DEPTH || !*type)
    {
        return NULL;
    }
    switch (*type)
    {
    case 'i': case 'b': case 'y': case 's': case 'f': case 'h':
        return type + 1;
    case 'a': case 'c': case 'l':
        next = type[1] == '[' ? type_end(type + 2, depth + 1) : NULL;
        return next && *next == ']' ? next + 1 : NULL;
    case 'r':
        if (type[1] != '[')
        {
            return NULL;
        }
        next = type + 2;
        if (*next == ']')
        {
            return NULL;
        }
        while (next && *next && *next != ']')
        {
            next = type_end(next, depth + 1);
        }
        return next && *next == ']' ? next + 1 : NULL;
    case '(':
        next = type + 1;
        while (next && *next && *next != ')')
        {
            next = type_end(next, depth + 1);
        }
        return next && *next == ')' ? next + 1 : NULL;
    default:
        return NULL;
    }
}


/* EN/RU: See managed_values.h. / См. managed_values.h. */
const char *fvm_styxrpc_type_end(const char *type)
{
    return type_end(type, 0);
}


/* EN: Match the ordinary managed ref rules across the checked argument list.
 * RU: Проверить условия обычного профиля ref для списка аргументов. */
static int ref_slots(const char *type, unsigned *slots)
{
    const char *end = fvm_styxrpc_type_end(type);
    const char *child;
    if (*type == 'r')
    {
        unsigned fields = 0;
        for (child = type + 2; *child != ']'; child = fvm_styxrpc_type_end(child))
        {
            if (++fields > FVM_STYXRPC_REF_FIELDS)
            {
                return 0;
            }
        }
        return ++*slots <= FVM_STYXRPC_REF_SLOTS && !memchr(type + 1, 'r', (size_t)(end - type - 1));
    }
    if (*type == 'a' || *type == 'l')
    {
        return !memchr(type, 'r', (size_t)(end - type));
    }
    if (*type == '(')
    {
        for (child = type + 1; *child != ')'; child = fvm_styxrpc_type_end(child))
        {
            if (!ref_slots(child, slots))
            {
                return 0;
            }
        }
    }
    return 1;
}


/* EN: Verify two complete type lists and zero or one result.
 * RU: Проверить два полных списка типов и пустой либо одиночный результат. */
static int schema_supported(const StyxRpcSchema *schema)
{
    const char *args;
    const char *result;
    if (!schema || !memchr(schema->arguments, 0, sizeof(schema->arguments)) ||
        !memchr(schema->result, 0, sizeof(schema->result)) ||
        schema->arguments[0] != '(' || schema->result[0] != '(')
    {
        return 0;
    }
    /* EN: The native codec starts the depth of each item at zero, outside the list.
     * RU: Нативный кодек начинает глубину элемента с нуля, вне списка. */
    args = schema->arguments + 1;
    while (args && *args != ')')
    {
        args = type_end(args, 0);
    }
    result = schema->result[1] == ')' ? schema->result + 1 : type_end(schema->result + 1, 0);
    return args && !args[1] && result && *result == ')' && !result[1];
}


/* EN/RU: See managed_values.h. / См. managed_values.h. */
int fvm_styxrpc_values_supported(const StyxRpcSchema *schema)
{
    unsigned slots = 0;
    return schema_supported(schema) && !strpbrk(schema->result, "rhc") && !strpbrk(schema->arguments, "hc") &&
        ref_slots(schema->arguments, &slots);
}


/* EN/RU: See managed_values.h. / См. managed_values.h. */
int fvm_styxrpc_graph_supported(const StyxRpcSchema *schema)
{
    return schema_supported(schema) && !strchr(schema->arguments, 'c') && !strchr(schema->result, 'c') &&
        (strpbrk(schema->arguments, "rh") || strpbrk(schema->result, "rh"));
}


/* EN/RU: See managed_values.h. / См. managed_values.h. */
int fvm_styxrpc_channel_signature(const StyxRpcSchema *schema)
{
    return schema_supported(schema) && styxrpc_channel_schema_valid(schema);
}


/* EN/RU: See managed_values.h. / См. managed_values.h. */
int fvm_styxrpc_channel_supported(const StyxRpcSchema *schema)
{
    return fvm_styxrpc_channel_signature(schema) && !strchr(schema->arguments, 'c');
}
