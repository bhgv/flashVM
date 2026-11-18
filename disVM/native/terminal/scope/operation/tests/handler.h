/* Copyright (c) 2026 bhgv
 * EN: Native-only fixture handler state; ASCII bodies are test schemas, not production command identifiers.
 * RU: Native-only fixture handler state; ASCII bodies лишь test schemas, не production command identifiers.
 */
#ifndef SCOPE_OPERATION_TEST_HANDLER_H
#define SCOPE_OPERATION_TEST_HANDLER_H
#include "service/service.h"
#include "operation/operation.h"


/* EN/RU: Main-owned state through rootfree; counters accessed atomically across native broker/main owners.
 * Main-owned state до rootfree; counters atomic между broker/main owners.
 */
typedef struct OperationHandler
{
    StyxScope *a;                  /* Native capability selected by host, never decoded from a peer pointer. */
    StyxScope *group;              /* Independent nested native parent with its own operation IDs. */
    StyxScope *b;                  /* Native capability selected by host. */
    struct OperationHandler *next; /* Replacement state, borrowed until final rootfree. */
    unsigned inside;               /* Atomic callback-active test marker. */
    unsigned released;             /* Atomic final-release count. */
    unsigned bad_release;          /* Atomic release-during-callback violation. */
} OperationHandler;


/* EN/RU: Fixture-only ordered root-service parser; host callbacks never wait or execute guest code.
 * Fixture-only ordered root-service parser; callbacks не ждут и не исполняют guest code.
 */
int operationhandler(void *state, StyxScope *root, const void *bytes, size_t size);


/* EN/RU: Final native handler-state release witness; state itself stays owned by main until after rootfree.
 * Final release witness native handler-state, сам state принадлежит main до конца rootfree.
 */
void operationrelease(void *state);
#endif
