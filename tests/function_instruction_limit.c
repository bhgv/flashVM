#include "flashvm/avm1.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include <assert.h>
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *fn = 0;
    FVM_Value v;
    const unsigned char body[] = {
        0x96, 2,    0,    3,    3, 0x17, 0x17, 0x96, 2,    0,    3,
        3,    0x17, 0x17, 0x96, 2, 0,    3,    3,    0x17, 0x17, 0}; /* Nine nonterminal actions. */
    const unsigned char caller[] = {0x96, 5, 0, 7,   0,   0, 0,    0,    0x96,
                                    4,    0, 0, 'f', 'n', 0, 0x3d, 0x17, 0};
    assert(fvm_config_init(&cfg) == FVM_OK);
    cfg.instruction_limit = 11;
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_function_create_avm1(c, body, sizeof(body), 0, 0, 0, &fn) == FVM_OK);
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = fn;
    assert(fvm_set_global(c, "fn", &v) == FVM_OK);
    assert(fvm_function_invoke(c, fn, 0, 0, 0, &v) == FVM_OK);
    /* Both frames fit individually; their combined instruction count does not. */
    assert(fvm_avm1_execute(c, caller, sizeof(caller)) == FVM_ERROR);
    assert(fvm_function_invoke(c, fn, 0, 0, 0, &v) == FVM_OK);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
