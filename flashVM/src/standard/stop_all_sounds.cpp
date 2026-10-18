/* FlashVM: global stopAllSounds delegates to the same operation as AVM1 Action 0x09. */
#include "stop_all_sounds.hpp"
#include "flashvm/multimedia.h"
/* Return undefined on success and propagate the first stopping error after all attempts. */
FVM_Result fvm_stop_all_sounds(FVM_Context *context, FVM_Object *, unsigned int,
                              const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    return fvm_audio_stop_all(context);
}
