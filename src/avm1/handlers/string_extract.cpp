/* FlashVM: StringExtract/MBStringExtract across the supported Flash versions. */
#include "support.hpp"
/* Convert a numeric operand to the low 32 unsigned bits without undefined float-to-int casts. */
static unsigned long integer_bits(double value)
{
    if (value != value || value == std::numeric_limits<double>::infinity() ||
        value == -std::numeric_limits<double>::infinity())
        return 0;
    value = value < 0 ? std::ceil(value) : std::floor(value);
    value = std::fmod(value, 4294967296.0);
    return (unsigned long)(value < 0 ? value + 4294967296.0 : value);
}
/* Represent a low 32-bit result as a signed ActionScript Number on both 32/64-bit hosts. */
static double signed_bits(unsigned long value)
{
    value &= 0xffffffffUL;
    return value >= 0x80000000UL ? (double)value - 4294967296.0 : (double)value;
}
/* Extract counted UTF-16 units after length/start/text conversion in player order. */
extern "C" FVM_Result fvm_exec_string_extract(FVM_Context *c, const FVM_Value *input,
                                              const FVM_Value *start, const FVM_Value *length,
                                              FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, input) != FVM_OK ||
        fvm_validate_value(c, start) != FVM_OK || fvm_validate_value(c, length) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, 3);
        roots.values[0] = *input;
        roots.values[1] = *start;
        roots.values[2] = *length;
        /* [GC:] Retain all three operands while conversion callbacks may invoke script. */
        double length_number = 0, start_number = 0;
        FVM_Result status = fvm_avm1_number(c, &roots.values[2], &length_number);
        if (status == FVM_OK)
            status = fvm_avm1_number(c, &roots.values[1], &start_number);
        FVM_Value text, output;
        if (status == FVM_OK)
            status = fvm_avm1_string(c, roots.values[0], &text);
        if (status != FVM_OK)
            return status;
        fvm_detail::vector<unsigned int> units;
        status = fvm_string_units(text, units);
        if (status != FVM_OK)
            return status;
        if (c->swf_loaded && c->swf_info.version < 6)
            for (size_t unit = 0; unit < units.size(); ++unit)
                if (units[unit] > 127)
                    return FVM_UNSUPPORTED;
        double signed_start = signed_bits(integer_bits(start_number));
        double signed_length = signed_bits(integer_bits(length_number));
        size_t start = signed_start >= 1 ? (size_t)signed_start - 1 : 0;
        size_t end = units.size();
        if (signed_length >= 0 && start <= end && (size_t)signed_length <= end - start)
            end = start + (size_t)signed_length;
        if (start > end)
            start = end;
        fvm_detail::vector<unsigned short> selected(end - start);
        for (size_t unit = start; unit < end; ++unit)
            selected[unit - start] = (unsigned short)units[unit];
        status = fvm_value_string_utf16(c, selected.begin(), selected.size(), &output);
        if (status != FVM_OK)
            return status;
        *out = output;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
