/* Owned grammar tree builder. Parser actions are optional and never retain caller storage. */
#include "frontend.h"
#include <stdlib.h>
#include <string.h>


/* Keep append bookkeeping private while public node views remain stable scalar copies. */
typedef struct TreeNode
{
    LimboNode view;
    size_t last_child;
    size_t depth;
} TreeNode;


/* One ownership boundary for the source snapshot and a bounded, relocatable node array. */
struct LimboTree
{
    char *source;
    size_t size;
    TreeNode *nodes;
    size_t count;
    size_t capacity;
    size_t current;
    int failure;
};

/* Only parser actions consult this pointer; immutable tree queries never do. */
static LimboTree *building;


/* Append one production node and link it without exposing reallocatable pointers. */
void limbo_tree_enter(const char *kind, long start)
{
    LimboTree *tree = building;
    TreeNode *node;
    size_t index;
    if (!tree || tree->failure)
    {
        return;
    }
    if (start < 0 || (size_t)start > tree->size || tree->count == 262144)
    {
        tree->failure = -5;
        return;
    }
    if (tree->count == tree->capacity)
    {
        size_t capacity = tree->capacity ? tree->capacity * 2 : 256;
        TreeNode *grown;
        if (capacity > SIZE_MAX / sizeof(*grown))
        {
            tree->failure = -5;
            return;
        }
        grown = (TreeNode *)realloc(tree->nodes, capacity * sizeof(*grown));
        if (!grown)
        {
            tree->failure = -2;
            return;
        }
        tree->nodes = grown;
        tree->capacity = capacity;
    }
    index = tree->count++;
    node = &tree->nodes[index];
    node->view.kind = kind;
    node->view.begin = node->view.end = (size_t)start;
    node->view.parent = tree->current;
    node->depth = tree->current == SIZE_MAX ? 0 : tree->nodes[tree->current].depth + 1;
    node->view.first_child = node->view.next_sibling = node->last_child = SIZE_MAX;
    if (tree->current != SIZE_MAX)
    {
        TreeNode *parent = &tree->nodes[tree->current];
        if (parent->last_child == SIZE_MAX)
        {
            parent->view.first_child = index;
        }
        else
        {
            tree->nodes[parent->last_child].view.next_sibling = index;
        }
        parent->last_child = index;
    }
    tree->current = index;
}


/* Finish a half-open span, retaining empty lookahead anchors inside ancestor spans. */
void limbo_tree_leave(long end)
{
    LimboTree *tree = building;
    LimboNode *node;
    size_t parent;
    if (!tree || tree->failure)
    {
        return;
    }
    if (tree->current == SIZE_MAX || end < 0 || (size_t)end > tree->size)
    {
        tree->failure = -5;
        return;
    }
    node = &tree->nodes[tree->current].view;
    if ((size_t)end > node->end)
    {
        node->end = (size_t)end;
    }
    parent = node->parent;
    if (parent != SIZE_MAX && tree->nodes[parent].view.end < node->end)
    {
        tree->nodes[parent].view.end = node->end;
    }
    tree->current = parent;
}


/* Destroy partially constructed trees through the same path as successful owners. */
void limbo_tree_destroy(LimboTree *tree)
{
    if (tree)
    {
        free(tree->nodes);
        free(tree->source);
        free(tree);
    }
}


/* Return an owned parse product only after syntax and builder state both succeed. */
int limbo_tree_parse(const void *data, size_t size, LimboTree **out, LimboDiagnostic *diagnostic)
{
    LimboDiagnostic local;
    LimboTree *tree;
    int ok;
    if (!diagnostic)
    {
        diagnostic = &local;
    }
    memset(diagnostic, 0, sizeof(*diagnostic));
    if (!out || (!data && size) || size > 2u * 1024u * 1024u)
    {
        diagnostic->code = -1;
        snprintf(diagnostic->message, sizeof(diagnostic->message), "invalid tree arguments/input size");
        return 0;
    }
    tree = (LimboTree *)calloc(1, sizeof(*tree));
    if (!tree)
    {
        diagnostic->code = -2;
        snprintf(diagnostic->message, sizeof(diagnostic->message), "tree allocation failure");
        return 0;
    }
    tree->current = SIZE_MAX;
    tree->source = (char *)malloc(size + 1);
    if (!tree->source)
    {
        limbo_tree_destroy(tree);
        diagnostic->code = -2;
        snprintf(diagnostic->message, sizeof(diagnostic->message), "source allocation failure");
        return 0;
    }
    if (size)
    {
        memcpy(tree->source, data, size);
    }
    tree->source[size] = 0;
    tree->size = size;
    building = tree;
    ok = limbo_check(tree->source, size, diagnostic);
    building = NULL;
    if (ok && (tree->failure || tree->current != SIZE_MAX || !tree->count))
    {
        diagnostic->code = tree->failure ? tree->failure : -5;
        snprintf(diagnostic->message,
                 sizeof(diagnostic->message),
                 "%s",
                 diagnostic->code == -2 ? "tree allocation failure" : "tree storage/node limit exceeded");
        ok = 0;
    }
    if (!ok)
    {
        limbo_tree_destroy(tree);
        return 0;
    }
    *out = tree;
    return 1;
}


/* Query immutable ownership without touching parser globals. */
size_t limbo_tree_count(const LimboTree *tree)
{
    return tree ? tree->count : 0;
}


/* Borrow the original source snapshot, including comments and authored spelling. */
const char *limbo_tree_source(const LimboTree *tree)
{
    return tree ? tree->source : NULL;
}


/* Report source bytes independently of terminating NUL and node spans. */
size_t limbo_tree_size(const LimboTree *tree)
{
    return tree ? tree->size : 0;
}


/* Keep caller views intact when an index or ownership argument is invalid. */
int limbo_tree_node(const LimboTree *tree, size_t index, LimboNode *out)
{
    if (!tree || !out || index >= tree->count)
    {
        return 0;
    }
    *out = tree->nodes[index].view;
    return 1;
}


/* Select an innermost grammar context without allocating, recursing or inspecting tokens. */
int limbo_tree_locate(const LimboTree *tree, size_t byte_offset, size_t *index)
{
    size_t i, best = SIZE_MAX, depth = 0;
    if (!tree || !index || byte_offset >= tree->size)
    {
        return 0;
    }
    for (i = 0; i < tree->count; ++i)
    {
        const TreeNode *node = &tree->nodes[i];
        if (node->view.begin <= byte_offset && byte_offset < node->view.end &&
            (best == SIZE_MAX || node->depth > depth))
        {
            best = i;
            depth = node->depth;
        }
    }
    if (best == SIZE_MAX)
    {
        return 0;
    }
    *index = best;
    return 1;
}
