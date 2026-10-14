/* FlashVM: deterministic per-context random state shared by Math and AVM1 operations. */
#ifndef FVM_RANDOM_CONSTANTS_H
#define FVM_RANDOM_CONSTANTS_H /* Random generator parameter guard. */
#define FVM_RANDOM_INITIAL_STATE                                                                   \
    1UL /* Historical FlashVM context seed; not a cryptographic source. */
#define FVM_RANDOM_MULTIPLIER                                                                      \
    1664525UL /* Multiplier of the existing 32-bit linear congruential generator. */
#define FVM_RANDOM_INCREMENT 1013904223UL /* Additive term of the existing context generator. */
#endif
