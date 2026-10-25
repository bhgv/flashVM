#include <stdio.h>
#include <stdlib.h>
#include "../src/details/hybrid_table.hpp"


int main()
{
    fvm_detail::hybrid_table<int> t;
    int v = 0;
    t.set(0UL, 11);
    t.set(2UL, 33);
    t.set(100000000UL, 99);
    t.set(fvm_detail::string("key"), 44);
    if (!(t.size() == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.array_capacity() < 1000000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.get(2UL, &v) && v == 33))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.get(100000000UL, &v) && v == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!t.get(1UL, &v)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.get(fvm_detail::string("key"), &v) && v == 44))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.set(2UL, 55);
    if (!(t.size() == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.queue_push(7);
    t.queue_push(8);
    if (!(t.queue_pop(&v) && v == 7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.queue_pop(&v) && v == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!t.queue_pop(&v)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
