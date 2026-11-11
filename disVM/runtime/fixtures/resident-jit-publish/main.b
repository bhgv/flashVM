# Copyright (c) 2026 bhgv
# EN: Compile one shared Module while permanent residents execute its real program.
# RU: Компилировать общий Module при исполнении его программы постоянными потоками.
implement JitPublish;
include "sys.m";
include "loader.m";
include "processctl.m";
include "provider.m";
JitPublish: module { init: fn(); };
sys: Sys; # Immutable builtin loaded before spawn.
loader: Loader; # Real checked native compilation API.
control: Processctl; # Exact child retirement after completions.
Workers: con 8; # Concurrent compiler residents sharing one Module/Modlink.
Rounds: con 64; # Repeated cache validation and execution per compiler.


# EN: Compile shared image and check typed provider execution; endpoints are borrowed.
# id selects private checksums and done carries the copied PID after all work.
# RU: Компилировать общий image и проверять provider; каналы заимствованы.
# id выбирает личные суммы, done несёт скопированный PID после всей работы.
compiler(id: int, image: Nilmod, provider: JitProvider, ready, start, done, published: chan of int)
{
    ready <-= 1;
    <-start;
    for (at := 0; at < Rounds; at++) {
        if (loader->compile(image, 1) != 0 && loader->compile(image, 1) != 0) {
            raise "shared native compile failed";
        }
        if (id == 0 && at == 0) {
            published <-= 1;
        }
        value := id * Rounds + at;
        if (provider->run(value) != value * 3 + 7) {
            raise "shared JIT publication changed compiler continuation";
        }
    }
    done <-= control->pid();
}


# EN: Execute the provider before, during and after concurrent native compilation.
# RU: Исполнять provider до, во время и после параллельной native-компиляции.
reader(provider: JitProvider, ready, start, done, published: chan of int)
{
    ready <-= 1;
    <-start;
    for (at := 0; at < 4096; at++) {
        if (at == 2048) {
            <-published;
        }
        if (provider->run(at) != at * 3 + 7) {
            raise "executing resident observed incomplete native code";
        }
    }
    done <-= control->pid();
}


# EN: Retain shared code/instances until all nine resident owners have retired.
# RU: Удерживать общий код/экземпляры до завершения всех девяти владельцев.
init()
{
    sys = load Sys Sys->PATH;
    loader = load Loader Loader->PATH;
    control = load Processctl Processctl->PATH;
    image := load Nilmod "provider.dis";
    provider := load JitProvider "provider.dis";
    if (image == nil || provider == nil) {
        raise "JIT publication provider did not load";
    }
    ready := chan of int;
    start := chan of int;
    done := chan of int;
    published := chan of int;
    for (id := 0; id < Workers; id++) {
        spawn compiler(id, image, provider, ready, start, done, published);
    }
    spawn reader(provider, ready, start, done, published);
    for (id = 0; id < Workers + 1; id++) {
        <-ready;
    }
    for (id = 0; id < Workers + 1; id++) {
        start <-= 1;
    }
    for (id = 0; id < Workers + 1; id++) {
        result := control->killwait(<-done);
        if (result != Processctl->Retired && result != Processctl->Denied) {
            raise "JIT publication resident did not retire";
        }
    }
    sys->print("PASS resident JIT publication compilers=8 calls=512 reader_calls=4096\n");
}
