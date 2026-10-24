/* Retained owners, independent query views and parser recovery inside one host process. */
#include "frontend.h"
#include <assert.h>
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
    assert(limbo_tree_parse(source, strlen(source), &tree, &diagnostic));
    before = tree;
    count = limbo_tree_count(tree);
    memset(source, 'X', sizeof(source) - 1);
    assert(!strcmp(limbo_tree_source(tree), "implement Example; f(){ return 1+2; }"));
    assert(limbo_tree_size(tree) == sizeof(source) - 1);
    assert(limbo_tree_node(tree, 0, &node));
    assert(!strcmp(node.kind, "Limbo") && node.parent == SIZE_MAX);
    assert(limbo_tree_locate(tree, 0, &index) && index == 0);
    assert(limbo_tree_locate(tree, 30, &index));
    assert(limbo_tree_node(tree, index, &node));
    assert(node.begin <= 30 && 30 < node.end);
    index = SIZE_MAX;
    assert(!limbo_tree_locate(tree, sizeof(source) - 1, &index) && index == SIZE_MAX);
    assert(!limbo_tree_locate(tree, SIZE_MAX, &index) && index == SIZE_MAX);
    assert(!limbo_tree_locate(NULL, 0, &index) && index == SIZE_MAX);
    assert(!limbo_tree_locate(tree, 0, NULL));
    saved = node;
    assert(!limbo_tree_node(tree, count, &node) && !memcmp(&node, &saved, sizeof(node)));
    assert(!limbo_tree_node(NULL, 0, &node) && !memcmp(&node, &saved, sizeof(node)));
    assert(!limbo_tree_node(tree, 0, NULL));
    assert(!limbo_tree_source(NULL) && !limbo_tree_count(NULL) && !limbo_tree_size(NULL));
    for (i = 0; i < 32; ++i)
    {
        assert(!limbo_tree_parse("@", 1, &tree, &diagnostic) && tree == before);
        assert(diagnostic.message[0]);
        assert(!limbo_tree_parse("\xff", 1, &tree, &diagnostic) && tree == before);
        assert(limbo_check("implement Plain;", strlen("implement Plain;"), &diagnostic));
        assert(limbo_tree_parse(NULL, 0, &other, NULL));
        assert(limbo_tree_count(other) == 1);
        limbo_tree_destroy(other);
        assert(limbo_tree_count(tree) == count);
    }
    assert(!limbo_tree_parse("x", 2u * 1024u * 1024u + 1, &tree, &diagnostic) && tree == before);
    assert(!limbo_tree_parse(NULL, 1, &tree, &diagnostic) && tree == before);
    assert(!limbo_tree_parse(NULL, 0, NULL, NULL));
    strcpy(deep, "x: ");
    for (i = 0; i < 600; ++i)
        strcat(deep, "ref ");
    strcat(deep, "T;");
    assert(!limbo_tree_parse(deep, strlen(deep), &tree, &diagnostic));
    assert(tree == before && diagnostic.code == -3);
    assert(limbo_tree_parse("implement Shallow;", strlen("implement Shallow;"), &other, NULL));
    limbo_tree_destroy(other);
    flat = (char *)malloc(60006);
    assert(flat);
    memcpy(flat, "f(){", 4);
    for (i = 4; i < 60004; i += 2)
        memcpy(flat + i, "a;", 2);
    flat[60004] = '}';
    assert(!limbo_tree_parse(flat, 60005, &tree, &diagnostic));
    assert(tree == before && diagnostic.code == -5);
    /* Syntax-only callers retain their original budget; recording is strictly optional. */
    assert(limbo_check(flat, 60005, &diagnostic));
    free(flat);
    assert(limbo_tree_parse("implement After;", strlen("implement After;"), &other, NULL));
    limbo_tree_destroy(other);
    assert(limbo_tree_node(tree, 0, &node) && !strcmp(node.kind, "Limbo"));
    limbo_tree_destroy(tree);
    limbo_tree_destroy(NULL);
    return 0;
}
