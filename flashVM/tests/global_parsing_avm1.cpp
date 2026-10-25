#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include "flashvm/gc.h"
#include "../src/details/vector.hpp"
#include <cstring>

typedef fvm_detail::vector<unsigned char> Bytes;


static void word(Bytes &b, size_t n)
{
    b.push_back((unsigned char)n);
    b.push_back((unsigned char)(n >> 8));
}


static void text(Bytes &b, const char *s)
{
    do
    {
        b.push_back((unsigned char)*s);
    } while (*s++);
}


static void string(Bytes &b, const char *s)
{
    b.push_back(0x96);
    word(b, std::strlen(s) + 2);
    b.push_back(0);
    text(b, s);
}


static void integer(Bytes &b, unsigned long n)
{
    b.push_back(0x96);
    word(b, 5);
    b.push_back(7);
    for (int i = 0; i < 4; ++i)
    {
        b.push_back((unsigned char)(n >> (i * 8)));
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Value result;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes actions;
    string(actions, "parsedInt");
    integer(actions, 16);
    string(actions, "3E8suffix");
    integer(actions, 2);
    string(actions, "parseInt");
    actions.push_back(0x3d);
    actions.push_back(0x1d);
    string(actions, "parsedFloat");
    string(actions, "12.5e1pixels");
    integer(actions, 1);
    string(actions, "parseFloat");
    actions.push_back(0x3d);
    actions.push_back(0x1d);
    string(actions, "converted");
    integer(actions, 42);
    integer(actions, 1);
    string(actions, "String");
    actions.push_back(0x3d);
    actions.push_back(0x1d);
    actions.push_back(0);
    if (!(fvm_avm1_execute(c, &actions[0], actions.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "parsedInt", &result) == FVM_OK && result.number == 1000))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "parsedFloat", &result) == FVM_OK && result.number == 125))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "converted", &result) == FVM_OK && !std::strcmp(result.string, "42")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
