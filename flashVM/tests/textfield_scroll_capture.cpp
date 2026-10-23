/* Capturing effective geometry cannot mutate pending source scroll requests on lazy fields. */
#include "textfield_scroll_fixture.hpp"
#include "../src/standard/text_state.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    FVM_Object *field=scroll_field(c);scroll_number(c,field,"scroll",2);scroll_number(c,field,"hscroll",7);
    scroll_number(c,field,"_width",100);scroll_number(c,field,"_height",200);
    FVM_TextFieldState state;assert(fvm_textfield_snapshot(field,state)==FVM_OK && state.scroll==1 && state.hscroll==0);
    FVM_Value root;fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=field;unsigned char *wire=0;size_t size=0;assert(fvm_value_serialize(c,&root,&wire,&size)==FVM_OK);fvm_serialized_free(wire);
    scroll_number(c,field,"_width",40);scroll_number(c,field,"_height",94);
    FVM_TextFieldScrollInfo i=scroll_info(c,field);assert(i.scroll==2 && i.hscroll==7);
    scroll_number(c,field,"_width",100);scroll_number(c,field,"_height",200);i=scroll_info(c,field);assert(i.scroll==1 && i.hscroll==0);
    scroll_number(c,field,"_width",40);scroll_number(c,field,"_height",94);i=scroll_info(c,field);assert(i.scroll==1 && i.hscroll==0);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
    return 0;
}
