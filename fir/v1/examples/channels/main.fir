FIR 1.0
// Hand-written FIR tutorial / Учебный FIR, написанный вручную.
module @channels
requires flashvm.channels
func @main() -> void {
^entry:
    // Existing FlashVM extension / Уже существующее расширение FlashVM.
    %ch: channel = limbo.chan_create
    %answer: i32 = const.i32 42
    // Unbounded queue: send before receive does not wait for a consumer.
    // Неограниченная очередь: отправка не ждёт получателя.
    limbo.chan_send %ch, %answer
    %received: any = limbo.chan_receive %ch
    trace %received
    // Close wakes waiters; wrapper lifetime is owned by the VM.
    // Close будит ожидающих; временем жизни обёртки управляет VM.
    limbo.chan_close %ch
    return
}
