#ifndef NTHREAD_H
#define NTHREAD_H

// minimal cross-platform threading shim: threads, a mutex, a condition
// variable, and a core-count query. POSIX maps to pthreads; Windows maps
// to _beginthreadex with SRWLOCK + CONDITION_VARIABLE (Vista+), which
// need no explicit destruction.

#include <stddef.h>

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <process.h>
#include <windows.h>

typedef HANDLE thread_t;
typedef SRWLOCK mutex_t;
typedef CONDITION_VARIABLE cond_t;
typedef unsigned thread_ret_t;
#define NVI_THREAD_CALL __stdcall

static inline int thread_create(thread_t *t, thread_ret_t(NVI_THREAD_CALL *fn)(void *), void *arg) {
    *t = (HANDLE)_beginthreadex(NULL, 0, fn, arg, 0, NULL);
    return *t == NULL ? -1 : 0;
}

static inline void thread_join(thread_t t) {
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
}

static inline void mutex_init(mutex_t *m) { InitializeSRWLock(m); }

static inline void mutex_lock(mutex_t *m) { AcquireSRWLockExclusive(m); }

static inline void mutex_unlock(mutex_t *m) { ReleaseSRWLockExclusive(m); }

static inline void mutex_destroy(mutex_t *m) { (void)m; }

static inline void cond_init(cond_t *c) { InitializeConditionVariable(c); }

static inline void cond_wait(cond_t *c, mutex_t *m) { SleepConditionVariableSRW(c, m, INFINITE, 0); }

static inline void cond_signal(cond_t *c) { WakeConditionVariable(c); }

static inline void cond_broadcast(cond_t *c) { WakeAllConditionVariable(c); }

static inline void cond_destroy(cond_t *c) { (void)c; }

static inline size_t cpu_count(void) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors > 0 ? (size_t)si.dwNumberOfProcessors : 1;
}

#else

#include <pthread.h>
#include <unistd.h>

typedef pthread_t thread_t;
typedef pthread_mutex_t mutex_t;
typedef pthread_cond_t cond_t;
typedef void *thread_ret_t;
#define NVI_THREAD_CALL

static inline int thread_create(thread_t *t, thread_ret_t (*fn)(void *), void *arg) {
    return pthread_create(t, NULL, fn, arg);
}

static inline void thread_join(thread_t t) { pthread_join(t, NULL); }

static inline void mutex_init(mutex_t *m) { pthread_mutex_init(m, NULL); }

static inline void mutex_lock(mutex_t *m) { pthread_mutex_lock(m); }

static inline void mutex_unlock(mutex_t *m) { pthread_mutex_unlock(m); }

static inline void mutex_destroy(mutex_t *m) { pthread_mutex_destroy(m); }

static inline void cond_init(cond_t *c) { pthread_cond_init(c, NULL); }

static inline void cond_wait(cond_t *c, mutex_t *m) { pthread_cond_wait(c, m); }

static inline void cond_signal(cond_t *c) { pthread_cond_signal(c); }

static inline void cond_broadcast(cond_t *c) { pthread_cond_broadcast(c); }

static inline void cond_destroy(cond_t *c) { pthread_cond_destroy(c); }

static inline size_t cpu_count(void) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n > 0 ? (size_t)n : 1;
}

#endif

#endif // NVI_NTHREAD_H
