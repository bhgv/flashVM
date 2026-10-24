/* Exercise immutable function views, nested grammar boundaries and cursor admission. */
#include "signature.h"
#include <assert.h>
#include <string.h>

/* Resolve a known module export while keeping all identity indices owner-local. */
static size_t export_index(const LimboInterface *owner, size_t scope, const char *name)
{
    size_t index = SIZE_MAX;
    assert(limbo_interface_find(owner, scope, name, &index));
    return index;
}

/* Source slices must name exactly the authored UTF-8 identifier, without trivia. */
static void parameter_name(const LimboInterface *owner, LimboParameter item, const char *expected)
{
    const char *source = limbo_tree_source(limbo_interface_tree(owner));
    assert(item.name_end - item.name_begin == strlen(expected));
    assert(!memcmp(source + item.name_begin, expected, strlen(expected)));
}

/* Compare defined fields, avoiding any assumptions about C struct padding bytes. */
static void same_parameter(LimboParameter a, LimboParameter b)
{
    assert(a.name_node == b.name_node && a.name_begin == b.name_begin && a.name_end == b.name_end);
    assert(a.type_node == b.type_node && a.self_parameter == b.self_parameter);
}

/* Failed view queries must leave each previously initialized public field alone. */
static void same_signature(LimboSignature a, LimboSignature b)
{
    assert(a.type_node == b.type_node && a.parameters_node == b.parameters_node);
    assert(a.result_node == b.result_node && a.parameter_count == b.parameter_count);
    assert(a.variadic == b.variadic && a.polymorphic == b.polymorphic);
}

/* Retain a catalog across another parse and inspect without touching parser globals. */
int main(void)
{
    const char source[] =
        "# original UTF-8 snapshot: Ж\n"
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
    assert(limbo_interface_parse(source, sizeof(source) - 1, &owner, &diagnostic));
    assert(limbo_interface_validate_functions(owner, &diagnostic));
    module = export_index(owner, SIZE_MAX, "Native");
    add = export_index(owner, module, "add");
    other = export_index(owner, module, "other");
    assert(limbo_interface_signature(owner, add, &signature));
    assert(signature.parameter_count == 3 && !signature.variadic && !signature.polymorphic);
    assert(!strcmp(limbo_interface_type_kind(owner, signature.result_node), "int"));
    saved = signature;
    assert(limbo_interface_signature(owner, other, &signature));
    assert(signature.type_node == saved.type_node); /* Grouped exports share the authored fn. */
    at = SIZE_MAX;
    for (i = 0; i < 3; ++i)
    {
        assert(limbo_interface_parameter_next(owner, add, at, &item));
        assert(limbo_interface_parameter(owner, add, i, &positional));
        assert(item.name_node == positional.name_node && item.type_node == positional.type_node);
        parameter_name(owner, item, i == 0 ? "a" : i == 1 ? "b" : "nil");
        assert(!strcmp(limbo_interface_type_kind(owner, item.type_node), i < 2 ? "int" : "byte"));
        at = item.name_node;
    }
    before = item;
    assert(!limbo_interface_parameter_next(owner, add, at, &item));
    same_parameter(before, item);
    assert(!limbo_interface_parameter(owner, add, 3, &item));
    complex = export_index(owner, module, "complex");
    assert(limbo_interface_signature(owner, complex, &signature));
    assert(signature.parameter_count == 2);
    assert(!strcmp(limbo_interface_type_kind(owner, signature.result_node), "tuple"));
    assert(limbo_interface_parameter(owner, complex, 0, &item));
    assert(!strcmp(limbo_interface_type_kind(owner, item.type_node), "fn"));
    assert(limbo_interface_parameter(owner, complex, 1, &item));
    assert(!strcmp(limbo_interface_type_kind(owner, item.type_node), "array"));
    before = item;
    assert(!limbo_interface_parameter_next(owner, add, item.name_node, &item));
    same_parameter(before, item);
    at = export_index(owner, module, "log");
    assert(limbo_interface_signature(owner, at, &signature));
    assert(signature.parameter_count == 1 && signature.variadic && signature.result_node == SIZE_MAX);
    generic = export_index(owner, module, "generic");
    assert(limbo_interface_signature(owner, generic, &signature) && signature.polymorphic);
    assert(!strcmp(limbo_interface_type_kind(owner, signature.result_node), "named"));
    at = export_index(owner, module, "receiver");
    assert(limbo_interface_parameter(owner, at, 0, &item) && item.self_parameter);
    assert(limbo_interface_type_kind(owner, item.type_node) == NULL); /* It is a NamedType. */
    at = export_index(owner, module, "empty");
    assert(limbo_interface_signature(owner, at, &signature));
    assert(signature.parameter_count == 0 && signature.result_node == SIZE_MAX);
    before = item;
    assert(!limbo_interface_parameter_next(owner, at, SIZE_MAX, &item));
    same_parameter(before, item);
    saved = signature;
    assert(!limbo_interface_signature(owner, module, &signature));
    same_signature(saved, signature);
    assert(!limbo_interface_signature(NULL, 0, &signature));
    assert(!limbo_interface_signature(owner, SIZE_MAX, &signature));
    assert(!limbo_interface_signature(owner, add, NULL));
    assert(limbo_interface_type_kind(owner, 0) == NULL);
    assert(limbo_interface_parse("X: module {};", 13, &another, &diagnostic));
    assert(limbo_interface_signature(owner, add, &signature) && signature.parameter_count == 3);
    limbo_interface_destroy(another);
    limbo_interface_destroy(owner);
    owner = NULL;
    {
        const char bad[] = "N: module { f: fn(ж: int, a: byte, ж: string); };";
        const char *duplicate = strstr(strstr(bad, "ж") + strlen("ж"), "ж");
        assert(limbo_interface_parse(bad, sizeof(bad) - 1, &owner, &diagnostic));
        assert(!limbo_interface_validate_functions(owner, &diagnostic));
        assert(diagnostic.code == -9 && diagnostic.offset == (long)(duplicate - bad));
        assert(diagnostic.line == 1 && diagnostic.column == diagnostic.offset + 1);
        assert(limbo_interface_signature(owner, 1, &signature) && signature.parameter_count == 3);
        limbo_interface_destroy(owner);
        owner = NULL;
    }
    {
        const char placeholders[] = "N: module { f: fn(nil, nil: int); };";
        assert(limbo_interface_parse(placeholders, sizeof(placeholders) - 1, &owner, &diagnostic));
        assert(limbo_interface_validate_functions(owner, &diagnostic));
        assert(diagnostic.code == 0 && diagnostic.message[0] == 0);
        assert(limbo_interface_validate_functions(owner, NULL));
        limbo_interface_destroy(owner);
    }
    assert(!limbo_interface_validate_functions(NULL, &diagnostic) && diagnostic.code == -1);
    return 0;
}
