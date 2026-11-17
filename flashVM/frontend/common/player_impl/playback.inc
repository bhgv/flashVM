/* Copyright (c) 2026 bhgv
 * EN: Shared player playback implementation; included in owner-thread dependency order.
 * RU: Shared player playback; подключается в порядке owner-thread зависимостей.
 */


/* Play borrowed binary input. VM/SDL objects stay on this thread; source bytes are never freed. */
static int play_bytes(const FVM_DesktopOptions *options,
                      const char *path,
                      const void *bytes,
                      size_t length,
                      int *quit,
                      FVM_DesktopNavigation *navigation)
{
#include "playback_setup.inc"
#include "playback_loop.inc"
#include "playback_cleanup.inc"
}
