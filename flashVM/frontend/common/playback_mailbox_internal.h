/* Private owner-publication boundary; worker-facing functions never receive managed pointers. */
#ifndef FVM_DESKTOP_PLAYBACK_MAILBOX_INTERNAL_H
#define FVM_DESKTOP_PLAYBACK_MAILBOX_INTERNAL_H /* Guard the internal publication declaration. */
#include "playback_mailbox.h"
/* Copy a valid owner observation, return one pending intent and clear only nonterminal intent.
 * Invalid snapshots/handles fail closed with STOP; caller-owned managed pointers are never stored.
 */
FVM_DesktopPlaybackCommand
fvm_desktop_playback_mailbox_publish(FVM_DesktopPlaybackMailbox *mailbox,
                                     const FVM_DesktopPlaybackState *state);
#endif
