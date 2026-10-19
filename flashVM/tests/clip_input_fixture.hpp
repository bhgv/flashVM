/* Reuse independently encoded SWF placement handlers to exercise whole-scene input delivery. */
#include "clip_actions_fixture.hpp"
#include "flashvm/input.h"
#include "flashvm/keyboard.h"
static FVM_Object *input_actor(FVM_Context *c,FVM_Object **out=0)
{FVM_Object *root=clip_load(c,clip_movie(8,clip_event_table(8,0xf8,action(42))));assert(fvm_movieclip_advance(c,root,1)==FVM_OK);if(out)*out=root;return get(c,root,"actor").object;}
static FVM_Object *input_duplicate(FVM_Context *c,FVM_Object *clip,const char *name,unsigned depth)
{FVM_Value a[2],out;assert(fvm_value_string(a,name)==FVM_OK);fvm_value_number(a+1,depth);assert(fvm_object_call(c,clip,"duplicateMovieClip",2,a,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT);return out.object;}
static void input_clean(FVM_Context *c)
{clip_clean(c);assert(!c->clip_input_dispatching);}
