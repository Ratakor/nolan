/*
 * Copyright © 2023, Ratakor <ratakor@disroot.org>
 *
 * This library is free software. You can redistribute it and/or modify it
 * under the terms of the ISC license. See cthread.c for details.
 */

#ifndef LIBRE_CTHREAD_H
#define LIBRE_CTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef _WIN32
#include <windows.h>

#define CTHREAD_MUTEX_INITIALIZER NULL

typedef HANDLE cthread_t;
typedef HANDLE cthread_mutex_t;
typedef HANDLE cthread_cond_t;
typedef HANDLE cthread_rwlock_t;
#else
#include <pthread.h>

#define CTHREAD_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

typedef pthread_t cthread_t;
typedef pthread_mutex_t cthread_mutex_t;
typedef pthread_cond_t cthread_cond_t;
typedef pthread_rwlock_t cthread_rwlock_t;
#endif /* _WIN32 */

typedef struct {
	size_t stacksize;

	int w_dwCreationFlags;

	int p_detachstate;
	size_t p_guardsize;
	int p_inheritsched;
	int p_schedpolicy;
	int p_scope;
	size_t p_stack;
	void *p_stackaddr;
} cthread_attr_t;

typedef struct {
	char *w_lpName;
	int w_bInitialOwner;

	int p_pshared;
	int p_type;
	int p_robust;
	int p_protocol;
	int p_prioceiling;
} cthread_mutexattr_t;

typedef struct {
	char *w_lpName;
	int w_bManualReset;
	int w_bInitialState;

	int p_pshared;
	int p_clock;
} cthread_condattr_t;

/**
 * Create a new thread.
 *
 * @param thread Pointer to the thread.
 * @param attr Pointer to the thread attributes. NULL for default attributes.
 * @param func Pointer to the function that will be executed in the new thread.
 * @param arg Pointer to the data that will be passed to the thread function.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_create(cthread_t *thread, cthread_attr_t *attr,
                   void *(*func)(void *), void *arg);

/**
 * Join with a terminated thread.
 *
 * @param thread The thread to be joined.
 * @param retval Pointer to store the exit code of the joined thread.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_join(cthread_t thread, void **retval);

/**
 * Detach a thread.
 *
 * @param thread Pointer to the thread.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_detach(cthread_t thread);

/**
 * Compares two threads for equality.
 *
 * @param t1 First thread to compare.
 * @param t2 Second thread to compare.
 * @return Non-zero if the threads are equal, zero otherwise.
 */
int cthread_equal(cthread_t t1, cthread_t t2);

/**
 * Obtain ID of the current thread.
 *
 * @return Thread ID of the current thread.
 */
cthread_t cthread_self(void);

/**
 * Terminate the current thread.
 *
 * @param retval Pointer to store the thread exit code.
 */
void cthread_exit(void *retval);

/**
 * Initialize a mutex.
 *
 * @param mutex Pointer to the mutex.
 * @param attr Pointer to the mutex attributes. NULL for default attributes.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_mutex_init(cthread_mutex_t *mutex, cthread_mutexattr_t *attr);

/**
 * Lock a mutex.
 *
 * @param mutex Pointer to the mutex.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_mutex_lock(cthread_mutex_t *mutex);

/**
 * Try to lock a mutex without blocking.
 *
 * @param mutex Pointer to the mutex.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_mutex_trylock(cthread_mutex_t *mutex);

/**
 * Unlock a mutex.
 *
 * @param mutex Pointer to the mutex.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_mutex_unlock(cthread_mutex_t *mutex);

/**
 * Destroy a mutex.
 *
 * @param mutex Pointer to the mutex.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_mutex_destroy(cthread_mutex_t *mutex);

/**
 * Initialize a condition.
 *
 * @param cond Pointer to the condition.
 * @param attr Pointer to the condition attributes. NULL for default.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_cond_init(cthread_cond_t *cond, cthread_condattr_t *attr);

/**
 * Signal a condition.
 *
 * @param cond Pointer to the condition.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_cond_signal(cthread_cond_t *cond);

/**
 * Broadcast a condition.
 *
 * @param cond Pointer to the condition.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_cond_broadcast(cthread_cond_t *cond);

/**
 * Wait on a condition.
 *
 * @param cond Pointer to the condition.
 * @param mutex Pointer to the associated mutex.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_cond_wait(cthread_cond_t *cond, cthread_mutex_t *mutex);

/**
 * Destroy a condition.
 *
 * @param cond Pointer to the condition.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_cond_destroy(cthread_cond_t *cond);

/**
 * Initialize a read-write lock.
 *
 * @param rwlock Pointer to the read-write lock.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_rwlock_init(cthread_rwlock_t *rwlock);

/**
 * Lock a read-write lock for reading.
 *
 * @param rwlock Pointer to the read-write lock.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_rwlock_rdlock(cthread_rwlock_t *rwlock);

/**
 * Lock a read-write lock for writing.
 *
 * @param rwlock Pointer to the read-write lock.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_rwlock_wrlock(cthread_rwlock_t *rwlock);

/**
 * Unlock a read-write lock.
 *
 * @param rwlock Pointer to the read-write lock.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_rwlock_unlock(cthread_rwlock_t *rwlock);

/**
 * Destroy a read-write lock.
 *
 * @param rwlock Pointer to the read-write lock.
 * @return 0 on success, non-zero error code on failure.
 */
int cthread_rwlock_destroy(cthread_rwlock_t *rwlock);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LIBRE_CTHREAD_H */
