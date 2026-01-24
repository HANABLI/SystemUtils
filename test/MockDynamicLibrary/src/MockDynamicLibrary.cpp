#ifdef _WIN32
#    define API __declspec(dllexport)
#else /* POSIX */
#    define API __attribute__((visibility("default")))
#endif /* _WIN32 / POSIX */

extern "C" API int Foo(int x) {
    return x * x;
}