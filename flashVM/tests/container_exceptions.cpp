#include <stdio.h>
#include <stdlib.h>
#include "../src/details/vector.hpp"
#include "../src/details/hash_table.hpp"


struct Value
{
    static int live, until_throw;
    int n;


    Value() : n(0)
    {
        ++live;
    }


    Value(const Value &v) : n(v.n)
    {
        ++live;
    }


    ~Value()
    {
        --live;
    }

    Value &operator=(const Value &v)
    {
        if (until_throw == 0)
        {
            throw 1;
        }
        if (until_throw > 0)
        {
            --until_throw;
        }
        n = v.n;
        return *this;
    }
};

int Value::live = 0;
int Value::until_throw = -1;


int main()
{
    Value::until_throw = 0;
    try
    {
        fvm_detail::vector<Value> failed(10);
        if (!(false))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    catch (int)
    {
    }
    if (!(Value::live == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Value::until_throw = -1;
    {
        fvm_detail::vector<Value> v;
        Value x;
        x.n = 42;
        v.push_back(x);
        int count = Value::live;
        Value::until_throw = 0;
        try
        {
            v.reserve(20);
            if (!(false))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        catch (int)
        {
        }
        if (!(Value::live == count && v.size() == 1 && v[0].n == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        try
        {
            v.push_back(x);
            if (!(false))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        catch (int)
        {
        }
        if (!(Value::live == count && v.size() == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Value::until_throw = -1;
    }
    if (!(Value::live == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        fvm_detail::map<unsigned long, Value> m;
        for (unsigned long i = 0; i < 5; ++i)
        {
            m[i].n = (int)i;
        }
        int count = Value::live;
        Value::until_throw = 2;
        try
        {
            fvm_detail::map<unsigned long, Value> copy(m);
            if (!(false))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        catch (int)
        {
        }
        if (!(Value::live == count && m.size() == 5))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Value::until_throw = -1;
    }
    if (!(Value::live == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
