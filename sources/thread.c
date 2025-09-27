#include "malloc_pthread.h"

// Recursive global allocator mutex state
static struct s_malloc_mutex_state
{
	pthread_mutex_t mutex;
	pthread_once_t once;
} g_malloc_mutex = {0};

static void malloc_mutex_init(void)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&g_malloc_mutex.mutex, &attr);
	pthread_mutexattr_destroy(&attr);
}

void malloc_lock(void)
{
	pthread_once(&g_malloc_mutex.once, malloc_mutex_init);
	pthread_mutex_lock(&g_malloc_mutex.mutex);
}

void malloc_unlock(void)
{
	pthread_mutex_unlock(&g_malloc_mutex.mutex);
}
