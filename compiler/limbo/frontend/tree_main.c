/* Tabular concrete-tree inspector; no semantic acceptance or target code is emitted. */
#include "frontend.h"
#include <stdlib.h>

/* Print absent links distinctly from valid zero-based node indices. */
static void link_index(size_t index)
{
    if (index == SIZE_MAX)
        fputs("-", stdout);
    else
        printf("%lu", (unsigned long)index);
}

/* Read one bounded source snapshot and print grammar node relationships with byte spans. */
int main(int argc, char **argv)
{
    FILE *input;
    char *data;
    size_t size, i;
    LimboTree *tree = NULL;
    LimboDiagnostic diagnostic;
    LimboNode node;
    int ok;
    if (argc != 2)
        return 2;
    input = fopen(argv[1], "rb");
    if (!input)
        return 2;
    data = (char *)malloc(2u * 1024u * 1024u + 1);
    if (!data)
    {
        fclose(input);
        return 2;
    }
    size = fread(data, 1, 2u * 1024u * 1024u + 1, input);
    if (ferror(input))
    {
        free(data);
        fclose(input);
        return 2;
    }
    if (fclose(input))
    {
        free(data);
        return 2;
    }
    ok = limbo_tree_parse(data, size, &tree, &diagnostic);
    free(data);
    if (!ok)
    {
        fprintf(stderr, "%d:%d E%d byte %ld: %s\n", diagnostic.line, diagnostic.column,
                diagnostic.code, diagnostic.offset, diagnostic.message);
        return 1;
    }
    printf("source\t%lu\nnodes\t%lu\n", (unsigned long)limbo_tree_size(tree),
           (unsigned long)limbo_tree_count(tree));
    for (i = 0; i < limbo_tree_count(tree); ++i)
    {
        if (!limbo_tree_node(tree, i, &node))
        {
            limbo_tree_destroy(tree);
            return 1;
        }
        printf("node\t%lu\t%s\t%lu\t%lu\t", (unsigned long)i, node.kind, (unsigned long)node.begin,
               (unsigned long)node.end);
        link_index(node.parent);
        putchar('\t');
        link_index(node.first_child);
        putchar('\t');
        link_index(node.next_sibling);
        putchar('\n');
    }
    limbo_tree_destroy(tree);
    return fflush(stdout) == EOF || ferror(stdout) ? 1 : 0;
}
