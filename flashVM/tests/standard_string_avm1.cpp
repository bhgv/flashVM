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
    FVM_Value out;
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
    Bytes b;
    string(b, "boxed");
    string(b, "A\320\221\360\237\230\200Z");
    integer(b, 1);
    string(b, "String");
    b.push_back(0x40);
    b.push_back(0x1d);
    string(b, "measured");
    string(b, "boxed");
    b.push_back(0x1c);
    string(b, "length");
    b.push_back(0x4e);
    b.push_back(0x1d);
    string(b, "selected");
    integer(b, 4);
    integer(b, 2);
    integer(b, 2);
    string(b, "boxed");
    b.push_back(0x1c);
    string(b, "substring");
    b.push_back(0x52);
    b.push_back(0x1d);
    string(b, "isString");
    string(b, "boxed");
    b.push_back(0x1c);
    string(b, "String");
    b.push_back(0x1c);
    b.push_back(0x54);
    b.push_back(0x1d);
    b.push_back(0);
    if (!(fvm_avm1_execute(c, &b[0], b.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "measured", &out) == FVM_OK && out.number == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "selected", &out) == FVM_OK && !std::strcmp(out.string, "\360\237\230\200")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "isString", &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
