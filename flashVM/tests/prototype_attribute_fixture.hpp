/* Per-method activation must not depend on another method's default shape. */
#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_TEST_PROTOTYPE_ATTRIBUTE_FIXTURE_HPP
#define FVM_TEST_PROTOTYPE_ATTRIBUTE_FIXTURE_HPP
#include "static_activation_fixture.hpp"

static const char *prototype_anchors[] = {"valueOf", "getTime", "charAt", "push"};
static const char *prototype_peers[] = {"toString", "getFullYear", "charCodeAt", "pop"};


static FVM_Object *prototype_target(FVM_Context *c, unsigned type)
{
    FVM_Value proto;
    if (!(fvm_object_get(c, static_class(c, type == 3 ? 4 : type), "prototype", &proto) == FVM_OK &&
          proto.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return proto.object;
}


static void prototype_edit(FVM_Context *c, FVM_Object *proto, const char *name, unsigned kind)
{
    FVM_Value value;
    int removed = 0;
    if (!kind)
    {
        return;
    }
    if (kind == 1)
    {
        fvm_value_number(&value, 81);
        if (!(fvm_object_set(c, proto, name, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (kind == 2 || kind == 3)
    {
        if (!(fvm_object_delete(c, proto, name, &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (kind == 3)
    {
        if (!(fvm_object_define_accessor_rooted(c, proto, name, static_getter, 0, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (kind == 4 || kind == 5)
    {
        if (!(fvm_object_set_property_flags(c, proto, name, 0, 7) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (kind == 5)
        {
            if (!(fvm_object_set_property_flags(c, proto, name, 7, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
}


static unsigned prototype_expected(unsigned kind)
{
    return !kind ? 3U : kind == 1 ? 1U : kind == 5 ? 7U : 0U;
}
#endif
