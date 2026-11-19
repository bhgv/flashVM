/* Copyright (c) 2026 bhgv
 * EN: Deterministic singleton publication race, constructor rollback and opened-factory lifetime.
 * RU: Воспроизводимая гонка публикации устройства, откат конструктора и время жизни открытой factory.
 */
#include "sysevents_internal.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define DEVICE_CREATORS 8U /* Independent native constructors paused after actual factory allocation. */
static pthread_barrier_t admission; /* Test-owned gate; initialized before workers, destroyed after join. */
static int competing; /* Immutable while constructor workers execute; zero in all other fixtures. */
static unsigned candidates; /* Atomic count of real, fully allocated factories entering the gate. */
static unsigned destroyed; /* Atomic count of actual candidate destructor calls, including the final winner. */
static pthread_mutex_t probe_lock = PTHREAD_MUTEX_INITIALIZER; /* Protects first installation of the test-only destructor observer. */
static const DVM_ProviderOps *original_ops; /* Borrowed static production table, valid through all joined candidates. */
static DVM_ProviderOps observed_ops; /* Immutable candidate table once workers leave the probe mutex. */


/* EN: Observe actual destruction, then delegate with the original state and ownership contract unchanged.
 * RU: Посчитать настоящее уничтожение и вызвать исходный деструктор с неизменённым владением state.
 */
static void destroy_candidate(void *state)
{
    original_ops->destroy(state);
    __atomic_add_fetch(&destroyed, 1, __ATOMIC_RELAXED);
}


/* EN/RU: Configure the shared fixture's real one-shot calloc refusal; call only without concurrent workers.
 * Настроить настоящий однократный отказ calloc общего теста; вызывать только без конкурирующих потоков.
 */
extern void test_sysevents_calloc_failure(size_t size, unsigned match);


/* EN/RU: Read the fixture's actual rejection count after synchronous allocation attempts.
 * Прочитать число фактических отказов после синхронных попыток выделения памяти.
 */
extern unsigned test_sysevents_calloc_failures(void);


/* EN/RU: Delegate ordinary factory construction; ops/state keep their original transfer contract.
 * Передать обычное создание factory; исходный контракт передачи ops/state сохраняется.
 */
extern DVM_Provider *__real_dvm_provider_create(const DVM_ProviderOps *ops, void *state);


/* EN: Abort on a failed native contract; borrowed label, no conditional assertions.
 * RU: Завершить тест при нарушении контракта; label заимствован, условных assert нет.
 */
static void require(int valid, const char *label)
{
    if (!valid)
    {
        fprintf(stderr, "sysevents device: %s\n", label);
        abort();
    }
}


/* EN: Wait for all actual constructors; no production mutex or guest execution participates.
 * RU: Дождаться всех настоящих конструкторов; без production mutex и исполнения гостя.
 */
static void rendezvous(void)
{
    int status = pthread_barrier_wait(&admission);
    require(!status || status == PTHREAD_BARRIER_SERIAL_THREAD, "constructor barrier");
}


/* EN: Return the real factory, pausing competing workers before the device constructor can publish it.
 * ops/state are borrowed until real construction accepts ownership; failures remain real failures.
 * RU: Вернуть настоящую factory, задержав конкурентов до публикации устройства.
 * ops/state заимствуются до принятия исходным конструктором; ошибки не подменяются.
 */
DVM_Provider *__wrap_dvm_provider_create(const DVM_ProviderOps *ops, void *state)
{
    const DVM_ProviderOps *selected = ops;
    if (competing)
    {
        require(!pthread_mutex_lock(&probe_lock), "probe lock");
        if (!original_ops)
        {
            original_ops = ops;
            observed_ops = *ops;
            observed_ops.destroy = destroy_candidate;
        }
        require(original_ops == ops, "only device constructors enter the gate");
        selected = &observed_ops;
        require(!pthread_mutex_unlock(&probe_lock), "probe unlock");
    }
    DVM_Provider *provider = __real_dvm_provider_create(selected, state);
    if (competing)
    {
        require(provider != NULL, "real candidate allocation");
        __atomic_add_fetch(&candidates, 1, __ATOMIC_RELAXED);
        rendezvous();
    }
    return provider;
}


