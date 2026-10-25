/* FlashVM: availability and graph identity of the context-clock global. */
#ifndef FVM_STANDARD_TIME_CONSTANTS_H
#define FVM_STANDARD_TIME_CONSTANTS_H     /* Guard the private clock-global constants. */
#define FVM_TIME_MIN_SWF_VERSION 4U       /* getTimer is available starting with Flash Player 4. */
#define FVM_TIME_GET_TIMER_CALLBACK 163UL /* Append after URL callbacks without changing older IDs. */
#endif
