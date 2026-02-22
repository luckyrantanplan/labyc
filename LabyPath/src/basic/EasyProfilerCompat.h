/**
 * @file EasyProfilerCompat.h
 * @brief Compatibility header for optional easy_profiler integration.
 *
 * When BUILD_WITH_EASY_PROFILER is defined, includes the real easy_profiler
 * header. Otherwise, provides no-op stubs for the profiling macros.
 */

#ifndef EASY_PROFILER_COMPAT_H_
#define EASY_PROFILER_COMPAT_H_

#ifdef BUILD_WITH_EASY_PROFILER
#include <easy/profiler.h>
#else
// No-op stubs when easy_profiler is not available
#define EASY_PROFILER_ENABLE
#define EASY_FUNCTION(...)
#define EASY_BLOCK(...)
#define EASY_END_BLOCK
namespace profiler {
    inline void startListen() {}
    inline void stopListen() {}
    inline void dumpBlocksToFile(const char*) {}
}
#endif

#endif // EASY_PROFILER_COMPAT_H_
