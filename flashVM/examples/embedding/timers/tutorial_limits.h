/* FlashVM timer tutorial: human-visible delay, independent of VM scheduler bounds. */
#ifndef FVM_TUTORIAL_TIMER_LIMITS_H /* Keep local constants private to this example. */
#define FVM_TUTORIAL_TIMER_LIMITS_H /* Prevent repeated definitions. */
#define FVM_TUTORIAL_TIMER_DELAY_MS 1000.0 /* Poll after about one second to observe a delivery. */
#define FVM_TUTORIAL_TIMER_WAIT_CAP_MS 1000UL /* Maximum host wait hint used by this tutorial. */
#endif /* End the local tutorial constants. */
