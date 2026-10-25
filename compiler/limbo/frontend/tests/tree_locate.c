/* Byte-to-grammar context must match a parent-link oracle for every retained source byte. */
#include <stdio.h>
#include <stdlib.h>
#include "frontend.h"
#include <string.h>


/* Derive depth by walking public parent links, independently of the store's cached depth. */
static size_t depth(const LimboTree *tree, size_t index)
{
    LimboNode node;
    size_t value = 0;
    while (1)
    {
        if (!(limbo_tree_node(tree, index, &node)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (node.parent == SIZE_MAX)
        {
            return value;
        }
        if (!(node.parent < index))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        index = node.parent;
        ++value;
    }
}


/* Compare one tree at all byte offsets, including UTF-8 continuation bytes and trivia. */
static void check(const char *source)
{
    LimboTree *tree = NULL;
    size_t offset, i, chosen, expected, best_depth, node_depth;
    LimboNode node;
    if (!(limbo_tree_parse(source, strlen(source), &tree, NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (offset = 0; offset < strlen(source); ++offset)
    {
        expected = SIZE_MAX;
        best_depth = 0;
        for (i = 0; i < limbo_tree_count(tree); ++i)
        {
            if (!(limbo_tree_node(tree, i, &node)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            node_depth = depth(tree, i);
            if (node.begin <= offset && offset < node.end && (expected == SIZE_MAX || node_depth > best_depth))
            {
                expected = i;
                best_depth = node_depth;
            }
        }
        chosen = SIZE_MAX;
        if (!(limbo_tree_locate(tree, offset, &chosen)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(chosen == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    chosen = SIZE_MAX;
    if (!(!limbo_tree_locate(tree, strlen(source), &chosen) && chosen == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    limbo_tree_destroy(tree);
}


/* Use LF/CRLF, comments, empty productions, non-ASCII strings and nested precedence contexts. */
int main(void)
{
    check("");
    check("# comment only\n");
    check("\xef\xbb\xbf# BOM\r\n");
    check("# leading\nimplement A; f(){ x:=1+2*3**4; } # trailing\n");
    check("implement A;\r\nf[T](x:T):T { return x; }\r\n");
    check("f(){ x:=\"\xe4\xb8\xad\"; if(x!=nil) { return; } }");
    return 0;
}
