/* Public URL sink declaration and flag constants compile as C89 without private runtime types. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/avm1.h"


static FVM_Result callback(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *v, void *u)
{
    (void)c;
    (void)o;
    (void)n;
    (void)a;
    (void)u;
    fvm_value_undefined(v);
    return FVM_OK;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
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
    if (!(FVM_AVM1_URL_METHOD_NONE == 0U && FVM_AVM1_URL_METHOD_GET == 1U && FVM_AVM1_URL_METHOD_POST == 2U &&
          FVM_AVM1_URL_LOAD_TARGET == 64U && FVM_AVM1_URL_LOAD_VARIABLES == 128U))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_avm1_set_url_rooted(c, callback, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_avm1_set_url_rooted(c, 0, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
