/* FlashVM: AVM1 named quality properties and typed intrinsic quality operations. */
#include "movieclip_quality.hpp"
#include "../avm1/coercion.hpp"
#include "../core/string_internal.hpp"
#include "flashvm/stage.h"
#include "movieclip_quality_constants.h"
#include "movieclip_state.hpp"

/* Compare the entire counted string with an ASCII label, without allocating or trimming. */
static bool quality_label(const FVM_Value &value, const char *label)
{
    size_t count = fvm_string_size(value);
    if (count != std::strlen(label))
        return false;
    for (size_t i = 0; i < count; ++i)
    {
        unsigned char ch = (unsigned char)value.string[i];
        if (ch >= 'a' && ch <= 'z')
            ch = (unsigned char)(ch - 'a' + 'A');
        if (ch != (unsigned char)label[i])
            return false;
    }
    return true;
}

/* Return stage state without invoking replaceable object properties. */
FVM_Result fvm_movieclip_quality_read(FVM_Context *c, bool legacy, FVM_Value *out)
{
    FVM_StageQuality quality;
    FVM_Result result = fvm_stage_get_quality(c, &quality);
    if (result != FVM_OK || !out)
        return result != FVM_OK ? result : FVM_INVALID_ARGUMENT;
    if (legacy)
        fvm_value_number(out, quality == FVM_STAGE_QUALITY_BEST   ? FVM_QUALITY_LEGACY_BEST
                              : quality == FVM_STAGE_QUALITY_HIGH ? FVM_QUALITY_LEGACY_HIGH
                                                                  : FVM_QUALITY_LEGACY_LOW);
    else
        fvm_value_string(out, quality == FVM_STAGE_QUALITY_LOW      ? "LOW"
                              : quality == FVM_STAGE_QUALITY_MEDIUM ? "MEDIUM"
                              : quality == FVM_STAGE_QUALITY_BEST   ? "BEST"
                                                                    : "HIGH");
    return FVM_OK;
}

/* Ignore unrecognized labels and NaN; publish recognized values through the stage API. */
FVM_Result fvm_movieclip_quality_write(FVM_Context *c, bool legacy, const FVM_Value &value)
{
    FVM_StageQuality quality;
    if (legacy)
    {
        if (value.type != FVM_VALUE_NUMBER)
            return FVM_INVALID_ARGUMENT;
        if (value.number != value.number)
            return FVM_OK;
        quality = value.number > FVM_QUALITY_BEST_THRESHOLD ? FVM_STAGE_QUALITY_BEST
                  : value.number == FVM_QUALITY_LEGACY_LOW  ? FVM_STAGE_QUALITY_LOW
                                                            : FVM_STAGE_QUALITY_HIGH;
    }
    else
    {
        if (value.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
        if (quality_label(value, "LOW"))
            quality = FVM_STAGE_QUALITY_LOW;
        else if (quality_label(value, "MEDIUM"))
            quality = FVM_STAGE_QUALITY_MEDIUM;
        else if (quality_label(value, "HIGH"))
            quality = FVM_STAGE_QUALITY_HIGH;
        else if (quality_label(value, "BEST"))
            quality = FVM_STAGE_QUALITY_BEST;
        else
            return FVM_OK;
    }
    return fvm_stage_set_quality(c, quality);
}

/* Named setters ignore absent arguments; numeric null/undefined differ from ActionSetProperty. */
static FVM_Result quality_set(FVM_Context *c, FVM_Object *clip, unsigned int argc, const FVM_Value *argv,
                              FVM_Value *out, bool legacy)
{
    fvm_value_undefined(out);
    if (!argc || fvm_movieclip_is_class_prototype(clip))
        return FVM_OK;
    if (legacy && (argv[0].type == FVM_VALUE_NULL || argv[0].type == FVM_VALUE_UNDEFINED))
        return FVM_OK;
    FVM_Value converted;
    FVM_Result result;
    /* [GC:] Native invocation retains receiver and arguments across script coercion.
     * The converted value is consumed without allocation or another script invocation. */
    if (legacy)
    {
        double number = 0;
        result = fvm_avm1_number(c, argv, &number);
        fvm_value_number(&converted, number);
    }
    else
        result = fvm_avm1_string(c, argv[0], &converted);
    return result == FVM_OK ? fvm_movieclip_quality_write(c, legacy, converted) : result;
}

/* Read the named string property for a display instance. */
FVM_Result fvm_movieclip_quality_get(FVM_Context *c, FVM_Object *clip, unsigned int, const FVM_Value *, FVM_Value *out,
                                     void *)
{
    if (fvm_movieclip_is_class_prototype(clip))
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return fvm_movieclip_quality_read(c, false, out);
}
/* Convert the named string property once. */
FVM_Result fvm_movieclip_quality_set(FVM_Context *c, FVM_Object *clip, unsigned int argc, const FVM_Value *argv,
                                     FVM_Value *out, void *)
{
    return quality_set(c, clip, argc, argv, out, false);
}
/* Read the named legacy numeric property for a display instance. */
FVM_Result fvm_movieclip_highquality_get(FVM_Context *c, FVM_Object *clip, unsigned int, const FVM_Value *,
                                         FVM_Value *out, void *)
{
    if (fvm_movieclip_is_class_prototype(clip))
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return fvm_movieclip_quality_read(c, true, out);
}
/* Convert the named legacy numeric property once. */
FVM_Result fvm_movieclip_highquality_set(FVM_Context *c, FVM_Object *clip, unsigned int argc, const FVM_Value *argv,
                                         FVM_Value *out, void *)
{
    return quality_set(c, clip, argc, argv, out, true);
}
