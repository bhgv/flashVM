/* Inherited-only descriptor admission is side-effect free and retains SWF4/5 event gates. */
#include "textfield_geometry_events_fixture.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version)for(unsigned folded=0;folded<2;++folded)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char movie[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};movie[3]=(unsigned char)version;assert(fvm_load_memory(c,movie,sizeof(movie))==FVM_OK);
        GeometryEvents events;events.field=scroll_field(c);events.count=0;GeometryObserver observer={&events,1};FVM_Object *prototype=0;
        assert(fvm_object_create(c,&prototype)==FVM_OK && fvm_object_define_method_rooted(c,prototype,folded?"ONSCROLLER":"onScroller",geometry_observe,&observer,events.field)==FVM_OK && fvm_object_set_prototype(c,events.field,prototype)==FVM_OK);
        scroll_number(c,events.field,"_width",60);assert(events.count==((version<6 || (folded && version>=7))?0U:1U) && scroll_info(c,events.field).maxhscroll==0);
        assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
