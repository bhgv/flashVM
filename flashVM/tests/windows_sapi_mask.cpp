/* Native Windows SDK contract; also usable as a strict C++98 cross-compile probe. */
#include <windows.h>
#include <sapi.h>
#include "../src/multimedia/speech/windows_constants.h"
#include <cassert>

/* Independent split-word oracle: recognition=38, reserved checks=30 and 33. */
typedef char mask_low_word[((unsigned long)FVM_WINDOWS_SAPI_FINAL_EVENTS == 0x40000000UL) ? 1 : -1];
typedef char mask_high_word[((unsigned long)(FVM_WINDOWS_SAPI_FINAL_EVENTS >> 32) == 0x42UL) ? 1 : -1];
int main()
{
    const ULONGLONG mask = FVM_WINDOWS_SAPI_FINAL_EVENTS;
    assert((unsigned long)mask == 0x40000000UL);
    assert((unsigned long)(mask >> 32) == 0x42UL);
    return 0;
}
