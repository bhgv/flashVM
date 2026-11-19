# Copyright (c) 2026 bhgv
# EN: Small runnable Coco/R syntax-service example, distinct from quota stress tests.
# RU: Короткий пример сервиса Coco/R, отдельный от стресс-проверок квот.
implement SyntaxExample;
include "sys.m";
include "idesyntax.m";

SyntaxExample: module {
    init: fn(); # Console entry; generated scanners run in a separate native Dis owner.
};


# EN: Start the real worker, scan two small Unicode inputs and close by reply.
# No arguments. Jobs/results remain rooted through each rendezvous; errors raise.
# No GUI or timing assumptions, no shell/subprocess implementation of scanning.
# RU: Запустить настоящий worker, разобрать два Unicode-текста и завершить с ответом.
# Аргументов нет. Каналы и значения удерживаются до ответов; ошибки дают исключение.
# Нет GUI, предположений о времени или shell-подмены сканера.
init()
{
    sys := load Sys Sys->PATH;
    syntax := load Idesyntax Idesyntax->PATH;
    if (sys == nil || syntax == nil) {
        raise "fail: syntax example dependencies";
    }
    (jobs, answers) := syntax->start();
    if (jobs == nil || answers == nil) {
        raise "fail: syntax example scanners";
    }
    source := "fn α😀() { # comment\n return 42; }";
    jobs <-= syntax->Request(1, 1, syntax->Limbo, source, 0);
    result := <-answers;
    palette := syntax->project(result, 1, syntax->Limbo, 0, len source);
    valid := result.status == syntax->Ok && len palette == len source;
    valid = valid && palette[0:2] == "44";
    valid = valid && syntax->project(result, 2, syntax->Limbo, 0, len source) == "";
    source = "var π😀 = \"text\"; // comment";
    jobs <-= syntax->Request(2, 2, syntax->ActionScript, source, 0);
    result = <-answers;
    palette = syntax->project(result, 2, syntax->ActionScript, 0, len source);
    valid = valid && result.status == syntax->Ok && len palette == len source;
    valid = valid && palette[0:3] == "444";
    jobs <-= syntax->Request(3, 3, syntax->Limbo, "", 1);
    stopped := <-answers;
    if (!valid || stopped.status != syntax->Ok || stopped.ticket != 3) {
        raise "fail: syntax example result or shutdown";
    }
    sys->print("PASS Coco/R syntax example: profiles=2 Unicode=ok stale=reject shutdown=ok\n");
}
