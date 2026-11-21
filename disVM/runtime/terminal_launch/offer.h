/* Copyright (c) 2026 bhgv
 * EN: Exact declarations for the two one-shot Styx launch profiles.
 * RU: Точные объявления двух профилей одноразового запуска Styx.
 */
#ifndef DVM_STYX_MOUNT_LAUNCH_OFFER_H
#define DVM_STYX_MOUNT_LAUNCH_OFFER_H

#define DVM_STYX_LAUNCH_ENTRY "implementation.dis" /* Program entry exported at the peer root. */
#define DVM_STYX_LAUNCH_REQUEST ".unibus" /* Request file exported at the peer root. */
#define DVM_STYX_LAUNCH_TAG 17UL /* Record tag required by both profiles. */
#define DVM_STYX_LAUNCH_RAW "disvm.launch/1 raw\n" /* Startup bytes only. */
#define DVM_STYX_LAUNCH_CONTEXT "disvm.launch/1 context\n" /* Process selector followed by startup bytes. */

#endif
