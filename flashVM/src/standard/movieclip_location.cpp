/* Display the retained content origin without performing transport or invoking script coercion. */
#include "movieclip_location.hpp"
#include "movieclip_state.hpp"
#include "flashvm/movieclip.h"

/* The source is counted managed storage, including final redirect and detached-child provenance.
 * [GC:] The accessor/action caller retains the clip while the returned string is adopted. */
FVM_Result fvm_movieclip_url_read(FVM_Context *c, FVM_Object *clip, FVM_Value *out)
{
    FVM_Result result = fvm_movieclip_get_source(c, clip, out);
    if (result == FVM_NOT_FOUND)
        return fvm_value_string(out, "");
    return result;
}

/* A getter-only descriptor prevents assignments from changing the loader's source base. */
FVM_Result fvm_movieclip_url_get(FVM_Context *c, FVM_Object *clip, unsigned, const FVM_Value *,
                                 FVM_Value *out, void *)
{
    if (fvm_movieclip_is_class_prototype(clip))
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return fvm_movieclip_url_read(c, clip, out);
}
