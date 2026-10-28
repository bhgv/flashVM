/* Resolve outer aliases without scanning Limbo source syntax a second time. */
#include "resolve.h"
#include <stdlib.h>
#include <string.h>


/* Copy a diagnostic at a tree byte offset; owner is borrowed, output is optional.
 * Return zero for direct use on a failure path; no parser state or allocation. */
static int
failure(const LimboInterface *owner, LimboDiagnostic *diagnostic, int code, size_t offset, const char *message)
{
    if (diagnostic)
    {
        const LimboTree *tree = limbo_interface_tree(owner);
        const char *source = limbo_tree_source(tree);
        size_t index;
        memset(diagnostic, 0, sizeof(*diagnostic));
        diagnostic->code = code;
        diagnostic->offset = (long)offset;
        diagnostic->line = 1;
        diagnostic->column = 1;
        for (index = 0; source && index < offset && index < limbo_tree_size(tree); index++)
        {
            if (source[index] == '\n')
            {
                diagnostic->line++;
                diagnostic->column = 1;
            }
            else
            {
                diagnostic->column++;
            }
        }
        snprintf(diagnostic->message, sizeof(diagnostic->message), "%s", message);
    }
    return 0;
}


/* Find a Type owned directly by the declaration's DeclType, not a nested argument.
 * Catalog/tree/output stay caller-owned; invalid or unsupported declarations preserve out. */
int limbo_interface_declared_type(const LimboInterface *owner, size_t declaration, size_t *out)
{
    const LimboTree *tree = limbo_interface_tree(owner);
    LimboDeclaration item;
    LimboNode node;
    size_t index;
    if (!out || !limbo_interface_declaration(owner, declaration, &item) ||
        (item.kind != LIMBO_ALIAS && item.kind != LIMBO_FUNCTION && item.kind != LIMBO_VALUE))
    {
        return 0;
    }
    for (index = item.node; limbo_tree_node(tree, index, &node); index++)
    {
        LimboNode parent;
        if (node.begin >= item.type_end)
        {
            break;
        }
        if (!strcmp(node.kind, "Type") && limbo_tree_node(tree, node.parent, &parent) &&
            !strcmp(parent.kind, "DeclType") && parent.begin == item.type_begin)
        {
            *out = index;
            return 1;
        }
    }
    return 0;
}


/* Obtain exactly one unqualified identifier from grammar-produced child nodes.
 * Return its exact token span; reject qualification and nonempty type arguments. */
static int type_identifier(const LimboTree *tree, size_t type, LimboNode *out)
{
    LimboNode node;
    LimboNode named;
    size_t index;
    unsigned int names = 0;
    if (!limbo_tree_node(tree, type, &node))
    {
        return 0;
    }
    for (index = node.first_child; limbo_tree_node(tree, index, &named); index = named.next_sibling)
    {
        if (!strcmp(named.kind, "NamedType"))
        {
            size_t child;
            for (child = named.first_child; limbo_tree_node(tree, child, &node); child = node.next_sibling)
            {
                if (!strcmp(node.kind, "TypeIdentifier"))
                {
                    *out = node;
                    names++;
                }
                if (!strcmp(node.kind, "TypeArguments") && node.end > node.begin)
                {
                    return 0;
                }
            }
            return names == 1;
        }
    }
    return 0;
}


/* Follow checked catalog bindings iteratively, retaining neither names nor VM state.
 * See resolve.h for lexical scope, diagnostics, composite and concurrency limits. */
int limbo_interface_resolve_type(
    const LimboInterface *owner, size_t scope, size_t type, size_t *out, LimboDiagnostic *diagnostic)
{
    const LimboTree *tree = limbo_interface_tree(owner);
    const char *source = limbo_tree_source(tree);
    size_t visited[LIMBO_ALIAS_MAX_DEPTH];
    size_t count = 0;
    LimboDeclaration item;
    if (diagnostic)
    {
        memset(diagnostic, 0, sizeof(*diagnostic));
    }
    if (!owner || !out ||
        (scope != SIZE_MAX && (!limbo_interface_declaration(owner, scope, &item) || item.kind != LIMBO_MODULE)))
    {
        return failure(owner, diagnostic, -1, 0, "invalid type resolution query");
    }
    for (;;)
    {
        const char *kind = limbo_interface_type_kind(owner, type);
        LimboNode node;
        size_t declaration;
        size_t lookup = scope;
        size_t previous;
        size_t length;
        char *name;
        int found;
        if (!kind || !limbo_tree_node(tree, type, &node))
        {
            return failure(owner, diagnostic, -1, 0, "type resolution requires a Type node");
        }
        if (strcmp(kind, "named"))
        {
            *out = type;
            return 1;
        }
        if (!type_identifier(tree, type, &node))
        {
            return failure(owner, diagnostic, -10, node.begin, "qualified or generic type binding is not supported");
        }
        length = node.end - node.begin;
        name = malloc(length + 1);
        if (!name)
        {
            return failure(owner, diagnostic, -2, node.begin, "type-name allocation failure");
        }
        memcpy(name, source + node.begin, length);
        name[length] = 0;
        for (;;)
        {
            found = limbo_interface_find(owner, lookup, name, &declaration);
            if (found || lookup == SIZE_MAX)
            {
                break;
            }
            if (!limbo_interface_declaration(owner, lookup, &item))
            {
                free(name);
                return failure(owner, diagnostic, -1, node.begin, "invalid enclosing module scope");
            }
            lookup = item.scope;
        }
        free(name);
        if (!found || !limbo_interface_declaration(owner, declaration, &item) || item.kind != LIMBO_ALIAS)
        {
            return failure(owner, diagnostic, -10, node.begin, "named type has no alias in its lexical scope");
        }
        for (previous = 0; previous < count; previous++)
        {
            if (visited[previous] == declaration)
            {
                return failure(owner, diagnostic, -11, node.begin, "cyclic type alias");
            }
        }
        if (count == LIMBO_ALIAS_MAX_DEPTH)
        {
            return failure(owner, diagnostic, -11, node.begin, "type alias depth limit");
        }
        visited[count++] = declaration;
        if (!limbo_interface_declared_type(owner, declaration, &type))
        {
            return failure(owner, diagnostic, -10, node.begin, "alias has no direct type production");
        }
        scope = item.scope;
    }
}
