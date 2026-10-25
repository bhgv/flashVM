/* Tutorial-only frame export budget; the native API accepts an explicit host-selected capacity. */
#ifndef EXAMPLE_CAPTURE_RESOURCE_LIMITS_H                      /* Guard the tutorial's semantic resource limit. */
#define EXAMPLE_CAPTURE_RESOURCE_LIMITS_H                      /* No VM, SDK or device initialization occurs here. */
#define EXAMPLE_CAPTURE_FRAME_CAPACITY (4UL * 1024UL * 1024UL) /* Four MiB per slot; three slots use twelve MiB. */
#endif                                                         /* Larger movies need a deliberate host budget, not an implicit unbounded allocation. */
