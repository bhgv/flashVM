/* Native count primitives. They do not acquire object lifetime or implement GC. */
#ifndef DVM_REFCOUNT_H
#define DVM_REFCOUNT_H
#ifdef __cplusplus
extern "C"
{
#endif


/* Native operation outcome; no destructor or memory reclamation is invoked. */
typedef enum DVM_RefcountResult
{
    DVM_REFCOUNT_REFUSED = 0, /* Zero/negative/overflow: count remains unchanged. */
    DVM_REFCOUNT_LIVE = 1,    /* Successful retain or nonfinal release. */
    DVM_REFCOUNT_LAST = 2     /* Successful last release: caller now owns retirement. */
} DVM_RefcountResult;


/* The portable fallback uses the port registry mutex: do not call this API
 * while already holding that mutex. Compiler atomics use no registry lock. */


/* Read an aligned, already-live native unsigned counter. counter is borrowed;
 * NULL yields zero. Lifetime must be held independently before this call. All
 * concurrent accesses must use this API. Acquire ordering; no allocation. */
unsigned long dvm_refcount_load_u(const unsigned long *counter);


/* Retain an already-live native unsigned counter. Refuse zero or ULONG_MAX
 * without mutation; never resurrect an object. Borrowed/aligned counter must
 * remain live through the call. Acquire/release ordering; no allocation. */
DVM_RefcountResult dvm_refcount_retain_u(unsigned long *counter);


/* Drop one owned native unsigned reference. Refuse zero/NULL. LAST transfers
 * sole retirement responsibility to the caller after acquire/release ordering;
 * memory is not freed here. Storage must remain live until the successful RMW.
 * No counter access follows that RMW: another last owner may reclaim before a
 * nonfinal call returns. LAST's caller retires only after receiving the result. */
DVM_RefcountResult dvm_refcount_release_u(unsigned long *counter);

/* Atomically claim the final owner, or leave the counter unchanged. */
int dvm_refcount_claim_last_u(unsigned long *counter);


/* Signed Type-count counterpart: acquire load of aligned borrowed live storage.
 * NULL yields zero; a negative diagnostic value is preserved, not normalized. */
int dvm_refcount_load_i(const int *counter);


/* Retain signed Type count; refuse <=0 or INT_MAX. Same lifetime/order rules
 * as retain_u. Ordinary initialization is allowed only before publication. */
DVM_RefcountResult dvm_refcount_retain_i(int *counter);


/* Drop owned signed Type reference; refuse <=0/NULL. LAST does not run a Type
 * destructor. Same lifetime/order rules as release_u; no allocation. */
DVM_RefcountResult dvm_refcount_release_i(int *counter);


/* Compare live native state, including zero and reserved sentinel values. */
int dvm_refcount_compare_i(int *counter, int expected, int desired);

/* Report implementation selection per native type. No parameters, ownership
 * transfer or allocation; returned static text is immutable for process life. */
const char *dvm_refcount_backend(void);
#ifdef __cplusplus
}
#endif
#endif
