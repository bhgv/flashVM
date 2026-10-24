/* Build explicit declaration metadata from the authoritative grammar's production tree. */
#include "interface.h"
#include <stdlib.h>
#include <string.h>

/* Keep source ownership and bounded declaration storage behind one destroy operation. */
struct LimboInterface
{
    LimboTree *tree;
    LimboDeclaration *declarations;
    size_t count;
    size_t capacity;
    struct NameKey *keys;
    size_t key_count;
};

/* Sorting these copied keys avoids global comparator state and quadratic name lookup. */
typedef struct NameKey
{
    size_t scope;
    const char *name;
    size_t declaration;
} NameKey;

/* Report the first semantic-catalog error with an authored byte offset. */
static int reject(LimboDiagnostic *diagnostic, int code, size_t offset, const char *message)
{
    diagnostic->code = code;
    diagnostic->offset = (long)offset;
    snprintf(diagnostic->message, sizeof(diagnostic->message), "%s", message);
    return 0;
}

/* Skip only trivia already accepted by Coco/R; do not recognize expressions or types here. */
static size_t trivia(const char *source, size_t at, size_t end)
{
    while (at < end)
    {
        unsigned char c = (unsigned char)source[at];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            ++at;
        else if (c == '#')
        {
            while (at < end && source[at] != '\n')
                ++at;
        }
        else
            break;
    }
    return at;
}

