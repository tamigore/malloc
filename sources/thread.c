#include "malloc_pthread.h"

/* Normal (non-recursive) mutex. We removed recursion to lower overhead. */
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t  g_once  = PTHREAD_ONCE_INIT;

/* Optional single-thread fast path: after first time another thread tries to lock
 * we mark multi_threaded=1; before that we skip the mutex to save cycles. */
static int multi_threaded = 0;
static pthread_t first_owner;

static void init_mutex(void)
{
    first_owner = pthread_self();
    /* g_mutex already statically initialized */
}

void malloc_lock(void)
{
    pthread_once(&g_once, init_mutex);
    if (!multi_threaded) {
        pthread_t self = pthread_self();
        if (!pthread_equal(self, first_owner)) {
            /* A second thread arrived; enable locking path */
            __atomic_store_n(&multi_threaded, 1, __ATOMIC_RELAXED);
        } else {
            /* Single-thread so far: skip lock */
            return;
        }
    }
    pthread_mutex_lock(&g_mutex);
}

void malloc_unlock(void)
{
    if (!multi_threaded) return; /* no lock was taken */
    pthread_mutex_unlock(&g_mutex);
}
