/* Copyright (c) 2026 bhgv
 * EN: Native typed handoff/context ownership, constructor rollback and parent/child contract lifetime.
 * RU: Native typed handoff/context ownership, rollback конструктора и lifetime контракта parent/child.
 */
#include "../backend.h"
#include "../../../../runtime/vfs/launch/context.h"
static unsigned position; /* Actual malloc positions inside the selected context constructor. */
static unsigned failure; /* One-based exact refusal, zero allows all allocations. */
static int watching; /* Main-thread-only native observation interval. */
static void *live[4]; /* Context allocation ledger, no observer allocation or VM storage. */


/* EN/RU: Original allocator for size; owned result/NULL without observer recursion.
 * Исходный allocator для size; свой result/NULL, без observer recursion.
 */
extern void *__real_malloc(size_t size);


/* EN/RU: Original native free consumes pointer/NULL without observer recursion.
 * Исходный free поглощает pointer/NULL, без observer recursion.
 */
extern void __real_free(void *pointer);


/* EN/RU: Refuse the selected malloc, otherwise track real native allocation in a fixed ledger; ownership unchanged.
 * Отказать selected malloc, иначе учесть real native allocation в fixed ledger; ownership не меняется.
 */
void *__wrap_malloc(size_t size)
{
    if (watching && ++position == failure)
    {
        return NULL;
    }
    void *pointer = __real_malloc(size);
    if (watching && pointer)
    {
        for (unsigned i = 0; i < 4; i++)
        {
            if (!live[i])
            {
                live[i] = pointer;
                return pointer;
            }
        }
        launch_check(0, "context contract allocation ledger overflow");
    }
    return pointer;
}


/* EN/RU: Observe native context/contract retirement even after constructor tracking ends; consume pointer once.
 * Учесть retirement native context/contract после constructor tracking; поглотить pointer один раз.
 */
void __wrap_free(void *pointer)
{
    for (unsigned i = 0; i < 4; i++)
    {
        if (pointer && live[i] == pointer)
        {
            live[i] = NULL;
        }
    }
    __real_free(pointer);
}


/* EN/RU: Require every observed native allocation freed; no input, explicit fixture failure instead of assert.
 * Требовать освобождения всех observed native allocations; без input, явный отказ вместо assert.
 */
static void drained(void)
{
    for (unsigned i = 0; i < 4; i++)
    {
        launch_check(!live[i], "context contract native owner leaked");
    }
}


/* EN/RU: Real request/take retains copied contract after client/request retirement; constructor refusal must not consume handoff.
 * Настоящие request/take удерживают copied contract после retirement client/request; отказ constructor не поглощает handoff.
 */
int main(void)
{
    LaunchState terminal = {.terminal = 1};
    LaunchState provider = {0};
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    DVM_LaunchSymbol symbol = {"value", 0x80000000UL};
    DVM_LaunchContract *exports = dvm_launch_contract_create(&symbol, 1);
    DVM_LaunchSpec spec = {0};
    spec.policy = options.policy;
    spec.terminal = options.terminal;
    spec.terminal_origin = options.terminal_origin;
    strcpy(spec.expected_provider, options.provider_origin.system);
    strcpy(spec.module, "/app.dis");
    spec.contract = exports;
    spec.arguments = "data";
    spec.argument_bytes = 4;
    DVM_LaunchRequest *request = NULL;
    DVM_LaunchTicket *ticket = dvm_launch_request_create(&spec, &request);
    launch_check(exports && ticket && dvm_launch_request_check(request, exports) &&
        dvm_launch_request_prepare(request, options.provider, &options.provider_origin), "typed context request preparation");
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request);
    dvm_launch_ticket_free(ticket);
    dvm_launch_request_free(request);
    launch_check(handoff && handoff->contract && dvm_launch_contract_matches(handoff->contract, exports),
        "take lost accepted interface requirements");
    DVM_LaunchHandoff invalid = {0};
    strcpy(invalid.description.module, "/app.dis");
    invalid.description.typed = 1;
    launch_check(!dvm_launch_context_create(&invalid), "typed context accepted missing contract");
    invalid.contract = exports;
    invalid.description.typed = 0;
    launch_check(!dvm_launch_context_create(&invalid), "untyped context accepted undeclared contract");
    invalid.description.typed = 2;
    launch_check(!dvm_launch_context_create(&invalid), "context accepted invalid typed marker");
    for (failure = 1; failure <= 2; failure++)
    {
        position = 0;
        watching = 1;
        DVM_LaunchContext *context = dvm_launch_context_create(handoff);
        watching = 0;
        launch_check(!context && dvm_launch_contract_matches(handoff->contract, exports),
            "context allocation refusal consumed handoff requirements");
        drained();
    }
    failure = 0;
    position = 0;
    watching = 1;
    DVM_LaunchContext *context = dvm_launch_context_create(handoff);
    watching = 0;
    launch_check(context && position == 2 && context->contract != handoff->contract &&
        dvm_launch_contract_matches(context->contract, exports) && !memcmp(context->arguments, "data", 4),
        "process context did not independently copy required interface");
    dvm_launch_handoff_free(handoff);
    launch_check(dvm_launch_context_retain(context), "child context retain");
    dvm_launch_context_release(context); /* Parent retires before child. */
    launch_check(dvm_launch_contract_matches(context->contract, exports), "parent retirement lost child requirements");
    dvm_launch_context_release(context);
    drained();
    dvm_launch_contract_free(exports);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "typed context native grants leaked");
    puts("PASS typed handoff/context copy, allocation rollback and child contract lifetime");
    return 0;
}
