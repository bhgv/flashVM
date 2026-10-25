/* Retained owners, independent query views and parser recovery inside one host process. */
#include <stdio.h>
#include "frontend.h"
#include <stdlib.h>
#include <string.h>


/* Exercise ownership across invalid syntax, tree exhaustion and later successful parsing. */
int main(void)
{
    char source[] = "implement Example; f(){ return 1+2; }";
    LimboTree *tree = NULL, *other = NULL, *before;
    LimboNode node, saved;
    LimboDiagnostic diagnostic;
    size_t count, i, index = SIZE_MAX;
    char *flat;
    char deep[4096];
    if (!(limbo_tree_parse(source, strlen(source), &tree, &diagnostic)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = tree;
    count = limbo_tree_count(tree);
    memset(source, 'X', sizeof(source) - 1);
    if (!(!strcmp(limbo_tree_source(tree), "implement Example; f(){ return 1+2; }")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_tree_size(tree) == sizeof(source) - 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_tree_node(tree, 0, &node)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(node.kind, "Limbo") && node.parent == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_tree_locate(tree, 0, &index) && index == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_tree_locate(tree, 30, &index)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_tree_node(tree, index, &node)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(node.begin <= 30 && 30 < node.end))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    index = SIZE_MAX;
    if (!(!limbo_tree_locate(tree, sizeof(source) - 1, &index) && index == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_tree_locate(tree, SIZE_MAX, &index) && index == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_tree_locate(NULL, 0, &index) && index == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_tree_locate(tree, 0, NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    saved = node;
    if (!(!limbo_tree_node(tree, count, &node) && !memcmp(&node, &saved, sizeof(node))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_tree_node(NULL, 0, &node) && !memcmp(&node, &saved, sizeof(node))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_tree_node(tree, 0, NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_tree_source(NULL) && !limbo_tree_count(NULL) && !limbo_tree_size(NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 32; ++i)
    {
        if (!(!limbo_tree_parse("@", 1, &tree, &diagnostic) && tree == before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(diagnostic.message[0]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!limbo_tree_parse("\xff", 1, &tree, &diagnostic) && tree == before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(limbo_check("implement Plain;", strlen("implement Plain;"), &diagnostic)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(limbo_tree_parse(NULL, 0, &other, NULL)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(limbo_tree_count(other) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        limbo_tree_destroy(other);
        if (!(limbo_tree_count(tree) == count))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!limbo_tree_parse("x", 2u * 1024u * 1024u + 1, &tree, &diagnostic) && tree == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_tree_parse(NULL, 1, &tree, &diagnostic) && tree == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_tree_parse(NULL, 0, NULL, NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    strcpy(deep, "x: ");
    for (i = 0; i < 600; ++i)
    {
        strcat(deep, "ref ");
    }
    strcat(deep, "T;");
    if (!(!limbo_tree_parse(deep, strlen(deep), &tree, &diagnostic)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(tree == before && diagnostic.code == -3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_tree_parse("implement Shallow;", strlen("implement Shallow;"), &other, NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    limbo_tree_destroy(other);
    flat = (char *)malloc(60006);
    if (!(flat))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memcpy(flat, "f(){", 4);
    for (i = 4; i < 60004; i += 2)
    {
        memcpy(flat + i, "a;", 2);
    }
    flat[60004] = '}';
    if (!(!limbo_tree_parse(flat, 60005, &tree, &diagnostic)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(tree == before && diagnostic.code == -5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Syntax-only callers retain their original budget; recording is strictly optional. */
    if (!(limbo_check(flat, 60005, &diagnostic)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    free(flat);
    if (!(limbo_tree_parse("implement After;", strlen("implement After;"), &other, NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    limbo_tree_destroy(other);
    if (!(limbo_tree_node(tree, 0, &node) && !strcmp(node.kind, "Limbo")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    limbo_tree_destroy(tree);
    limbo_tree_destroy(NULL);
    return 0;
}
