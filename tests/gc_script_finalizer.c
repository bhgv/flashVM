#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include "flashvm/table.h"
#include <assert.h>
int main(void)
{
    /* cleanup() { survivor=this; return 1; } */
    static const unsigned char code[] = {
        0x9b, 39,  0,   'c', 'l',  'e',  'a',  'n', 'u', 'p', 0,   0,    0, 27, 0,
        0x96, 16,  0,   0,   's',  'u',  'r',  'v', 'i', 'v', 'o', 'r',  0, 0,  't',
        'h',  'i', 's', 0,   0x1c, 0x1d, 0x96, 2,   0,   5,   1,   0x3e, 0};
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Object *o, *m;
    FVM_Value v;
    unsigned long n;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_avm1_execute(c, code, sizeof(code)) == FVM_OK);
    assert(fvm_object_create(c, &o) == FVM_OK);
    assert(fvm_object_create(c, &m) == FVM_OK);
    assert(fvm_value_string(&v, "cleanup") == FVM_OK);
    assert(fvm_object_set(c, m, "__gc", &v) == FVM_OK);
    assert(fvm_table_set_metatable(c, o, m) == FVM_OK);
    assert(fvm_gc_collect(c, &n) == FVM_OK && n == 0);
    assert(fvm_get_global(c, "survivor", &v) == FVM_OK && v.object == o);
    fvm_value_undefined(&v);
    assert(fvm_set_global(c, "survivor", &v) == FVM_OK);
    /* The two objects and a heap activation from the finalizer become unreachable. */
    assert(fvm_gc_collect(c, &n) == FVM_OK && n >= 2);
    assert(fvm_gc_collect(c, &n) == FVM_OK && n == 0);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
