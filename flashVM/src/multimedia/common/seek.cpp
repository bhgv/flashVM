/* OS: callback-selected. Reposition a decoder without retaining old buffer views. */
#include "session.hpp" /* Access the optional backend seek callback. */
#include <float.h>     /* Bound finite timestamps in C++98. */


/* Seek in seconds; the callback owns demuxer and decoder flushing. */
FVM_Result fvm_media_seek(FVM_MediaSession *session, double seconds) /* Validate the host timestamp. */
{                                                                    /* Begin synchronous seeking. */
    if (!session || !(seconds >= 0.0 && seconds <= DBL_MAX))         /* Reject negative, NaN and infinite offsets. */
    {                                                                /* Begin conditional branch: Reject negative, NaN and infinite offsets. */
        return FVM_INVALID_ARGUMENT;                                 /* Reject negative, NaN and infinite offsets. */
    } /* End conditional branch: Reject negative, NaN and infinite offsets. */
    if (!session->backend.seek) /* Keep nonseekable backends explicit. */
    {                           /* Begin conditional branch: Keep nonseekable backends explicit. */
        return FVM_UNSUPPORTED; /* Keep nonseekable backends explicit. */
    } /* End conditional branch: Keep nonseekable backends explicit. */
    return session->backend.seek(session->stream, seconds); /* Delegate timestamp conversion and state reset. */
} /* Finish seeking. */
