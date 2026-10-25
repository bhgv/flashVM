/* Private frame-observer validation; the public copy utility remains allocation-free C89. */
#ifndef FVM_DESKTOP_FRAME_INTERNAL_H
#define FVM_DESKTOP_FRAME_INTERNAL_H /* Guard internal observer contracts. */
#include "frame.h"


/* Reject malformed policies before any source, worker, VM or window startup. */
int fvm_desktop_frame_observer_valid(const FVM_DesktopFrameObserver *observer);
#endif
