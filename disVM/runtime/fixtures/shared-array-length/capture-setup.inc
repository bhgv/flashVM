/* Copyright (c) 2026 bhgv
 * EN: Capture retries and receive-error boundary injection.
 * RU: Повторы захвата и инъекции ошибок границы receive.
 * Extracted private oracle fragment; assembled only by its Python driver.
 */

#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "isa.h"
#include "interp.h"
#include "disvm/native_process.h"
#include "raise.h"
#include "shared_array_length.h"
#include "array_length_value.h"
#include "managed_owner.h"
#include "root_registry.h"
#include "runtime_gc.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern void lena(void); /* Actual native opcode, not a modeled operation. */
static __thread int active; /* Only the serialized fixture controller observes helpers. */
static __thread unsigned long allocations; /* Native allocation/free calls inside LENA. */
static __thread unsigned int expected_depth; /* Independent six-key union size. */
static DVM_GCWorkers *array_collectors; /* Borrowed collector service until the fixture exits. */
static __thread int address_publication; /* Only this owner's private-to-shared publication call. */
const char *__real_dvm_runtime_bytes_publish_opcode(int, const void *, void *, size_t);
DVM_CellResult __real_dvm_managed_ranges_begin(const void *, size_t, void *, size_t, DVM_RangeAccess *);


char *__real_dvm_runtime_string_view(String *, const char **);

/* Make the second CVTCL view fail after it has reserved its destination.
 * The guest catches the exact error and converts again on the same frame. */
char *__wrap_dvm_runtime_string_view(String *string, const char **problem)
{
    static unsigned int calls; /* CVTCL calls that reached view preparation. */
    if (CONTROL == 0 && dvm_native_execution()->arithmetic_retry_opcode == ICVTCL && ++calls == 2)
    {
        *problem = "test UTF-8 view refusal";
        fprint(1, "PASS injected string view refusal\n");
        return NULL;
    }
    return __real_dvm_runtime_string_view(string, problem);
}


/* EN: In the reference capture profile, refuse stage once and verify its replay PC.
 * stage indexes the 50 independent capture sites. Return one to inject BUSY,
 * zero after replay or in the separate native commit-error workload. A wrong PC
 * exits 10. The tables belong only to the single-threaded reference profile;
 * native owners must not share them. No managed ownership is acquired.
 * RU: В reference-профиле один раз отказать stage и сверить PC повтора.
 * stage — индекс одного из 50 мест. Единица инъецирует BUSY, ноль — после повтора
 * либо в отдельной native-нагрузке ошибок commit. Неверный PC завершает с 10.
 * Таблицы принадлежат лишь однопоточному reference-профилю; native-владельцы
 * не разделяют их. Владение managed-объектами не захватывается. */
static int contend_capture(int stage)
{
    static Inst *pcs[50]; /* Post-decode PC for each refused capture. */
    static int replayed[50]; /* One successful replay for each stage. */

    if (getenv("DVM_TEST_RECEIVE_ERRORS") || replayed[stage])
    {
        return 0;
    }
    if (!pcs[stage])
    {
        pcs[stage] = DVM_R.PC;
        return 1;
    }
    if (pcs[stage] != DVM_R.PC)
    {
        fprint(2, "FAIL guarded capture replay changed PC\n");
        exit(10);
    }
    replayed[stage] = 1;
    fprint(1, "PASS guarded capture replay %d\n", stage);
    return 0;
}


static int receive_fault; /* Prepublication then postpublication. */
static __thread int release_fault; /* Fail once after dropping the captured array owner. */
void __real_destroy(void *);

void __wrap_destroy(void *value)
{
    if (release_fault && value != H && D2H(value)->t == &Tarray)
    {
        release_fault = 0;
        __real_destroy(value);
        fprint(1, "PASS injected receive array release failure\n");
        error("receive array release failed");
    }
    __real_destroy(value);
}

static pthread_mutex_t sender_gate = PTHREAD_MUTEX_INITIALIZER; /* Protect queued-send evidence. */
static pthread_cond_t sender_ready = PTHREAD_COND_INITIALIZER; /* Notify actual waiter publication. */
static unsigned int queued_senders; /* Published SEND waits in the receive-error guest. */
static unsigned int required_senders; /* Parent's completed test handshakes. */
static void check(int valid, const char *message);
void __real_dvm_runtime_channel_waiter_published(void);
int __real_dvm_runtime_guest_sleep(unsigned long milliseconds);


/* EN: Observe committed SEND waiting state after normal waiter publication.
 * RU: Зафиксировать опубликованное ожидание SEND после штатной публикации. */
