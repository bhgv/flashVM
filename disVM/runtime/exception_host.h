/* Copyright (c) 2026 bhgv
 * EN: Exception lookup, emergency ownership and collector retirement contracts.
 * RU: Поиск обработчиков, резервное владение и освобождение сборщиком.
 */
#ifndef DVM_EXCEPTION_HOST_H
#define DVM_EXCEPTION_HOST_H


/* Copy the current bound owner's module path and predecessor PC for diagnostics.
 * registers must be its active REG; name is borrowed writable capacity bytes.
 * No heap allocation, ownership transfer or payload lock acquisition occurs. */
WORD dvm_exception_status(REG *registers, char *name, int capacity);
struct Prog;


/* Start an audited read-only handler lookup. */
int dvm_runtime_exception_search_enter(struct Prog *process);


/* Pop an ordinary frame through the shared retirement protocol. */
int dvm_runtime_exception_pop(struct Prog *process, const unsigned char *handler_frame);


/* Publish an already-owned exception and defer displaced handler slots. */
int dvm_runtime_exception_install(Prog *process, Type *cleanup, void *slot, Inst *target);


/* Transfer validated pending argument frames and return the remaining suffix. */
unsigned char *dvm_runtime_exception_pending(struct Prog *process, unsigned char *last);


/* Replace the typed process root with its message while shared. */
int dvm_runtime_exception_convert(struct Prog *process);


/* Prepare a missing runtime exception message using shared allocation. */
int dvm_runtime_exception_message(struct Prog *process, const char *message);


/* Initialize and retire the bound thread's optional emergency string owner. */
void dvm_runtime_exception_reserve_start(void);


void dvm_runtime_exception_reserve_stop(void);


/* EN: Borrow this thread's reserve, or null. Only for admitted managed use.
 * RU: Получить резерв текущего потока либо null. Нужен допуск к куче. */
String *dvm_runtime_exception_reserve_value(void);


/* EN: Inspect pending reserves, or retire one from the Fast collector.
 * RU: Проверить очередь резервов либо снять один на потоке Fast. */
int dvm_runtime_exception_retirement_ready(void);


void dvm_runtime_exception_retire_one(void);

/* [GC:] EN: On bound Fast context, retire at most budget queued strings after
 * their reader epoch. Return progress/retry without invoking retained tracing.
 * RU: На привязанном context Fast освободить до budget строк после эпохи
 * читателей. Вернуть работу/повтор без вызова резервной трассировки. */
struct DVM_NativeContext;


unsigned int dvm_runtime_exception_retire_shared(struct DVM_NativeContext *context, size_t budget);


/* [GC:] EN: Print counters only after all producer/collector joins if diagnostics.
 * RU: При diagnostics вывести счётчики только после join производителей и GC. */
void dvm_runtime_exception_retirement_report(int diagnostics);

#endif
