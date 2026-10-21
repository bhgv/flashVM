/* Native prepared-source ownership policy shared with console help; no SDL dependency. */
#ifndef FVM_DESKTOP_AUDIO_MEMORY_H
#define FVM_DESKTOP_AUDIO_MEMORY_H /* Guard the C89 preparation-policy constant. */
#ifndef FVM_DESKTOP_AUDIO_PREPARED_BYTES
#define FVM_DESKTOP_AUDIO_PREPARED_BYTES                                                           \
    (64UL * 1024UL * 1024UL) /* Aggregate segment table, retained snapshots and expansion peak. */
#endif                       /* Explicit build-time ceilings may replace the default. */
#endif                       /* FVM_DESKTOP_AUDIO_MEMORY_H */
