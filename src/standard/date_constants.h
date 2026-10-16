/* FlashVM: semantic bounds shared by Date argument projection and time clipping. */
#ifndef FVM_STANDARD_DATE_CONSTANTS_H
#define FVM_STANDARD_DATE_CONSTANTS_H
/* Native Date methods project at most the seven calendar component operands. */
#define FVM_DATE_ARGUMENT_LIMIT 7
/* Largest timestamp magnitude accepted by the current calendar engine, in milliseconds. */
#define FVM_DATE_TIME_CLIP_LIMIT 8640000000000000.0
#endif
