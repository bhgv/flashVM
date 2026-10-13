#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <string.h>
int main(void)
{
    const unsigned char code[]={0x96,19,0,0,'s',0,0,'p','r','e','f','i','x',0,0,'s','u','f','f','i','x',0,0x47,0x1d,0};
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Value value;FVM_GCStringStats stats;unsigned i;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);
    assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);assert(fvm_gc_mode(c,100)==FVM_OK);
    for(i=0;i<1000;++i)
    {
        assert(fvm_avm1_execute(c,code,sizeof(code))==FVM_OK);
        assert(fvm_get_global(c,"s",&value)==FVM_OK && !strcmp(value.string,"prefixsuffix"));
    }
    assert(fvm_gc_collect(c,0)==FVM_OK);
    assert(fvm_gc_string_stats(c,&stats)==FVM_OK && stats.buffers==1 && stats.payload_bytes==12);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
