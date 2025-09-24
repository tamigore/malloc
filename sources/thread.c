#include <pthread.h>
#include "malloc_pthread.h"

static pthread_mutex_t g_mutex;
static pthread_once_t g_once = PTHREAD_ONCE_INIT;

static void init_mutex(void)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

void malloc_lock(void)
{
    pthread_once(&g_once, init_mutex);
    pthread_mutex_lock(&g_mutex);
}

void malloc_unlock(void)
{
    pthread_mutex_unlock(&g_mutex);
}
