/* OS: all supported targets. Validate and own a host decoder/capture session. */
#include "session.hpp" /* Import the bridge table and private session layout. */
#include <new>         /* Use nonthrowing C++98 allocation. */


/* Open one private stream; callbacks must clean partial state when they fail. */
FVM_Result fvm_media_open_internal(const FVM_MediaBackend *backend, const FVM_MediaRequest *request, /* Begin validated opening. */
                                   FVM_MediaSession **out)                                           /* Begin validated opening. */
{                                                                                                    /* Enter the synchronous open operation. */
    if (!out)                                                                                        /* Require an output slot before touching it. */
    {                                                                                                /* Begin conditional branch: Require an output slot before touching it. */
        return FVM_INVALID_ARGUMENT;                                                                 /* Require an output slot before touching it. */
    } /* End conditional branch: Require an output slot before touching it. */
    *out = 0;                                                                                                   /* Never expose a stale session after failure. */
    if (!backend || !request || backend->struct_size != sizeof(*backend) ||                                     /* Reject incompatible or incomplete tables. */
        backend->abi_version != FVM_MEDIA_ABI_VERSION || !backend->open || !backend->read || !backend->close || /* Reject incompatible or incomplete tables. */
        !request->url || !*request->url)                                                                        /* Reject incompatible or incomplete tables. */
    {                                                                                                           /* Begin scope: Reject incompatible or incomplete tables. */
        return FVM_INVALID_ARGUMENT;                                                                            /* Reject incompatible or incomplete tables. */
    } /* End scope: Reject incompatible or incomplete tables. */
    FVM_MediaSession *session = new (std::nothrow) FVM_MediaSession; /* Allocate an unmanaged wrapper. */
    if (!session)                                                    /* Do not invoke callbacks without a wrapper. */
    {                                                                /* Begin conditional branch: Do not invoke callbacks without a wrapper. */
        return FVM_OUT_OF_MEMORY;                                    /* Do not invoke callbacks without a wrapper. */
    } /* End conditional branch: Do not invoke callbacks without a wrapper. */
    session->backend = *backend;                                             /* Snapshot the caller's callback table. */
    session->stream = 0;                                                     /* Establish a safe pre-open state. */
    session->kind =                                                          /* Camera decoders produce video frames. */
        request->kind == FVM_MEDIA_CAMERA ? FVM_MEDIA_VIDEO : request->kind; /* Camera decoders produce video frames. */
    FVM_Result result = backend->open(backend->userdata, request,            /* Delegate to the selected real library or system bridge. */
                                      &session->stream);                     /* Delegate to the selected real library or system bridge. */
    if (result != FVM_OK || !session->stream)                                /* Publish only a successful non-null stream. */
    {                                                                        /* Clean the wrapper after an unsuccessful open. */
        if (result == FVM_OK)                                                /* Treat an empty successful handle as a backend error. */
        {                                                                    /* Begin conditional branch: Treat an empty successful handle as a backend error. */
            result = FVM_ERROR;                                              /* Treat an empty successful handle as a backend error. */
        } /* End conditional branch: Treat an empty successful handle as a backend error. */
        delete session; /* A failing callback owns its own partial-allocation cleanup. */
        return result;  /* Preserve the backend failure category. */
    } /* Finish failed-open cleanup. */
    *out = session; /* Transfer wrapper ownership to the host. */
    return FVM_OK;  /* Report a fully initialized session. */
} /* Finish the common open implementation. */
