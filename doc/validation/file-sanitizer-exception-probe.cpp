/* Reproducer for restricted-environment libasan stack discovery during C++
   exception unwinding. This probe is independent of FlashVM. */
struct Failure
{
    int result;
    Failure(int r) : result(r)
    {
    }
};
struct Local
{
    int a[4];
    ~Local()
    {
        a[0] = 0;
    }
};
void reject(bool ok)
{
    if (!ok)
        throw Failure(-4);
}
void inner()
{
    Local local;
    local.a[0] = 1;
    reject(false);
}
void middle()
{
    try
    {
        inner();
    }
    catch (...)
    {
        throw;
    }
}
int main()
{
    try
    {
        middle();
    }
    catch (const Failure &f)
    {
        return f.result == -4 ? 0 : 1;
    }
    return 2;
}

extern "C" const char *__asan_default_options()
{
    return "detect_leaks=0:halt_on_error=1";
}
