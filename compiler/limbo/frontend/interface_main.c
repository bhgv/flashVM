/* Native .m declaration inspector; layout, type resolution and C emission are separate passes. */
#include "interface.h"
#include <stdlib.h>

/* Print absent links distinctly from valid zero-based node indices. */
static void link_index(size_t index)
{
    if (index == SIZE_MAX)
        fputs("-", stdout);
    else
        printf("%lu", (unsigned long)index);
}

/* Read one bounded interface and emit source-ordered declaration identities and source spans. */
int main(int argc, char **argv)
{
    FILE *input;
    char *data;
    size_t size, i;
    LimboInterface *interface = NULL;
    const LimboTree *tree;
    LimboDiagnostic diagnostic;
    LimboDeclaration declaration;
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
    ok = limbo_interface_parse(data, size, &interface, &diagnostic);
    free(data);
    if (!ok)
    {
        fprintf(stderr, "%d:%d E%d byte %ld: %s\n", diagnostic.line, diagnostic.column,
                diagnostic.code, diagnostic.offset, diagnostic.message);
        return 1;
    }
    tree = limbo_interface_tree(interface);
    printf("source\t%lu\ndeclarations\t%lu\n", (unsigned long)limbo_tree_size(tree),
           (unsigned long)limbo_interface_count(interface));
    for (i = 0; i < limbo_interface_count(interface); ++i)
    {
        if (!limbo_interface_declaration(interface, i, &declaration))
        {
            limbo_interface_destroy(interface);
            return 1;
        }
        printf("decl\t%lu\t%s\t%s\t", (unsigned long)i, limbo_declaration_kind(declaration.kind),
               declaration.name);
        link_index(declaration.scope);
        printf("\t%lu\t%lu\t%lu\t%lu\t%lu\n", (unsigned long)declaration.node,
               (unsigned long)declaration.name_begin, (unsigned long)declaration.name_end,
               (unsigned long)declaration.type_begin, (unsigned long)declaration.type_end);
    }
    limbo_interface_destroy(interface);
    return fflush(stdout) == EOF || ferror(stdout) ? 1 : 0;
}