void __wrap_dvm_runtime_channel_waiter_published(void)
{
    __real_dvm_runtime_channel_waiter_published();
    if (getenv("DVM_TEST_RECEIVE_ERRORS") && up->prog && ((Prog *)up->prog)->state == Psend)
    {
        pthread_mutex_lock(&sender_gate);
        queued_senders++;
        pthread_cond_broadcast(&sender_ready);
        pthread_mutex_unlock(&sender_gate);
    }
}


/* EN: The guest's sentinel sleep waits outside heap admission for its queued sender.
 * milliseconds otherwise passes to the real sleep unchanged. Return one when the handshake handled the sleep.
 * RU: Тестовое ожидание подтверждает очередь отправителя вне допуска к куче.
 * Иные milliseconds передаются штатному ожиданию. При подтверждении вернуть единицу. */
int __wrap_dvm_runtime_guest_sleep(unsigned long milliseconds)
{
    if (milliseconds != 42421 || !getenv("DVM_TEST_RECEIVE_ERRORS"))
    {
        return __real_dvm_runtime_guest_sleep(milliseconds);
    }
    int released = dvm_runtime_gc_native_try_leave();
    check(released == 1, "queued sender handshake releases heap participation");
    pthread_mutex_lock(&sender_gate);
    required_senders++;
    while (queued_senders < required_senders)
    {
        pthread_cond_wait(&sender_ready, &sender_gate);
    }
    pthread_mutex_unlock(&sender_gate);
    dvm_runtime_gc_native_enter(released);
    return 1;
}


static __thread int ring_kind; /* 1 mapped, 2 native scalar, 3 raw record. */
int __real_dvm_runtime_mapped_buffer_transfer(const void *, void *, Type *, int, int *, int, const Prog *);
DVM_CellResult __real_dvm_managed_spans_begin(const void *, size_t, void *, size_t, DVM_CellAccess *);

/* Identify the exact transfer layout before injecting at its range claim. */
int __wrap_dvm_runtime_mapped_buffer_transfer(const void *source, void *destination,
    Type *type, int consume, int *committed, int opcode, const Prog *sender)
{
    int result;
    int inject = getenv("DVM_TEST_RECEIVE_ERRORS") != NULL;
    int fault = inject && consume && opcode == IRECV;
    if (fault && receive_fault == 0)
    {
        receive_fault = 1;
        release_fault = 1;
        error("receive failed before publication");
    }
    if (inject && opcode == 0 && !consume && receive_fault == 4)
    {
        receive_fault = 5;
        error("refill failed before publication");
    }
    ring_kind = type->np ? 1 : (type->size <= sizeof(void *) ? 2 : 3);
    result = __real_dvm_runtime_mapped_buffer_transfer(source, destination, type, consume, committed, opcode, sender);
    ring_kind = 0;
    if (fault && receive_fault == 1 && *committed)
    {
        receive_fault = 2;
        release_fault = 1;
        error("receive failed after publication");
    }
    if (inject && opcode == ISEND && receive_fault == 2 && *committed)
    {
        receive_fault = 3;
        error("send failed after publication");
    }
    if (inject && opcode == 0 && !consume && receive_fault == 3 && *committed)
    {
        receive_fault = 4;
        error("refill failed after publication");
    }
    return result;
}


/* Inject BUSY into the first guarded WORD send and receive. */
DVM_CellResult __wrap_dvm_managed_spans_begin(const void *source, size_t source_size,
    void *destination, size_t destination_size, DVM_CellAccess *access)
{
    int opcode = dvm_native_execution()->arithmetic_retry_opcode;
    if (CONTROL == 0 && ring_kind == 2 && source_size == sizeof(WORD) &&
        ((opcode == ISEND && contend_capture(44)) || (opcode == IRECV && contend_capture(45))))
    {
        return DVM_CELL_BUSY;
    }
    return __real_dvm_managed_spans_begin(source, source_size, destination, destination_size, access);
}


DVM_CellResult __real_dvm_managed_three_begin(const void *, size_t, const void *, size_t,
    void *, size_t, DVM_RangeAccess *);

/* Refuse once at the existing-owner payload/header/slot claim of each codec. */
DVM_CellResult __wrap_dvm_managed_three_begin(const void *payload, size_t payload_size,
    const void *header, size_t header_size, void *slot, size_t slot_size, DVM_RangeAccess *access)
{
    int opcode = dvm_native_execution()->arithmetic_retry_opcode;
    if (CONTROL == 0 && !active &&
        ((opcode == ICVTCA && contend_capture(19)) || (opcode == ICVTAC && contend_capture(20))))
    {
        return DVM_CELL_BUSY;
    }
    return __real_dvm_managed_three_begin(payload, payload_size, header, header_size, slot, slot_size, access);
}


/* Observe source/destination geometry inside the actual guard acquisition. */
