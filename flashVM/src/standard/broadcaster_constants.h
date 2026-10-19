/* FlashVM: persisted broadcaster callback IDs; never renumber or reuse these values. */
#ifndef FVM_STANDARD_BROADCASTER_CONSTANTS_H
#define FVM_STANDARD_BROADCASTER_CONSTANTS_H /* Guard graph representation constants. */
#define FVM_BROADCASTER_CALLBACK_INITIALIZE 16000UL /* Namespace initialization bridge. */
#define FVM_BROADCASTER_CALLBACK_LISTENERS 16001UL /* [GC:] Live Array captured by accessor root. */
#define FVM_BROADCASTER_CALLBACK_ADD 16002UL /* Identity registration method. */
#define FVM_BROADCASTER_CALLBACK_REMOVE 16003UL /* Identity removal method. */
#define FVM_BROADCASTER_CALLBACK_BROADCAST 16004UL /* Rooted snapshot dispatcher. */
#endif
