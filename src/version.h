#ifndef VERSION_H
#define VERSION_H

#define NVI_VERSION "0.0.14"

#ifndef NVI_BUILD
#define NVI_BUILD "debug"
#endif

#ifndef NVI_COMMIT
#define NVI_COMMIT "unknown"
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#define NVI_ARCH "aarch64"
#elif defined(__x86_64__) || defined(_M_X64)
#define NVI_ARCH "x86_64"
#elif defined(__arm__)
#define NVI_ARCH "arm"
#elif defined(__i386__) || defined(_M_IX86)
#define NVI_ARCH "i386"
#else
#define NVI_ARCH "unknown"
#endif

#if defined(__APPLE__)
#define NVI_OS "macos"
#elif defined(__linux__)
#define NVI_OS "linux"
#elif defined(_WIN32) && defined(_MSC_VER)
#define NVI_OS "windows"
#else
#define NVI_OS "unknown"
#endif

#define NVI_TARGET NVI_ARCH "-" NVI_OS

#endif
