/* Native prepared-source ownership policy shared with console help; no SDL dependency. */
#ifndef FVM_DESKTOP_AUDIO_MEMORY_H
#define FVM_DESKTOP_AUDIO_MEMORY_H /* Guard the C89 preparation-policy constant. */
#ifndef FVM_DESKTOP_AUDIO_PREPARED_BYTES
#define FVM_DESKTOP_AUDIO_PREPARED_BYTES                                                           \
    (64UL * 1024UL * 1024UL) /* Aggregate segment table, retained snapshots and expansion peak. */
#endif                       /* Explicit build-time ceilings may replace the default. */
#ifndef FVM_DESKTOP_AUDIO_EVENT_BYTES
#define FVM_DESKTOP_AUDIO_EVENT_BYTES \
    (64UL * 1024UL * 1024UL) /* Event mixer/slots plus all retained and incoming native voices. */
#endif /* Explicit host builds may choose a separate event-source ceiling. */
#ifndef FVM_DESKTOP_AUDIO_MIX_BYTES
#define FVM_DESKTOP_AUDIO_MIX_BYTES \
    (1024UL * 1024UL) /* Separate root composition owner, PCM retries, fixed ID deadlines and FIFO origins. */
#endif /* Host may set a smaller ceiling; predicted requests fail before opening a device. */
#endif                       /* FVM_DESKTOP_AUDIO_MEMORY_H */
