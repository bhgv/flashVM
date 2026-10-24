/* Query authoritative production boundaries; this is not another Limbo parser. */
#include "signature.h"
#include <string.h>
#include <stdlib.h>

/* Find one direct child without confusing nested function parameter/result types. */
static size_t child(const LimboTree *tree, size_t parent, const char *kind)
{
    LimboNode node;
    size_t at;
    if (!limbo_tree_node(tree, parent, &node))
        return SIZE_MAX;
    at = node.first_child;
    while (limbo_tree_node(tree, at, &node))
    {
        if (!strcmp(node.kind, kind))
            return at;
        at = node.next_sibling;
    }
    return SIZE_MAX;
}

/* Obtain the declaration's exact Type production, using authored begin offsets. */
static size_t function_type(const LimboInterface *owner, size_t declaration)
{
    const LimboTree *tree = limbo_interface_tree(owner);
    LimboDeclaration item;
    LimboNode node;
    size_t at;
    if (!limbo_interface_declaration(owner, declaration, &item) || item.kind != LIMBO_FUNCTION)
        return SIZE_MAX;
    for (at = item.node; limbo_tree_node(tree, at, &node); ++at)
    {
        if (node.begin > item.type_begin)
            break;
        if (node.begin == item.type_begin && !strcmp(node.kind, "Type"))
            return at;
    }
    return SIZE_MAX;
}

/* Walk only the fixed-formal Names children; comments/grouping remain tree-owned. */
static size_t names_count(const LimboTree *tree, size_t formal)
{
    size_t names = child(tree, formal, "Names"), count = 0, at;
    LimboNode node;
    if (!limbo_tree_node(tree, names, &node))
        return 0;
    for (at = node.first_child; limbo_tree_node(tree, at, &node); at = node.next_sibling)
        if (!strcmp(node.kind, "Name"))
            ++count;
    return count;
}

/* Copy structural arity and direct result without interpreting named type bindings. */
int limbo_interface_signature(const LimboInterface *owner, size_t declaration, LimboSignature *out)
{
    const LimboTree *tree = limbo_interface_tree(owner);
    LimboSignature result;
    LimboNode node;
    size_t at, poly;
    if (!out)
        return 0;
    memset(&result, 0, sizeof(result));
    result.type_node = function_type(owner, declaration);
    if (result.type_node == SIZE_MAX)
        return 0;
    result.parameters_node = child(tree, result.type_node, "Parameters");
    result.result_node = child(tree, result.type_node, "Type");
    poly = child(tree, result.type_node, "PolyParameters");
    result.polymorphic = child(tree, poly, "Names") != SIZE_MAX;
    if (!limbo_tree_node(tree, result.parameters_node, &node))
        return 0;
    for (at = node.first_child; limbo_tree_node(tree, at, &node); at = node.next_sibling)
    {
        size_t count = names_count(tree, at);
        if (!strcmp(node.kind, "Formal"))
        {
            result.parameter_count += count;
            if (!count)
                result.variadic = 1;
        }
    }
    *out = result;
    return 1;
}

/* Expand grouped names left to right without allocating or retaining caller storage. */
int limbo_interface_parameter(const LimboInterface *owner, size_t declaration, size_t parameter,
                              LimboParameter *out)
{
    const LimboTree *tree = limbo_interface_tree(owner);
    LimboSignature signature;
    LimboNode node, name;
    LimboParameter result;
    size_t formal, names, at;
    if (!out || !limbo_interface_signature(owner, declaration, &signature) ||
        parameter >= signature.parameter_count)
        return 0;
    limbo_tree_node(tree, signature.parameters_node, &node);
    for (formal = node.first_child; limbo_tree_node(tree, formal, &node); formal = node.next_sibling)
    {
        size_t count = names_count(tree, formal);
        if (parameter >= count)
        {
            parameter -= count;
            continue;
        }
        names = child(tree, formal, "Names");
        limbo_tree_node(tree, names, &name);
        at = name.first_child;
        while (parameter--)
        {
            limbo_tree_node(tree, at, &name);
            at = name.next_sibling;
        }
        limbo_tree_node(tree, at, &name);
        result.name_node = at;
        result.name_begin = name.begin;
        result.name_end = name.end;
        result.type_node = child(tree, formal, "Type");
        result.self_parameter = result.type_node == SIZE_MAX;
        if (result.self_parameter)
            result.type_node = child(tree, formal, "NamedType");
        *out = result;
        return 1;
    }
    return 0;
}

/* Advance via immutable sibling/parent links, validating the cursor's owner chain. */
int limbo_interface_parameter_next(const LimboInterface *owner, size_t declaration,
                                   size_t previous_name, LimboParameter *out)
{
    const LimboTree *tree = limbo_interface_tree(owner);
    LimboNode node, names, formal;
    LimboParameter result;
    size_t type = function_type(owner, declaration);
    size_t parameters = child(tree, type, "Parameters"), at;
    if (!out || parameters == SIZE_MAX)
        return 0;
    if (previous_name == SIZE_MAX)
    {
        limbo_tree_node(tree, parameters, &node);
        at = node.first_child;
    }
    else
    {
        if (!limbo_tree_node(tree, previous_name, &node) || strcmp(node.kind, "Name") ||
            !limbo_tree_node(tree, node.parent, &names) || strcmp(names.kind, "Names") ||
            !limbo_tree_node(tree, names.parent, &formal) || strcmp(formal.kind, "Formal") ||
            formal.parent != parameters)
            return 0;
        if (node.next_sibling != SIZE_MAX)
            at = names.parent;
        else
            at = formal.next_sibling;
        if (node.next_sibling != SIZE_MAX)
        {
            result.name_node = node.next_sibling;
            goto found;
        }
    }
    while (limbo_tree_node(tree, at, &formal))
    {
        size_t group = child(tree, at, "Names");
        if (limbo_tree_node(tree, group, &names) && names.first_child != SIZE_MAX)
        {
            result.name_node = names.first_child;
            goto found;
        }
        at = formal.next_sibling;
    }
    return 0;
found:
    limbo_tree_node(tree, result.name_node, &node);
    result.name_begin = node.begin;
    result.name_end = node.end;
    result.type_node = child(tree, at, "Type");
    result.self_parameter = result.type_node == SIZE_MAX;
    if (result.self_parameter)
        result.type_node = child(tree, at, "NamedType");
    *out = result;
    return 1;
}