/* EN: Store one real constructor result into the caller-owned slot; caller joins before reading.
 * RU: Записать результат конструктора в слот вызывающего; читать слот можно после join.
 */
static void *construct(void *slot)
{
    *(DVM_Provider **)slot = dvm_sysevents_device();
    return NULL;
}


/* EN: Retain the winner through an actual opened handle, then prove final close permits recreation.
 * Consumes provider; no device reference is held by the test after initial release.
 * RU: Удержать победителя настоящим открытым FD; проверить повторное создание после закрытия.
 * Поглощает provider; после начального release тест не держит других ссылок на устройство.
 */
static void retained_handle(DVM_Provider *provider)
{
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
    DVM_SysEvents *events = group ? dvm_sysevents_create(group) : NULL;
    DVM_VfsHandle handle = {0};
    require(events != NULL, "handle inbox");
    dvm_sysevents_bind(events);
    require(dvm_provider_open(provider, ".", "r", &handle) == FVM_OK, "actual contextual open");
    dvm_sysevents_bind(NULL);
    dvm_provider_release(provider);
    require(dvm_sysevents_device() == NULL, "opened factory lost singleton reservation");
    dvm_sysevents_retire(events);
    dvm_sysevents_release(events);
    dvm_vfs_close(&handle);
    dvm_namespace_release(group);
    DVM_Provider *replacement = dvm_sysevents_device();
    require(replacement && dvm_sysevents_is_device(replacement), "recreation after final handle close");
    dvm_provider_release(replacement);
}


/* EN: Exercise eight concurrent constructors repeatedly; exactly one owned result must remain recognizable.
 * Each round joins all workers before release; unpublished losers must not erase the winner's identity.
 * RU: Несколько раз запустить восемь конкурирующих конструкторов; ровно один результат должен распознаваться.
 * Перед освобождением дождаться всех потоков; проигравшие не должны сбросить идентичность победителя.
 */
void test_sysevents_device(void)
{
    for (unsigned failure = 1; failure <= 2; failure++)
    {
        unsigned before = test_sysevents_calloc_failures();
        test_sysevents_calloc_failure(0, failure);
        require(dvm_sysevents_device() == NULL, "state/factory allocation refusal");
        require(test_sysevents_calloc_failures() == before + 1, "actual constructor allocation refusal");
        test_sysevents_calloc_failure(0, 0);
        DVM_Provider *retry = dvm_sysevents_device();
        require(retry && dvm_sysevents_is_device(retry), "recovery after allocation refusal");
        dvm_provider_release(retry);
    }
    for (unsigned round = 0; round < 4; round++)
    {
        pthread_t workers[DEVICE_CREATORS];
        DVM_Provider *results[DEVICE_CREATORS] = {0};
        DVM_Provider *winner = NULL;
        require(!pthread_barrier_init(&admission, NULL, DEVICE_CREATORS), "barrier initialization");
        candidates = 0;
        destroyed = 0;
        original_ops = NULL;
        competing = 1;
        for (unsigned index = 0; index < DEVICE_CREATORS; index++)
        {
            require(!pthread_create(&workers[index], NULL, construct, &results[index]), "constructor thread");
        }
        for (unsigned index = 0; index < DEVICE_CREATORS; index++)
        {
            require(!pthread_join(workers[index], NULL), "constructor join");
            if (results[index])
            {
                require(!winner, "multiple published device factories");
                winner = results[index];
            }
        }
        competing = 0;
        require(!pthread_barrier_destroy(&admission), "barrier destruction");
        require(candidates == DEVICE_CREATORS && winner && dvm_sysevents_is_device(winner),
            "winner identity after losing candidate destruction");
        require(destroyed == DEVICE_CREATORS - 1, "all unpublished candidates actually destroyed");
        require(dvm_sysevents_device() == NULL, "duplicate live device");
        retained_handle(winner);
        require(destroyed == DEVICE_CREATORS, "winner actually destroyed on final close");
    }
    puts("PASS sysevents device: state/factory refusal, 32 real racing candidates and opened-handle lifetime");
}
