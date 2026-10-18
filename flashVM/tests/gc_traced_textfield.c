#include "flashvm/gc.h"
#include "flashvm/standard.h"
#include "flashvm/standard_text.h"
#include "flashvm/gc_roots.h"
#include "flashvm/transfer.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <string.h>
int main(void)
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c,*other;FVM_Object *field;
    FVM_Value value,args[3],out;FVM_GCRootScope *scope;FVM_Channel *channel;FVM_Path *path;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);
    assert(fvm_standard_install(c)==FVM_OK);assert(fvm_standard_text_install(c)==FVM_OK);
    assert(fvm_standard_text_install(other)==FVM_OK);
    assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
    assert(fvm_gc_string_mode(other,FVM_GC_STRINGS_TRACED)==FVM_OK);
    assert(fvm_standard_text_register_font(c,"Geometry",fvm_test_font,sizeof(fvm_test_font),0)==FVM_OK);
    assert(fvm_textfield_create(c,"Geometry",100,100,&field)==FVM_OK);assert(fvm_gc_pin(c,field)==FVM_OK);
    assert(fvm_value_string(&value,"AB")==FVM_OK);assert(fvm_object_set(c,field,"text",&value)==FVM_OK);
    assert(fvm_gc_collect(c,0)==FVM_OK);
    assert(fvm_object_get(c,field,"text",&value)==FVM_OK && !strcmp(value.string,"AB"));
    assert(fvm_textfield_path(c,field,&path)==FVM_OK);fvm_path_destroy(path);
    assert(fvm_gc_roots_begin(c,&scope)==FVM_OK);assert(fvm_gc_roots_add(scope,&value)==FVM_OK);
    fvm_value_number(&args[0],0);fvm_value_number(&args[1],1);assert(fvm_value_string(&args[2],"V")==FVM_OK);
    assert(fvm_object_call(c,field,"replaceText",3,args,&out)==FVM_OK);
    assert(fvm_gc_collect(c,0)==FVM_OK && !strcmp(value.string,"AB"));
    assert(fvm_gc_roots_end(scope)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);
    assert(fvm_object_get(c,field,"text",&out)==FVM_OK && !strcmp(out.string,"VB"));
    assert(fvm_channel_create(&channel)==FVM_OK);
    fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=field;
    assert(fvm_channel_send_value(c,channel,&value,0)==FVM_OK);
    assert(fvm_gc_unpin(c,field)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);
    assert(fvm_channel_receive_value(other,channel,&value,0)==FVM_OK);
    assert(fvm_gc_pin(other,value.object)==FVM_OK);assert(fvm_gc_collect(other,0)==FVM_OK);
    assert(fvm_object_get(other,value.object,"text",&out)==FVM_OK && !strcmp(out.string,"VB"));
    assert(fvm_textfield_path(other,value.object,&path)==FVM_OK);fvm_path_destroy(path);
    assert(fvm_gc_unpin(other,value.object)==FVM_OK);assert(fvm_gc_collect(other,0)==FVM_OK);
    fvm_channel_destroy(channel);fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
    return 0;
}