/* Return the byte extent of one already-validated grammar identifier. */
static size_t identifier_end(const char *source, size_t at, size_t end)
{
    while (at < end)
    {
        unsigned char c = (unsigned char)source[at];
        if (!(c >= 128 || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_'))
            break;
        ++at;
    }
    return at;
}

/* Search direct production children only, preserving grammar boundaries and nested scopes. */
static size_t child(const LimboTree *tree, const LimboNode *parent, const char *kind)
{
    size_t index = parent->first_child;
    LimboNode node;
    while (index != SIZE_MAX)
    {
        if (!limbo_tree_node(tree, index, &node))
            return SIZE_MAX;
        if (!strcmp(node.kind, kind))
            return index;
        index = node.next_sibling;
    }
    return SIZE_MAX;
}

/* Distinguish literal declaration introducers without interpreting an alias as its target. */
static LimboDeclKind category(const char *source, size_t begin, size_t end)
{
    static const char *const words[] = {"module", "adt",       "fn",    "con",
                                        "type",   "exception", "import"};
    size_t i, stop = identifier_end(source, begin, end);
    for (i = 0; i < sizeof(words) / sizeof(words[0]); ++i)
        if (stop - begin == strlen(words[i]) && !memcmp(source + begin, words[i], stop - begin))
            return (LimboDeclKind)i;
    return LIMBO_VALUE;
}

/* Append one owned name while keeping all public links stable across reallocations. */
static int append(LimboInterface *interface, LimboDeclaration declaration,
                  LimboDiagnostic *diagnostic)
{
    const char *source = limbo_tree_source(interface->tree);
    size_t length = declaration.name_end - declaration.name_begin;
    char *name;
    if (interface->count == 16384)
        return reject(diagnostic, -8, declaration.name_begin,
                      "interface declaration limit exceeded");
    if (interface->count == interface->capacity)
    {
        size_t capacity = interface->capacity ? interface->capacity * 2 : 64;
        LimboDeclaration *grown;
        if (capacity > SIZE_MAX / sizeof(*grown))
            return reject(diagnostic, -8, declaration.name_begin,
                          "interface storage limit exceeded");
        grown = (LimboDeclaration *)realloc(interface->declarations, capacity * sizeof(*grown));
        if (!grown)
            return reject(diagnostic, -2, declaration.name_begin, "interface allocation failure");
        interface->declarations = grown;
        interface->capacity = capacity;
    }
    name = (char *)malloc(length + 1);
    if (!name)
        return reject(diagnostic, -2, declaration.name_begin, "name allocation failure");
    memcpy(name, source + declaration.name_begin, length);
    name[length] = 0;
    declaration.name = name;
    interface->declarations[interface->count++] = declaration;
    return 1;
}

/* Order exact scope/name identities, then source order for deterministic duplicate diagnostics. */
static int compare_names(const void *left, const void *right)
{
    const NameKey *a = (const NameKey *)left, *b = (const NameKey *)right;
    int order;
    if (a->scope != b->scope)
        return a->scope < b->scope ? -1 : 1;
    order = strcmp(a->name, b->name);
    if (order)
        return order;
    return a->declaration < b->declaration ? -1 : a->declaration != b->declaration;
}

/* Reject ambiguous exported names without rearranging the source-order public catalog. */
static int unique_names(LimboInterface *interface, LimboDiagnostic *diagnostic)
{
    NameKey *keys;
    size_t i, count = 0, duplicate = SIZE_MAX;
    if (!interface->count)
        return 1;
    if (interface->count > SIZE_MAX / sizeof(*keys))
        return reject(diagnostic, -8, 0, "name index storage limit exceeded");
    keys = (NameKey *)malloc(interface->count * sizeof(*keys));
    if (!keys)
        return reject(diagnostic, -2, 0, "duplicate-check allocation failure");
    for (i = 0; i < interface->count; ++i)
    {
        const LimboDeclaration *declaration = &interface->declarations[i];
        if (declaration->kind == LIMBO_INCLUDE)
            continue;
        keys[count].scope = declaration->scope;
        keys[count].name = declaration->name;
        keys[count++].declaration = i;
    }
    qsort(keys, count, sizeof(*keys), compare_names);
    for (i = 1; i < count; ++i)
        if (keys[i - 1].scope == keys[i].scope && !strcmp(keys[i - 1].name, keys[i].name) &&
            keys[i].declaration < duplicate)
            duplicate = keys[i].declaration;
    if (duplicate != SIZE_MAX)
    {
        free(keys);
        return reject(diagnostic, -7, interface->declarations[duplicate].name_begin,
                      "duplicate interface declaration in the same scope");
    }
    interface->keys = keys;
    interface->key_count = count;
    return 1;
}

/* Catalog a Top declaration using its direct NamedTop/DeclarationTail/DeclType chain. */
static int collect(LimboInterface *interface, size_t index, const LimboNode *top, size_t *owners,
                   LimboDiagnostic *diagnostic)
{
    const LimboTree *tree = interface->tree;
    const char *source = limbo_tree_source(tree);
    LimboNode named, tail, type;
    LimboDeclaration declaration;
    size_t named_index, tail_index, type_index, at, end, first = interface->count;
    memset(&declaration, 0, sizeof(declaration));
    declaration.scope = top->parent == 0 ? SIZE_MAX : owners[top->parent];
    declaration.node = index;
    if (top->parent != 0 && declaration.scope == SIZE_MAX)
        return reject(diagnostic, -6, top->begin,
                      "nested declaration scope is not a module catalog scope");
    at = top->begin;
    end = identifier_end(source, at, top->end);
    if (end - at == 7 && !memcmp(source + at, "include", 7))
    {
        declaration.kind = LIMBO_INCLUDE;
        declaration.name_begin = declaration.name_end = at;
        declaration.type_begin = top->begin;
        declaration.type_end = top->end;
        return append(interface, declaration, diagnostic);
    }
    named_index = child(tree, top, "NamedTop");
    if (named_index == SIZE_MAX || !limbo_tree_node(tree, named_index, &named))
        return reject(diagnostic, -6, at, "interface requires explicit named declarations");
    tail_index = child(tree, &named, "DeclarationTail");
    if (tail_index == SIZE_MAX || !limbo_tree_node(tree, tail_index, &tail))
        return reject(diagnostic, -6, at, "function bodies are not interface declarations");
    type_index = child(tree, &tail, "DeclType");
    if (type_index == SIZE_MAX || !limbo_tree_node(tree, type_index, &type))
        return reject(diagnostic, -6, at, "inferred declarations are not interface declarations");
    declaration.kind = category(source, type.begin, type.end);
    declaration.type_begin = type.begin;
    declaration.type_end = type.end;
    while (1)
    {
        declaration.name_begin = at;
        declaration.name_end = end;
        if (at == end)
            return reject(diagnostic, -6, at, "missing declaration identifier");
        if (!append(interface, declaration, diagnostic))
            return 0;
        at = trivia(source, end, tail.begin);
        if (at == tail.begin)
            break;
        if (source[at] != ',')
            return reject(diagnostic, -6, at, "unsupported declaration name prefix");
        at = trivia(source, at + 1, tail.begin);
        end = identifier_end(source, at, tail.begin);
    }
    if (declaration.kind == LIMBO_MODULE)
    {
        if (interface->count != first + 1)
            return reject(diagnostic, -6, top->begin,
                          "grouped module definitions need scope expansion");
        owners[type_index] = first;
    }
    return 1;
}

/* Release partial and complete catalogs through the same ownership boundary. */
void limbo_interface_destroy(LimboInterface *interface)
{
    size_t i;
    if (!interface)
        return;
    for (i = 0; i < interface->count; ++i)
        free((void *)interface->declarations[i].name);
    free(interface->keys);
    free(interface->declarations);
    limbo_tree_destroy(interface->tree);
    free(interface);
}

/* Parse once, derive module scopes in preorder, then check exact scope/name uniqueness. */
int limbo_interface_parse(const void *data, size_t size, LimboInterface **out,
                          LimboDiagnostic *diagnostic)
{
    LimboDiagnostic local;
    LimboInterface *interface;
    LimboNode node;
    size_t *owners, i, count;
    int ok = 1;
    if (!diagnostic)
        diagnostic = &local;
    memset(diagnostic, 0, sizeof(*diagnostic));
    if (!out)
        return reject(diagnostic, -1, 0, "missing interface output owner");
    interface = (LimboInterface *)calloc(1, sizeof(*interface));
    if (!interface)
        return reject(diagnostic, -2, 0, "interface owner allocation failure");
    if (!limbo_tree_parse(data, size, &interface->tree, diagnostic))
    {
        limbo_interface_destroy(interface);
        return 0;
    }
    count = limbo_tree_count(interface->tree);
    if (count > SIZE_MAX / sizeof(*owners))
    {
        limbo_interface_destroy(interface);
        return reject(diagnostic, -8, 0, "scope index storage limit exceeded");
    }
    owners = (size_t *)malloc(count * sizeof(*owners));
    if (!owners)
    {
        limbo_interface_destroy(interface);
        return reject(diagnostic, -2, 0, "scope index allocation failure");
    }
    for (i = 0; i < count; ++i)
        owners[i] = SIZE_MAX;
    for (i = 0; i < count && ok; ++i)
    {
        limbo_tree_node(interface->tree, i, &node);
        if (node.parent == 0 && !strcmp(node.kind, "Names"))
            ok = reject(diagnostic, -6, node.begin, "implement is not an interface declaration");
        else if (!strcmp(node.kind, "Top"))
            ok = collect(interface, i, &node, owners, diagnostic);
    }
    free(owners);
    if (ok)
        ok = unique_names(interface, diagnostic);
    if (!ok)
    {
        limbo_interface_destroy(interface);
        return 0;
    }
    *out = interface;
    return 1;
}

/* Borrow the immutable complete tree while retaining its catalog owner. */
const LimboTree *limbo_interface_tree(const LimboInterface *interface)
{
    return interface ? interface->tree : NULL;
}

/* Report source-order cardinality without parser or filesystem access. */
size_t limbo_interface_count(const LimboInterface *interface)
{
    return interface ? interface->count : 0;
}

/* Copy a stable view; names and source remain borrowed from the same catalog. */
int limbo_interface_declaration(const LimboInterface *interface, size_t index,
                                LimboDeclaration *out)
{
    if (!interface || !out || index >= interface->count)
        return 0;
    *out = interface->declarations[index];
    return 1;
}

/* Category spelling is metadata, not a runtime type hash or target C type name. */
const char *limbo_declaration_kind(LimboDeclKind kind)
{
    static const char *const names[] = {"module",    "adt",    "function", "constant", "alias",
                                        "exception", "import", "value",    "include"};
    return (unsigned)kind < sizeof(names) / sizeof(names[0]) ? names[kind] : NULL;
}

/* Resolve an export identity through sorted keys while preserving source-order public indices. */
int limbo_interface_find(const LimboInterface *interface, size_t scope, const char *name,
                         size_t *index)
{
    size_t low = 0, high;
    if (!interface || !name || !index || !*name ||
        (scope != SIZE_MAX &&
         (scope >= interface->count || interface->declarations[scope].kind != LIMBO_MODULE)))
        return 0;
    high = interface->key_count;
    while (low < high)
    {
        size_t middle = low + (high - low) / 2;
        const NameKey *key = &interface->keys[middle];
        int order = key->scope < scope ? -1 : key->scope > scope ? 1 : strcmp(key->name, name);
        if (order < 0)
            low = middle + 1;
        else if (order > 0)
            high = middle;
        else
        {
            *index = key->declaration;
            return 1;
        }
    }
    return 0;
}
