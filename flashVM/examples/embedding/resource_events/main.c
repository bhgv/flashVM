/* Independent native observers; see README.md and doc/RESOURCE_EVENTS.md. */
#include "flashvm/revents.h" /* Copy events without constructing a VM or transferring managed pointers. */
#include <stdio.h>           /* Console output is only this example's presentation adapter. */


/* This resource has no executable methods; its READ operation is a documented placeholder. */
static FVM_RResult inspect(const FVM_RRequest *q, void *reply, size_t capacity, size_t *used, /* Continue the explicitly borrowed arguments of this native call. */
                           void *host)                                                        /* Owner callback ABI. */
{                                                                                             /* The example exercises event delivery rather than VM-specific inspection. */
    (void)q;                                                                                  /* Keep this native declaration or continuation within its documented owner scope. */
    (void)reply;                                                                              /* Keep this native declaration or continuation within its documented owner scope. */
    (void)capacity;                                                                           /* Keep this native declaration or continuation within its documented owner scope. */
    (void)host;                                                                               /* No borrowed input is retained. */
    *used = 0;                                                                                /* Return an explicitly empty reply. */
    return FVM_R_OK;                                                                          /* A real resource would inspect its owner-managed state here. */
} /* End the owner handler. */


/* Single-owner demonstration; real shared threads must supply mutex and owner hooks. */
int main(void)                                          /* No console-dependent control API is required. */
{                                                       /* Every handle is native and scoped to one dispatcher lifetime. */
    FVM_RDispatch *dispatch = 0;                        /* Own this allocation until successful destruction. */
    FVM_RHandle resource;                               /* Registered resource incarnation. */
    FVM_ROperation operation = {1, FVM_R_READ};         /* Server chooses the required permission. */
    FVM_RWatch watches[2];                              /* Separate readers, not competing consumers of one queue. */
    FVM_REvent event;                                   /* Copied output survives the next take. */
    FVM_RResource row;                                  /* Discovery returns copied names and filtered operation metadata. */
    unsigned long sequence;                             /* Owner publication returns the shared sequence. */
    size_t count, i;                                    /* Count discovery rows and tutorial readers. */
    int result = 1;                                     /* Every failed step takes the same owned cleanup path. */
    if (fvm_rdispatch_create(0, &dispatch) != FVM_R_OK) /* Check fvm_rdispatch_create before using its output. */
    {                                                   /* Begin conditional branch: Check fvm_rdispatch_create before using its output. */
        return 1;                                       /* NULL policy means single-owner use. */
    } /* End conditional branch: Check fvm_rdispatch_create before using its output. */
    if (fvm_rdispatch_register(dispatch, "/debug/events", &operation, 1, inspect, 0, &resource) != /* Check fvm_rdispatch_register before using its output. */
        FVM_R_OK)                                                                                  /* Keep this native declaration or continuation within its documented owner scope. */
    {                                                                                              /* Begin scope: Keep this native declaration or continuation within its documented owner scope. */
        goto cleanup;                                                                              /* Register a copied path. */
    } /* End scope: Keep this native declaration or continuation within its documented owner scope. */
    for (i = 0; i < 2; ++i)                                                /* Keep this native declaration or continuation within its documented owner scope. */
    {                                                                      /* Grant and subscribe each authenticated adapter-assigned session. */
        if (fvm_rdispatch_grant(dispatch, resource, (unsigned long)i + 1,  /* Check fvm_rdispatch_grant before using its output. */
                                FVM_R_READ | FVM_R_SUBSCRIBE) != FVM_R_OK) /* Keep this native declaration or continuation within its documented owner scope. */
        {                                                                  /* Begin scope: Keep this native declaration or continuation within its documented owner scope. */
            goto cleanup;                                                  /* Explicit authority. */
        } /* End scope: Keep this native declaration or continuation within its documented owner scope. */
        if (fvm_rdispatch_subscribe(dispatch, (unsigned long)i + 1, resource, watches + i) != /* Check fvm_rdispatch_subscribe before using its output. */
            FVM_R_OK)                                                                         /* Keep this native declaration or continuation within its documented owner scope. */
        {                                                                                     /* Begin scope: Keep this native declaration or continuation within its documented owner scope. */
            goto cleanup;                                                                     /* Queue starts empty. */
        } /* End scope: Keep this native declaration or continuation within its documented owner scope. */
    } /* Both readers now independently observe future publications. */
    if (fvm_rdispatch_resources(dispatch, 1, &row, 1, &count) != FVM_R_OK || count != 1) /* Check fvm_rdispatch_resources before using its output. */
    {                                                                                    /* Begin conditional branch: Check fvm_rdispatch_resources before using its output. */
        goto cleanup;                                                                    /* No hidden resources are disclosed. */
    } /* End conditional branch: Check fvm_rdispatch_resources before using its output. */
    if (fvm_rdispatch_publish(dispatch, resource, 1, "stop", 4, &sequence) != FVM_R_OK) /* Check fvm_rdispatch_publish before using its output. */
    {                                                                                   /* Begin conditional branch: Check fvm_rdispatch_publish before using its output. */
        goto cleanup;                                                                   /* Copy once for each observer; no socket is opened. */
    } /* End conditional branch: Check fvm_rdispatch_publish before using its output. */
    for (i = 0; i < 2; ++i)                                                                 /* Keep this native declaration or continuation within its documented owner scope. */
    {                                                                                       /* Taking one reader never removes the other reader's copy. */
        if (fvm_rdispatch_event_take(dispatch, (unsigned long)i + 1, watches[i], &event) != /* Check fvm_rdispatch_event_take before using its output. */
            FVM_R_OK)                                                                       /* Keep this native declaration or continuation within its documented owner scope. */
        {                                                                                   /* Begin scope: Keep this native declaration or continuation within its documented owner scope. */
            goto cleanup;                                                                   /* Authenticate every read. */
        } /* End scope: Keep this native declaration or continuation within its documented owner scope. */
        printf("session=%lu path=%s sequence=%lu missed=%lu bytes=%lu\n", (unsigned long)i + 1, /* Continue the explicitly borrowed arguments of this native call. */
               row.path, event.sequence, event.missed,                                          /* Continue the explicitly borrowed arguments of this native call. */
               (unsigned long)event.size);                                                      /* Print native metadata. */
        if (fvm_rdispatch_unsubscribe(dispatch, (unsigned long)i + 1, watches[i]) != FVM_R_OK)  /* Check fvm_rdispatch_unsubscribe before using its output. */
        {                                                                                       /* Begin conditional branch: Check fvm_rdispatch_unsubscribe before using its output. */
            goto cleanup;                                                                       /* Close this reader only. */
        } /* End conditional branch: Check fvm_rdispatch_unsubscribe before using its output. */
    } /* Both copied event payloads have been received. */
    result = 0;                                            /* Expected: two rows with sequence=1, missed=0 and bytes=4. */
cleanup:                                                   /* No accepted command is outstanding; disconnect also cleans partial subscriptions. */
    if (fvm_rdispatch_disconnect(dispatch, 1) != FVM_R_OK) /* Check fvm_rdispatch_disconnect before using its output. */
    {                                                      /* Begin conditional branch: Check fvm_rdispatch_disconnect before using its output. */
        result = 1;                                        /* Revoke session one and discard its native state. */
    } /* End conditional branch: Check fvm_rdispatch_disconnect before using its output. */
    if (fvm_rdispatch_disconnect(dispatch, 2) != FVM_R_OK) /* Check fvm_rdispatch_disconnect before using its output. */
    {                                                      /* Begin conditional branch: Check fvm_rdispatch_disconnect before using its output. */
        result = 1;                                        /* Other session cleanup is independent. */
    } /* End conditional branch: Check fvm_rdispatch_disconnect before using its output. */
    if (fvm_rdispatch_destroy(&dispatch) != FVM_R_OK) /* Check fvm_rdispatch_destroy before using its output. */
    {                                                 /* Begin conditional branch: Check fvm_rdispatch_destroy before using its output. */
        result = 1;                                   /* Free native queues after consumers are quiesced. */
    } /* End conditional branch: Check fvm_rdispatch_destroy before using its output. */
    return result; /* No SDK/VM/SDL shutdown is needed: none was started. */
} /* End the native observer tutorial. */
