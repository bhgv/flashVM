/* Copyright (c) 2026 bhgv
 * EN: Mapped payload, owner release and native buffer wait probes.
 * RU: Проверки mapped-данных, снятия владельца и ожидания буфера.
 * Included once by test_record_new.c in declaration order.
 */

/* Execute the real channel mover with the fixture's decoded operands. */
static int mapped_channel_move(void *unused)
{
    (void)unused;
    movtmp();
    return 1;
}


/* The guarded mover refuses an invalid mapped source before changing output. */
static void mapped_channel_move_case(void)
{
    struct {
        void *pointer;
        WORD scalar;
    } source = {NULL, 17}, destination_value = {H, 29};
    REG *registers = dvm_native_registers();
    REG saved = *registers;
    registers->s = &source;
    registers->d = &destination_value;
    registers->m = &record;
    check(!kprotect(mapped_channel_move, NULL) &&
        destination_value.pointer == H && destination_value.scalar == 29,
        "mapped channel mover refusal leaves destination intact");
    source.pointer = H;
    check(kprotect(mapped_channel_move, NULL) &&
        destination_value.pointer == H && destination_value.scalar == 17,
        "mapped channel mover copies validated fields");
    *registers = saved;
    puts("PASS guarded mapped channel mover");
}


/* A mapped send must leave the empty ring unchanged when ownership fails. */
static void mapped_channel_send_case(void)
{
    struct {
        void *pointer;
        WORD scalar;
    } source = {NULL, 73};
    REG *registers = dvm_native_registers();
    REG saved = *registers;
    Channel *channel = H;
    int receive = 0;
    void *pointer;
    WORD scalar;
    channel = dvm_runtime_channel_new(&channel, record, 1, movtmp, 0);
    registers->s = &source;
    registers->d = &channel;
    check(!kprotect(direct_channel_transfer, &receive) && channel->size == 0,
        "refused mapped send does not publish occupancy");
    memmove(&pointer, channel->buf->data, sizeof(pointer));
    check(pointer == H, "refused mapped send preserves empty slot");
    source.pointer = H;
    registers->s = &source;
    registers->d = &channel;
    check(kprotect(direct_channel_transfer, &receive) && channel->size == 1,
        "successful mapped send publishes occupancy");
    memmove(&scalar, channel->buf->data + sizeof(pointer), sizeof(scalar));
    check(scalar == 73, "published mapped slot contains the complete payload");
    *registers = saved;
    destroy(channel);
    puts("PASS mapped channel send publication");
}


/* Check both sides of the ownership commit through the actual SEND/RECV. */
static void mapped_channel_receive_case(void)
{
    struct {
        void *pointer;
        WORD scalar;
    } source = {H, 83}, output = {NULL, 31};
    REG *registers = dvm_native_registers();
    REG saved = *registers;
    Channel *channel = H;
    int receive = 0;
    void *incoming;
    void *slot;
    channel = dvm_runtime_channel_new(&channel, record, 1, movtmp, 0);
    dvm_runtime_record_new(&source.pointer, record, 1);
    incoming = source.pointer;
    registers->s = &source;
    registers->d = &channel;
    check(kprotect(direct_channel_transfer, &receive) && channel->size == 1,
        "seed mapped ring with an owned reference");
    destroy(source.pointer);
    source.pointer = H;
    receive = 1;
    registers->s = &channel;
    registers->d = &output;
    check(!kprotect(direct_channel_transfer, &receive) && channel->size == 1 &&
        !output.pointer && output.scalar == 31, "failed mapped read preserves occupancy");
    memmove(&slot, channel->buf->data, sizeof(slot));
    check(slot == incoming, "failed mapped read retains source ownership");
    output.pointer = H;
    dvm_runtime_record_new(&output.pointer, record, 1);
    refill_failure_refs = 1;
    refill_release_failure = output.pointer;
    registers->s = &channel;
    registers->d = &output;
    check(!kprotect(direct_channel_transfer, &receive) && !refill_release_failure &&
        strstr(getup()->env->errstr, "fixture channel release failure") &&
        channel->size == 0 && output.pointer == incoming && output.scalar == 83,
        "committed mapped read publishes despite displaced-owner error");
    memmove(&slot, channel->buf->data, sizeof(slot));
    check(slot == H, "committed mapped read clears the ring owner");
    *registers = saved;
    destroy(output.pointer);
    destroy(channel);
    puts("PASS mapped channel receive ownership");
}


