/* FlashVM: allocation-free activation links and owning postmortem state. */
#ifndef FVM_CRASH_STATE_HPP
#define FVM_CRASH_STATE_HPP /* Private crash state guard. */
#include "../details/string.hpp"
#include "../details/vector.hpp"
#include "flashvm/crash.h"

struct FVM_CrashActivation;
struct FVM_CrashReport;
struct FVM_DebugSymbols;


/* Release symbol bytes through their defining implementation. */
void fvm_debug_symbols_destroy(FVM_DebugSymbols *);


/* Report storage contains copied bytes only, never untraced managed references. */
struct FVM_CrashState
{
    FVM_CrashDetail detail;        /* Natural inspection or explicit entry preservation. */
    FVM_Result preparation_status; /* Failed FORCE preservation without replacing VM execution status. */
    FVM_DebugSymbols *symbols;     /* Owned optional SWF/SWD source bundle. */
    FVM_CrashMode mode;            /* Configured failure policy. */
    fvm_detail::string path;       /* Optional copied output path. */
    FVM_CrashHandler handler;      /* Borrowed synchronous debugger callback. */
    void *userdata;                /* Borrowed host debugger state. */
    FVM_CrashActivation *active;   /* Stack-owned links, valid only during execution. */
    FVM_CrashReport *report;       /* Owned immutable copied postmortem data. */
    bool busy;                     /* Prevent callback/report recursion. */
    bool propagating;              /* Preserve the deepest report during one error unwind. */
    FVM_Result diagnostic_status;  /* Capture or file-write failure independent of VM status. */


    /* Initialize the disabled diagnostic policy. */
    FVM_CrashState()
        : detail(FVM_CRASH_DETAIL_NATURAL), preparation_status(FVM_OK), symbols(0), mode(FVM_CRASH_NONE), handler(0),
          userdata(0), active(0), report(0), busy(false), propagating(false), diagnostic_status(FVM_OK)
    {
    }

    /* Release only copied report storage. */
    ~FVM_CrashState();

  private:

    FVM_CrashState(const FVM_CrashState &);            /* Owning state is not copyable. */
    FVM_CrashState &operator=(const FVM_CrashState &); /* Preserve unique report ownership. */
};
#endif
