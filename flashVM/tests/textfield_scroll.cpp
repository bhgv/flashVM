/* One-based visual lines, pixel overflow and exact contour translations from analytic font metrics. */
#include "textfield_scroll_fixture.hpp"
#include <limits>
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    FVM_Object *field=scroll_field(c);FVM_TextFieldScrollInfo i=scroll_info(c,field);
    assert(i.scroll==1 && !i.hscroll && i.maxscroll==2 && i.maxhscroll==12 && i.bottom_scroll==2 && i.line_count==3 && i.line_height==46 && i.clip_x==2 && i.clip_y==2 && i.clip_width==36 && i.clip_height==90);
    double width=counted_metric(c,field,"textWidth"),height=counted_metric(c,field,"textHeight");FVM_PathBounds initial=scroll_bounds(c,field);
    scroll_number(c,field,"scroll",2.9);scroll_number(c,field,"hscroll",7.9);i=scroll_info(c,field);
    assert(i.scroll==2 && i.hscroll==7 && i.bottom_scroll==3 && i.offset_x==-7 && i.offset_y==-46);
    FVM_PathBounds moved=scroll_bounds(c,field);assert(moved.x_min==initial.x_min-7 && moved.x_max==initial.x_max-7 && moved.y_min==initial.y_min-46 && moved.y_max==initial.y_max-46);
    assert(counted_metric(c,field,"textWidth")==width && counted_metric(c,field,"textHeight")==height && counted_metric(c,field,"maxscroll")==2 && counted_metric(c,field,"maxhscroll")==12 && counted_metric(c,field,"bottomScroll")==3);
    for(unsigned k=0;k<2;++k){scroll_number(c,field,k?"hscroll":"scroll",1e100);assert(counted_metric(c,field,k?"hscroll":"scroll")== (k?12:2));scroll_number(c,field,k?"hscroll":"scroll",-1e100);assert(counted_metric(c,field,k?"hscroll":"scroll")== (k?0:1));}
    FVM_Value v;double invalid[]={std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()};
    for(unsigned k=0;k<3;++k){fvm_value_number(&v,invalid[k]);assert(fvm_object_set(c,field,"scroll",&v)==FVM_INVALID_ARGUMENT && counted_metric(c,field,"scroll")==1);}
    FVM_TextFieldScrollInfo saved=i;--i.struct_size;saved=i;assert(fvm_textfield_scroll_info(c,field,&i)==FVM_INVALID_ARGUMENT && !std::memcmp(&i,&saved,sizeof(i)));
    assert(fvm_textfield_scroll_info(0,field,&i)==FVM_INVALID_ARGUMENT && !std::memcmp(&i,&saved,sizeof(i)));
    scroll_number(c,field,"_height",0);i=scroll_info(c,field);assert(i.maxscroll==3 && i.bottom_scroll==1 && !i.clip_height);
    scroll_number(c,field,"_height",200);i=scroll_info(c,field);assert(i.maxscroll==1 && i.bottom_scroll==3);
    scroll_number(c,field,"_height",94);scroll_number(c,field,"scroll",2);scroll_number(c,field,"hscroll",12);
    assert(fvm_value_string(&v,"A")==FVM_OK && fvm_object_set(c,field,"text",&v)==FVM_OK);i=scroll_info(c,field);assert(i.scroll==1 && !i.hscroll && i.maxscroll==1 && !i.maxhscroll);
    assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);fvm_context_destroy(c);fvm_runtime_destroy(rt);
#else
    FVM_TextFieldScrollInfo info;info.struct_size=sizeof(info);assert(fvm_textfield_scroll_info(0,0,&info)==FVM_INVALID_ARGUMENT);
#endif
    return 0;
}