static Channel **buffer_owner_slot; /* Test application owner. */


/* Drop the application's only channel reference from a real buffered mover. */
static void drop_buffer_owner(void)
{
    Channel *channel = *buffer_owner_slot;
    REG *registers = dvm_native_registers();
    check(dvm_runtime_root_count(D2H(channel)) == 1, "buffer mover has a rooted channel");
    *buffer_owner_slot = H;
    destroy(channel);
    check(dvm_runtime_root_count(D2H(channel)) == 1 && channel->size == 0,
        "buffer reservation survives loss of application owner");
    memmove(registers->d, registers->s, sizeof(WORD));
}


/* The reservation keeps the ring alive until metadata has been published. */
static void buffer_owner_case(void)
{
    REG *registers = dvm_native_registers();
    REG saved = *registers;
    Channel *channel = H;
    WORD value = 53;
    int receive = 0;
    channel = dvm_runtime_channel_new(&channel, &Tword, 1, drop_buffer_owner, 0);
    buffer_owner_slot = &channel;
    registers->s = &value;
    registers->d = &channel;
    check(kprotect(direct_channel_transfer, &receive) && channel == H,
        "send completes after application owner disappears");
    buffer_owner_slot = NULL;
    *registers = saved;
    puts("PASS rooted channel buffer lifetime");
}


typedef struct BufferWaitProbe
{
    Channel *channel; /* The test's application owner outlives the worker. */
    pthread_t thread;
    pthread_barrier_t parked;
    int result;
    int fail;
} BufferWaitProbe;

static BufferWaitProbe *buffer_wait_probe; /* Bound mover only. */
static __thread BufferWaitProbe *buffer_wait_thread; /* Native waiter only. */

void __real_fvm_port_sync_wait(FVM_PortSync *sync);


/* Prove the native waiter reaches the condition wait before the mover exits. */
void __wrap_fvm_port_sync_wait(FVM_PortSync *sync)
{
    BufferWaitProbe *probe = buffer_wait_thread;
    if (probe)
    {
        buffer_wait_thread = NULL;
        pthread_barrier_wait(&probe->parked);
    }
    __real_fvm_port_sync_wait(sync);
}


static void *wait_buffer_idle(void *argument)
{
    BufferWaitProbe *probe = argument;
    extern int dvm_runtime_channel_wait_idle(Channel *);
    buffer_wait_thread = probe;
    probe->result = dvm_runtime_channel_wait_idle(probe->channel);
    return NULL;
}


/* Park an unbound native thread during the reservation, then exit either way. */
static void wait_buffer_mover(void)
{
    BufferWaitProbe *probe = buffer_wait_probe;
    REG *registers = dvm_native_registers();
    extern int dvm_runtime_channel_wait_idle(Channel *);
    check(!dvm_runtime_channel_wait_idle(probe->channel), "bound owner cannot park");
    check(!pthread_create(&probe->thread, NULL, wait_buffer_idle, probe),
        "start native buffer waiter");
    pthread_barrier_wait(&probe->parked);
    if (probe->fail)
    {
        error("fixture buffered wait mover failure");
    }
    memmove(registers->d, registers->s, sizeof(WORD));
}


static void buffer_wait_case(void)
{
    REG *registers = dvm_native_registers();
    REG saved = *registers;
    int receive = 0;
    int fail;
    for (fail = 0; fail <= 1; fail++)
    {
        BufferWaitProbe probe = {0};
        WORD value = 67;
        probe.channel = H;
        probe.fail = fail;
        check(!pthread_barrier_init(&probe.parked, NULL, 2), "init buffer wait barrier");
        dvm_runtime_channel_new(&probe.channel, &Tword, 1, wait_buffer_mover, 0);
        buffer_wait_probe = &probe;
        registers->s = &value;
        registers->d = &probe.channel;
        check(kprotect(direct_channel_transfer, &receive) == !fail, "buffer mover outcome");
        check(!pthread_join(probe.thread, NULL) && probe.result &&
            probe.channel->size == !fail, "native waiter wakes after reservation exits");
        pthread_barrier_destroy(&probe.parked);
        buffer_wait_probe = NULL;
        destroy(probe.channel);
    }
    *registers = saved;
    puts("PASS native channel buffer wait");
}


