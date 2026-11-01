/* Public coordinate/span API; see README.md and frontend/SOURCE-POSITIONS.md. */
#include "source_position.h" /* Declares physical coordinates and half-open spans. */
#include <stdio.h> /* Supplies diagnostic output, with no source-file access. */


/* Resolve one borrowed snapshot. Return0 for exact expected coordinates,1 for
 * refusal or mismatch. No parser, callback, shared state or file is accessed. */
int main(void) /* The example owns the immutable bytes and output storage. */
{ /* Both calls validate the complete snapshot before publishing results. */
    const char source[] = "\t\316\273\360\237\230\200\r\ne\314\201"; /* Tab, lambda, smile, CRLF, e, accent. */
    LimboSourcePosition position; /* Receives the physical EOF position. */
    LimboSourceSpan span; /* Receives both boundaries of the second physical line. */
    if (!limbo_source_position(source,sizeof(source)-1,sizeof(source)-1,&position)) /* Exclude the C NUL. */
    { /* Invalid UTF-8/boundaries leave position untouched. */
        fputs("Invalid source boundary\n",stderr); /* Explain the failed conversion. */
        return 1; /* Do not inspect an unpublished result. */
    } /* EOF is a valid UTF-8/CRLF boundary. */
    if (!limbo_source_span(source,sizeof(source)-1,9,sizeof(source)-1,&span)) /* Resolve both endpoints in one pass. */
    { /* Byte9 follows the complete CRLF pair. */
        fputs("Invalid source span\n",stderr); /* Report an all-or-nothing span refusal. */
        return 1; /* Neither endpoint may be used after refusal. */
    } /* The caller continues owning the original bytes. */
    printf("EOF byte=%lu line=%lu scalar-column=%lu\n", /* Columns count scalars, not bytes/display cells. */
        (unsigned long)position.byte_offset,(unsigned long)position.line, /* Print copied physical coordinates. */
        (unsigned long)position.column); /* The combining accent counts separately. */
    printf("span=%lu..%lu line=%lu columns=%lu..%lu\n", /* The end is exclusive. */
        (unsigned long)span.begin.byte_offset,(unsigned long)span.end.byte_offset, /* Retain original byte anchors. */
        (unsigned long)span.begin.line,(unsigned long)span.begin.column, /* The second line starts at column1. */
        (unsigned long)span.end.column); /* EOF follows its two scalars at column3. */
    return position.line == 2 && position.column == 3 && span.begin.column == 1 && /* Check independent fixture facts. */
        span.end.line == 2 && span.end.column == 3 ? 0 : 1; /* Preserve a useful exit status. */
} /* No allocation or runtime cleanup was required. */
