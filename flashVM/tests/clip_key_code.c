/* C89 independent wire-code oracle: special keys, every printable ASCII and invalid snapshots. */
#include "flashvm/keyboard.h"
#include "flashvm/clip_actions.h"
#include <assert.h>
#include <string.h>
/* Validate pure mapping without a runtime, allocation, state publication or callbacks. */
int main(void)
{
    const unsigned virtuals[] = {37,39,36,35,45,46,8,13,38,40,33,34,9,27};
    const unsigned wire[] = {1,2,3,4,5,6,8,13,14,15,16,17,18,19};
    FVM_KeyboardState state, before;
    unsigned i, out;
    memset(&state,0,sizeof(state));state.struct_size=sizeof(state);
    for(i=0;i<sizeof(virtuals)/sizeof(virtuals[0]);++i)
    {
        memset(state.down,0,sizeof(state.down));state.last_code=virtuals[i];
        state.down[state.last_code]=1;state.last_ascii='Z';before=state;out=999;
        assert(fvm_input_clip_key_code(&state,1,&out)==FVM_OK && out==wire[i]);
        assert(!memcmp(&state,&before,sizeof(state)));
        assert(fvm_input_clip_key_code(&state,0,&out)==FVM_OK && !out);
    }
    memset(state.down,0,sizeof(state.down));state.last_code=65;state.down[65]=1;
    for(i=0;i<=127;++i)
    {
        state.last_ascii=i;out=999;
        assert(fvm_input_clip_key_code(&state,1,&out)==FVM_OK);
        assert(out==(i>=32 && i<=126?i:0));
    }
    state.down[65]=0;state.last_ascii='A';
    assert(fvm_input_clip_key_code(&state,1,&out)==FVM_OK && !out);
    state.last_code=0;
    assert(fvm_input_clip_key_code(&state,1,&out)==FVM_OK && !out);
    out=999;assert(fvm_input_clip_key_code(0,1,&out)==FVM_INVALID_ARGUMENT && out==999);
    assert(fvm_input_clip_key_code(&state,2,&out)==FVM_INVALID_ARGUMENT && out==999);
    assert(fvm_input_clip_key_code(&state,1,0)==FVM_INVALID_ARGUMENT);
    for(i=0;i<7;++i)
    {
        before=state;
        if(i==0)state.struct_size=0;
        if(i==1)state.down[0]=1;
        if(i==2)state.down[200]=2;
        if(i==3)state.last_code=256;
        if(i==4)state.last_ascii=128;
        if(i==5)state.caps_lock=2;
        if(i==6)state.num_lock=-1;
        assert(fvm_input_clip_key_code(&state,0,&out)==FVM_INVALID_ARGUMENT && out==999);
        state=before;
    }
    return 0;
}
