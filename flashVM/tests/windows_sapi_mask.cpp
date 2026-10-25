/* Native Windows SDK contract; also usable as a strict C++98 cross-compile probe. */
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <sapi.h>
#include "../src/multimedia/speech/windows_constants.h"

/* Independent split-word oracle: recognition=38, reserved checks=30 and 33. */
typedef char mask_low_word[((unsigned long)FVM_WINDOWS_SAPI_FINAL_EVENTS == 0x40000000UL) ? 1 : -1];
typedef char mask_high_word[((unsigned long)(FVM_WINDOWS_SAPI_FINAL_EVENTS >> 32) == 0x42UL) ? 1 : -1];


int main()
{
    const ULONGLONG mask = FVM_WINDOWS_SAPI_FINAL_EVENTS;
    if (!((unsigned long)mask == 0x40000000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!((unsigned long)(mask >> 32) == 0x42UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
