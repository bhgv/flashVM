/* Four OS workers own separate VM/listener graphs across 128 synchronous scroll event epochs. */
#include "textfield_geometry_events_fixture.hpp"
#include "../ports/thread_port.h"
#ifdef FVM_HAS_FONTS
struct ScrollEventJob {unsigned done;};
static void event_worker(void *raw)
{
    ScrollEventJob *job=(ScrollEventJob*)raw;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    GeometryEvents events;events.field=scroll_field(c);events.count=0;GeometryObserver own={&events,1},observer={&events,2};
    assert(fvm_object_define_method_rooted(c,events.field,"onScroller",geometry_observe,&own,events.field)==FVM_OK);geometry_listener(c,&observer);
    for(unsigned epoch=0;epoch<32;++epoch)
    {
        events.count=0;scroll_number(c,events.field,"_width",epoch%2?40:60);
        assert(events.count==2 && events.order[0]==1 && events.order[1]==2 && events.last.maxhscroll==(epoch%2?12UL:0UL));
        assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);++job->done;
    }
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
#endif
int main()
{
#ifdef FVM_HAS_FONTS
    ScrollEventJob jobs[4];FVM_PortThread *threads[4];for(unsigned i=0;i<4;++i){jobs[i].done=0;threads[i]=fvm_port_thread_start(event_worker,jobs+i);assert(threads[i]);}for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(jobs[i].done==32);}
#endif
    return 0;
}
