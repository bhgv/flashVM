/* OS: Linux/Android. Preserve filesystem versus binary abstract Unix socket addressing explicitly. */
#include "linux_ssip.hpp" /* Import the native sockaddr_un layout privately. */
#if defined(__linux__) /* Android uses the same Linux abstract socket namespace. */
#include <stddef.h> /* Compute the actual native path-field offset portably. */
#include <string.h> /* Copy exact name bytes without treating abstract names as strings. */
/* Build native address bytes and the exact length required by connect/bind semantics. */
FVM_Result fvm_ssip_address(const FVM_SpeechUnixEndpoint *endpoint, struct sockaddr_un *address, /* Abstract input excludes the leading namespace NUL. */
                            socklen_t *length) /* Abstract input excludes the leading namespace NUL. */
{ /* Begin binary-safe native address construction. */
    if (!address || !length) /* Require native result storage. */
        return FVM_INVALID_ARGUMENT; /* Require native result storage. */
    memset(address, 0, sizeof(*address)); /* Clear deterministic unused native address bytes. */
    *length = 0; /* Publish no stale length after validation failure. */
    if (!endpoint || endpoint->struct_size != sizeof(*endpoint) || !endpoint->name || !endpoint->length || /* Reserve one namespace/terminator byte and reject empty names. */
        endpoint->length >= sizeof(address->sun_path)) /* Reserve one namespace/terminator byte and reject empty names. */
        return FVM_INVALID_ARGUMENT; /* Reserve one namespace/terminator byte and reject empty names. */
    address->sun_family = AF_UNIX; /* Select the actual target's local socket address family. */
    if (endpoint->name_space == /* Preserve every abstract name byte, including interior/trailing NULs. */
        FVM_SPEECH_UNIX_ABSTRACT) /* Preserve every abstract name byte, including interior/trailing NULs. */
    { /* Insert the namespace marker separately from the caller's binary name. */
        address->sun_path[0] = 0; /* Linux/Android identify abstract addressing by this initial NUL. */
        memcpy(address->sun_path + 1, endpoint->name, /* Copy exact name bytes without strlen or a synthetic terminator. */
               endpoint->length); /* Copy exact name bytes without strlen or a synthetic terminator. */
    } /* Finish binary abstract addressing. */
    else if (endpoint->name_space == /* Validate a real pathname rather than reinterpreting binary bytes. */
             FVM_SPEECH_UNIX_FILESYSTEM) /* Validate a real pathname rather than reinterpreting binary bytes. */
    { /* Require all supplied pathname bytes to precede its terminator. */
        if (memchr(endpoint->name, 0, endpoint->length)) /* Reject embedded NUL truncation in filesystem names. */
            return FVM_INVALID_ARGUMENT; /* Reject embedded NUL truncation in filesystem names. */
        memcpy(address->sun_path, endpoint->name, endpoint->length); /* Copy the complete explicit pathname. */
        address->sun_path[endpoint->length] = 0; /* Add the filesystem pathname terminator explicitly. */
    } /* Finish filesystem pathname addressing. */
    else /* Never guess namespace semantics from the first caller byte. */
        return FVM_INVALID_ARGUMENT; /* Never guess namespace semantics from the first caller byte. */
    *length = /* Include either the abstract marker or filesystem terminator, without trailing structure padding. */
        (socklen_t)(offsetof(struct sockaddr_un, sun_path) + endpoint->length + /* Include either the abstract marker or filesystem terminator, without trailing structure padding. */
                    1); /* Include either the abstract marker or filesystem terminator, without trailing structure padding. */
    return FVM_OK; /* Return exact binary address bytes ready for native connect. */
} /* Finish binary-safe native address construction. */
#endif /* Finish Linux/Android implementation. */
