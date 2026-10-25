/* Exercise immutable function views, nested grammar boundaries and cursor admission. */
#include <stdio.h>
#include <stdlib.h>
#include "signature.h"
#include <string.h>


/* Resolve a known module export while keeping all identity indices owner-local. */
static size_t export_index(const LimboInterface *owner, size_t scope, const char *name)
{
    size_t index = SIZE_MAX;
    if (!(limbo_interface_find(owner, scope, name, &index)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return index;
}


/* Source slices must name exactly the authored UTF-8 identifier, without trivia. */
static void parameter_name(const LimboInterface *owner, LimboParameter item, const char *expected)
{
    const char *source = limbo_tree_source(limbo_interface_tree(owner));
    if (!(item.name_end - item.name_begin == strlen(expected)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!memcmp(source + item.name_begin, expected, strlen(expected))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


/* Compare defined fields, avoiding any assumptions about C struct padding bytes. */
static void same_parameter(LimboParameter a, LimboParameter b)
{
    if (!(a.name_node == b.name_node && a.name_begin == b.name_begin && a.name_end == b.name_end))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(a.type_node == b.type_node && a.self_parameter == b.self_parameter))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


/* Failed view queries must leave each previously initialized public field alone. */
static void same_signature(LimboSignature a, LimboSignature b)
{
    if (!(a.type_node == b.type_node && a.parameters_node == b.parameters_node))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(a.result_node == b.result_node && a.parameter_count == b.parameter_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(a.variadic == b.variadic && a.polymorphic == b.polymorphic))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


/* Retain a catalog across another parse and inspect without touching parser globals. */
int main(void)
{
    const char source[] = "# original UTF-8 snapshot: Ж\n"
                          "Native: module {\n"
                          "  Alias: type int;\n"
                          "  add, other: fn(a, # comment within a grouped formal\n b: int, nil: byte): int;\n"
                          "  complex: fn(callback: fn(x: int): string, data: array of ref Alias): (int, string);\n"
                          "  log: fn(label: string, *);\n"
                          "  generic: fn[T](value: T): T;\n"
                          "  receiver: fn(me: self ref Alias);\n"
                          "  empty: fn();\n"
                          "  all: fn(a: big, b: real, c: list of byte, d: chan of string, e: ref Alias);\n"
                          "};";
    LimboInterface *owner = NULL, *another = NULL;
    LimboDiagnostic diagnostic;
    LimboSignature signature, saved;
    LimboParameter item, before, positional;
    size_t module, add, other, complex, generic, at, i;
    if (!(limbo_interface_parse(source, sizeof(source) - 1, &owner, &diagnostic)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_validate_functions(owner, &diagnostic)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    module = export_index(owner, SIZE_MAX, "Native");
    add = export_index(owner, module, "add");
    other = export_index(owner, module, "other");
    if (!(limbo_interface_signature(owner, add, &signature)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(signature.parameter_count == 3 && !signature.variadic && !signature.polymorphic))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(limbo_interface_type_kind(owner, signature.result_node), "int")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    saved = signature;
    if (!(limbo_interface_signature(owner, other, &signature)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(signature.type_node == saved.type_node))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    } /* Grouped exports share the authored fn. */
    at = SIZE_MAX;
    for (i = 0; i < 3; ++i)
    {
        if (!(limbo_interface_parameter_next(owner, add, at, &item)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(limbo_interface_parameter(owner, add, i, &positional)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(item.name_node == positional.name_node && item.type_node == positional.type_node))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        parameter_name(owner, item, i == 0 ? "a" : i == 1 ? "b" : "nil");
        if (!(!strcmp(limbo_interface_type_kind(owner, item.type_node), i < 2 ? "int" : "byte")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        at = item.name_node;
    }
    before = item;
    if (!(!limbo_interface_parameter_next(owner, add, at, &item)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    same_parameter(before, item);
    if (!(!limbo_interface_parameter(owner, add, 3, &item)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    complex = export_index(owner, module, "complex");
    if (!(limbo_interface_signature(owner, complex, &signature)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(signature.parameter_count == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(limbo_interface_type_kind(owner, signature.result_node), "tuple")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_parameter(owner, complex, 0, &item)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(limbo_interface_type_kind(owner, item.type_node), "fn")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_parameter(owner, complex, 1, &item)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(limbo_interface_type_kind(owner, item.type_node), "array")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = item;
    if (!(!limbo_interface_parameter_next(owner, add, item.name_node, &item)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    same_parameter(before, item);
    at = export_index(owner, module, "log");
    if (!(limbo_interface_signature(owner, at, &signature)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(signature.parameter_count == 1 && signature.variadic && signature.result_node == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    generic = export_index(owner, module, "generic");
    if (!(limbo_interface_signature(owner, generic, &signature) && signature.polymorphic))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(limbo_interface_type_kind(owner, signature.result_node), "named")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    at = export_index(owner, module, "receiver");
    if (!(limbo_interface_parameter(owner, at, 0, &item) && item.self_parameter))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_type_kind(owner, item.type_node) == NULL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    } /* It is a NamedType. */
    at = export_index(owner, module, "empty");
    if (!(limbo_interface_signature(owner, at, &signature)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(signature.parameter_count == 0 && signature.result_node == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = item;
    if (!(!limbo_interface_parameter_next(owner, at, SIZE_MAX, &item)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    same_parameter(before, item);
    saved = signature;
    if (!(!limbo_interface_signature(owner, module, &signature)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    same_signature(saved, signature);
    if (!(!limbo_interface_signature(NULL, 0, &signature)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_interface_signature(owner, SIZE_MAX, &signature)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_interface_signature(owner, add, NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_type_kind(owner, 0) == NULL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_parse("X: module {};", 13, &another, &diagnostic)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_signature(owner, add, &signature) && signature.parameter_count == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    limbo_interface_destroy(another);
    limbo_interface_destroy(owner);
    owner = NULL;
    {
        const char bad[] = "N: module { f: fn(ж: int, a: byte, ж: string); };";
        const char *duplicate = strstr(strstr(bad, "ж") + strlen("ж"), "ж");
        if (!(limbo_interface_parse(bad, sizeof(bad) - 1, &owner, &diagnostic)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!limbo_interface_validate_functions(owner, &diagnostic)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(diagnostic.code == -9 && diagnostic.offset == (long)(duplicate - bad)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(diagnostic.line == 1 && diagnostic.column == diagnostic.offset + 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(limbo_interface_signature(owner, 1, &signature) && signature.parameter_count == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        limbo_interface_destroy(owner);
        owner = NULL;
    }
    {
        const char placeholders[] = "N: module { f: fn(nil, nil: int); };";
        if (!(limbo_interface_parse(placeholders, sizeof(placeholders) - 1, &owner, &diagnostic)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(limbo_interface_validate_functions(owner, &diagnostic)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(diagnostic.code == 0 && diagnostic.message[0] == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(limbo_interface_validate_functions(owner, NULL)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        limbo_interface_destroy(owner);
    }
    if (!(!limbo_interface_validate_functions(NULL, &diagnostic) && diagnostic.code == -1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