/* Type begins at its accepted introducer. Inspect only that token, not type syntax. */
const char *limbo_interface_type_kind(const LimboInterface *owner, size_t type_node)
{
    static const char *const words[] = {
        "byte", "int", "big", "real", "string", "array", "list", "chan", "ref", "fn"
    };
    const LimboTree *tree = limbo_interface_tree(owner);
    const char *source = limbo_tree_source(tree);
    LimboNode node;
    size_t i;
    if (!limbo_tree_node(tree, type_node, &node) || strcmp(node.kind, "Type"))
        return NULL;
    if (child(tree, type_node, "NamedType") != SIZE_MAX)
        return "named";
    if (child(tree, type_node, "FixedType") != SIZE_MAX)
        return "fixed";
    if (child(tree, type_node, "TypeList") != SIZE_MAX)
        return "tuple";
    for (i = 0; i < sizeof(words) / sizeof(words[0]); ++i)
    {
        size_t length = strlen(words[i]);
        if (node.end - node.begin >= length && !memcmp(source + node.begin, words[i], length))
            return words[i];
    }
    return NULL;
}

/* Temporary exact UTF-8 names for one function; nil is intentionally not a binding. */
typedef struct ParameterKey
{
    const char *text;
    size_t length;
    size_t offset;
} ParameterKey;

/* Sort names without locale/global state, then authored order for duplicate diagnostics. */
static int compare_parameters(const void *left, const void *right)
{
    const ParameterKey *a = (const ParameterKey *)left, *b = (const ParameterKey *)right;
    size_t common = a->length < b->length ? a->length : b->length;
    int order = memcmp(a->text, b->text, common);
    if (order)
        return order;
    if (a->length != b->length)
        return a->length < b->length ? -1 : 1;
    return a->offset < b->offset ? -1 : a->offset != b->offset;
}

/* Populate one authored byte diagnostic without parser globals or retained output pointers. */
static int invalid_function(LimboDiagnostic *diagnostic, int code, const char *source,
                            size_t offset, const char *message)
{
    size_t i;
    diagnostic->code = code;
    diagnostic->offset = (long)offset;
    diagnostic->line = diagnostic->column = 1;
    for (i = 0; i < offset; ++i)
    {
        if (source[i] == '\n')
        {
            ++diagnostic->line;
            diagnostic->column = 1;
        }
        else
            ++diagnostic->column;
    }
    snprintf(diagnostic->message, sizeof(diagnostic->message), "%s", message);
    return 0;
}

/* Check fixed parameter bindings, with per-function temporary ownership only.
 * Alias/type/layout/receiver/constraint validity is deliberately a separate pass. */
int limbo_interface_validate_functions(const LimboInterface *owner, LimboDiagnostic *diagnostic)
{
    LimboDiagnostic local;
    const char *source = limbo_tree_source(limbo_interface_tree(owner));
    size_t declaration, previous_type = SIZE_MAX;
    if (!diagnostic)
        diagnostic = &local;
    memset(diagnostic, 0, sizeof(*diagnostic));
    if (!owner)
        return invalid_function(diagnostic, -1, "", 0, "interface owner is required");
    for (declaration = 0; declaration < limbo_interface_count(owner); ++declaration)
    {
        LimboSignature signature;
        LimboParameter parameter;
        ParameterKey *keys;
        size_t previous = SIZE_MAX, count = 0, i, duplicate = SIZE_MAX;
        if (!limbo_interface_signature(owner, declaration, &signature))
            continue;
        if (signature.type_node == previous_type)
            continue; /* Grouped exports share one authored signature. */
        previous_type = signature.type_node;
        if (!signature.parameter_count)
            continue;
        if (signature.parameter_count > SIZE_MAX / sizeof(*keys))
            return invalid_function(diagnostic, -2, source, 0, "parameter storage limit exceeded");
        keys = (ParameterKey *)malloc(signature.parameter_count * sizeof(*keys));
        if (!keys)
            return invalid_function(diagnostic, -2, source, 0, "parameter validation allocation failure");
        while (limbo_interface_parameter_next(owner, declaration, previous, &parameter))
        {
            size_t length = parameter.name_end - parameter.name_begin;
            previous = parameter.name_node;
            if (length == 3 && !memcmp(source + parameter.name_begin, "nil", 3))
                continue;
            keys[count].text = source + parameter.name_begin;
            keys[count].length = length;
            keys[count++].offset = parameter.name_begin;
        }
        qsort(keys, count, sizeof(*keys), compare_parameters);
        for (i = 1; i < count; ++i)
            if (keys[i - 1].length == keys[i].length &&
                !memcmp(keys[i - 1].text, keys[i].text, keys[i].length) && keys[i].offset < duplicate)
                duplicate = keys[i].offset;
        free(keys);
        if (duplicate != SIZE_MAX)
            return invalid_function(diagnostic, -9, source, duplicate,
                                    "duplicate fixed parameter name in function signature");
    }
    return 1;
}
