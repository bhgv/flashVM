/* Verify append failure atomicity, aliasing and linear allocation work for many path fragments. */
#include "flashvm/path.h"
#include <cstdio>
#include <cstdlib>
#include <new>

static long allowance = -1;       /* Fail the selected allocation while a transaction is under observation. */
static bool counting = false;     /* Count allocated bytes only inside the fragment-growth matrix. */
static std::size_t requested = 0; /* Cumulative native allocation work, not peak/live storage. */

/* Observe/fail native allocations; bytes is copied, result is malloc-owned or throws bad_alloc. */
void *operator new(std::size_t bytes) throw(std::bad_alloc)
{
    if (!allowance)
    {
        throw std::bad_alloc();
    }
    if (allowance > 0)
    {
        --allowance;
    }
    if (counting)
    {
        requested += bytes;
    }
    void *result = std::malloc(bytes ? bytes : 1);
    if (!result)
    {
        throw std::bad_alloc();
    }
    return result;
}


/* Release malloc-owned test storage; null is accepted. */
void operator delete(void *storage) throw()
{
    std::free(storage);
}


/* Route array allocations through the same independently observed byte budget. */
void *operator new[](std::size_t bytes) throw(std::bad_alloc)
{
    return ::operator new(bytes);
}


/* Release test array storage using the matching allocator. */
void operator delete[](void *storage) throw()
{
    ::operator delete(storage);
}


/* Preserve nothrow allocation semantics while retaining allocation-failure injection. */
void *operator new(std::size_t bytes, const std::nothrow_t &) throw()
{
    try
    {
        return ::operator new(bytes);
    }
    catch (...)
    {
        return 0;
    }
}


/* Release storage after a failed nothrow construction. */
void operator delete(void *storage, const std::nothrow_t &) throw()
{
    ::operator delete(storage);
}


/* Route nothrow array allocations through the same observer. */
void *operator new[](std::size_t bytes, const std::nothrow_t &) throw()
{
    return ::operator new(bytes, std::nothrow);
}


/* Release storage after a failed nothrow array construction. */
void operator delete[](void *storage, const std::nothrow_t &) throw()
{
    ::operator delete(storage);
}


/* Require an independently specified public result; failure exits with a concrete diagnostic. */
static void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::fprintf(stderr, "%s\n", message);
        std::exit(EXIT_FAILURE);
    }
}


/* Append a scalar command to a borrowed exclusive test path; returns its public status. */
static FVM_Result add(FVM_Path *path, FVM_PathVerb verb, double x, double y)
{
    FVM_PathCommand command = {};
    command.verb = verb;
    command.x[0] = x;
    command.y[0] = y;
    return fvm_path_add(path, &command);
}


/* Create an eight-command open path with independent expected coordinates. */
static FVM_Path *destination()
{
    FVM_Path *path = 0;
    require(fvm_path_create(&path) == FVM_OK, "create destination");
    for (unsigned int index = 0; index < 8; ++index)
    {
        require(add(path, index ? FVM_PATH_LINE : FVM_PATH_MOVE, index, index + 1) == FVM_OK, "seed destination");
    }
    return path;
}


/* Check every original public command and its coordinates after a failed transaction. */
static void unchanged(FVM_Path *path)
{
    require(fvm_path_count(path) == 8, "append failure changed command count");
    for (unsigned int index = 0; index < 8; ++index)
    {
        FVM_PathCommand command;
        require(fvm_path_get(path, index, &command) == FVM_OK, "read preserved command");
        require(command.verb == (index ? FVM_PATH_LINE : FVM_PATH_MOVE) && command.x[0] == index &&
                    command.y[0] == index + 1,
                "append failure changed original geometry");
    }
}


/* Exercise every staging/reservation failure, self-append, late transform rejection and memory-work bounds. */
int main()
{
    FVM_Path *source = 0;
    require(fvm_path_create(&source) == FVM_OK, "create source");
    require(add(source, FVM_PATH_MOVE, 0, 0) == FVM_OK && add(source, FVM_PATH_LINE, 1, 2) == FVM_OK &&
                add(source, FVM_PATH_CLOSE, 0, 0) == FVM_OK,
            "seed closed source");
    FVM_Affine matrix = {1, 0, 0, 1, 10, -10};
    bool failed = false;
    bool succeeded = false;
    for (long boundary = 0; boundary < 8 && !succeeded; ++boundary)
    {
        FVM_Path *path = destination();
        allowance = boundary;
        FVM_Result result = fvm_path_append(path, source, &matrix);
        allowance = -1;
        if (result == FVM_OUT_OF_MEMORY)
        {
            failed = true;
            unchanged(path);
            require(add(path, FVM_PATH_LINE, 9, 9) == FVM_OK, "append failure closed original open contour");
        }
        else
        {
            require(result == FVM_OK && fvm_path_count(path) == 11, "successful append geometry/count");
            require(add(path, FVM_PATH_LINE, 9, 9) == FVM_INVALID_ARGUMENT,
                    "closed source did not close appended contour");
            succeeded = true;
        }
        fvm_path_destroy(path);
    }
    require(failed && succeeded, "allocation matrix missed failure or success");
    FVM_Path *path = destination();
    FVM_Affine invalid = {1000000, 0, 0, 1000000, 0, 0};
    require(fvm_path_append(path, source, &invalid) == FVM_INVALID_ARGUMENT, "late transformed point was admitted");
    unchanged(path);
    require(fvm_path_append(path, path, &matrix) == FVM_OK && fvm_path_count(path) == 16, "self-append failed");
    for (unsigned int index = 0; index < 8; ++index)
    {
        FVM_PathCommand command;
        require(fvm_path_get(path, index + 8, &command) == FVM_OK && command.x[0] == index + 10 &&
                    command.y[0] == index - 9.0,
                "self-append did not use the original source stream");
    }
    fvm_path_destroy(path);
    require(fvm_path_create(&path) == FVM_OK, "create fragment destination");
    requested = 0;
    counting = true;
    for (unsigned int index = 0; index < 1024; ++index)
    {
        matrix.x0 = index * 3;
        require(fvm_path_append(path, source, &matrix) == FVM_OK, "append repeated fragment");
    }
    counting = false;
    require(fvm_path_count(path) == 3072, "fragment append lost commands");
    require(requested < 16 * 3072 * sizeof(FVM_PathCommand), "fragment append exceeded linear allocation-work budget");
    for (unsigned int index = 0; index < 1024; ++index)
    {
        FVM_PathCommand command;
        require(fvm_path_get(path, index * 3 + 1, &command) == FVM_OK && command.x[0] == index * 3 + 1 &&
                    command.y[0] == -8,
                "fragment append changed command order or transform");
    }
    fvm_path_destroy(path);
    fvm_path_destroy(source);
    std::puts("PASS path append: allocation/transform atomicity, self alias, open state and linear fragment growth");
    return 0;
}
