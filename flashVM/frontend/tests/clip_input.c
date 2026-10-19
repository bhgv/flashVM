/* Real SDL adapter: namespace publication, retained clip input, primary touch, errors and FIFO deferral. */
#include "../common/sdl_window.c"
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include "flashvm/stage.h"
#include "clip_input_movie.h"
#include <assert.h>
struct Trace {FVM_DesktopWindow *view;FVM_Object *root;unsigned count,namespace_count,events[32],append,fail;double xs[32],ys[32];unsigned keys[32];};
static void motion(FVM_DesktopWindow *v,int x,int y)
{SDL_Event e;memset(&e,0,sizeof(e));e.type=SDL_MOUSEMOTION;e.motion.x=x;e.motion.y=y;assert(handle_event(v,&e));}
static void button(FVM_DesktopWindow *v,Uint32 type,int x,int y)
{SDL_Event e;memset(&e,0,sizeof(e));e.type=type;e.button.button=SDL_BUTTON_LEFT;e.button.x=x;e.button.y=y;assert(handle_event(v,&e));}
static void key(FVM_DesktopWindow *v,Uint32 type)
{SDL_Event e;memset(&e,0,sizeof(e));e.type=type;e.key.keysym.sym=SDLK_LEFT;assert(handle_event(v,&e));}
static void finger(FVM_DesktopWindow *v,Uint32 type,float x,float y)
{SDL_Event e;memset(&e,0,sizeof(e));e.type=type;e.tfinger.touchId=10;e.tfinger.fingerId=111;e.tfinger.x=x;e.tfinger.y=y;assert(handle_event(v,&e));}
static FVM_Result namespace_event(FVM_Context *c,FVM_Object *self,unsigned n,const FVM_Value *a,FVM_Value *out,void *raw)
{struct Trace *t=(struct Trace *)raw;(void)c;(void)self;(void)a;assert(!n);++t->namespace_count;fvm_value_undefined(out);return FVM_OK;}
static FVM_Result receive(FVM_Context *c,FVM_Object *self,unsigned n,const FVM_Value *a,FVM_Value *out,void *raw)
{
 struct Trace *t=(struct Trace *)raw;unsigned i=t->count++;FVM_KeyboardState state;assert(t->namespace_count==t->count);(void)self;assert(n==1 && a[0].type==FVM_VALUE_NUMBER && i<32);t->events[i]=(unsigned)a[0].number;assert(fvm_input_get_pointer(c,t->xs+i,t->ys+i)==FVM_OK);memset(&state,0,sizeof(state));state.struct_size=sizeof(state);assert(fvm_input_get_keyboard(c,&state)==FVM_OK);t->keys[i]=state.down[FVM_KEY_LEFT];
 assert(fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT && fvm_desktop_window_pump_clip_input(t->view,c,t->root,0,0)==FVM_INVALID_ARGUMENT);
 if(t->append){t->append=0;motion(t->view,70,80);}fvm_value_undefined(out);return t->fail==t->count?FVM_ERROR:FVM_OK;
}
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *root=0,*mouse=0,*keyboard=0;FVM_DesktopWindow *view;struct Trace t;unsigned before;int pending;
 assert(SDL_setenv("SDL_VIDEODRIVER","dummy",1)==0);view=fvm_desktop_window_open("clip-input",200,100);assert(view && fvm_desktop_window_events(view,0));view->event_count=view->event_head=0;memset(&t,0,sizeof(t));t.view=view;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);assert(fvm_movieclip_load_swf(c,input_movie,sizeof(input_movie),"root",&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK && fvm_movieclip_advance(c,root,1)==FVM_OK);t.root=root;assert(fvm_register_function(c,"clipInput",receive,&t)==FVM_OK);
 assert(fvm_object_create(c,&mouse)==FVM_OK && fvm_object_create(c,&keyboard)==FVM_OK);assert(fvm_object_define_method_rooted(c,mouse,"onMouseMove",namespace_event,&t,0)==FVM_OK && fvm_object_define_method_rooted(c,mouse,"onMouseDown",namespace_event,&t,0)==FVM_OK && fvm_object_define_method_rooted(c,mouse,"onMouseUp",namespace_event,&t,0)==FVM_OK && fvm_object_define_method_rooted(c,keyboard,"onKeyDown",namespace_event,&t,0)==FVM_OK && fvm_object_define_method_rooted(c,keyboard,"onKeyUp",namespace_event,&t,0)==FVM_OK);assert(fvm_input_add_mouse_listener(c,mouse)==FVM_OK && fvm_input_add_key_listener(c,keyboard)==FVM_OK);

 motion(view,10,20);button(view,SDL_MOUSEBUTTONDOWN,12,22);key(view,SDL_KEYDOWN);key(view,SDL_KEYUP);button(view,SDL_MOUSEBUTTONUP,30,40);
 assert(fvm_desktop_window_pump_clip_input(view,c,root,5,7)==FVM_OK && t.count==5);assert(t.events[0]==8 && t.xs[0]==15 && t.ys[0]==27 && t.events[1]==16 && t.events[2]==64 && t.keys[2] && t.events[3]==128 && !t.keys[3] && t.events[4]==32);
 finger(view,SDL_FINGERDOWN,.25f,.5f);finger(view,SDL_FINGERMOTION,.5f,.25f);finger(view,SDL_FINGERUP,.5f,.25f);assert(fvm_desktop_window_pump_clip_input(view,c,root,5,7)==FVM_OK && t.count==8);assert(t.events[5]==16 && t.xs[5]==55 && t.ys[5]==57 && t.events[6]==8 && t.events[7]==32);
 before=t.count;t.append=1;motion(view,1,2);assert(fvm_desktop_window_pump_clip_input(view,c,root,0,0)==FVM_OK && t.count==before+1 && view->event_count==1);assert(fvm_desktop_window_pump_clip_input(view,c,root,0,0)==FVM_OK && t.count==before+2);
 t.fail=t.count+1;motion(view,2,3);motion(view,4,5);assert(fvm_desktop_window_pump_clip_input(view,c,root,0,0)==FVM_ERROR && view->event_count==1);t.fail=0;assert(fvm_desktop_window_pump_clip_input(view,c,root,0,0)==FVM_OK && !view->event_count);
 assert(fvm_stage_take_update(c,&pending)==FVM_OK && !pending);assert(fvm_gc_collect(c,0)==FVM_OK);fvm_desktop_window_close(view);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
