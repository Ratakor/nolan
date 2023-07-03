/*
 * Copyright © 2023, Ratakor <ratakor@disroot.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "cthread.h"

#ifdef _WIN32
#include <stdint.h>
#define WIN32_FAILURE 0
typedef DWORD WINAPI w_func(LPVOID);
#endif /* _WIN32 */

int
cthread_create(cthread_t *thread, cthread_attr_t *attr, void *(*func)(void *),
               void *arg)
{
#ifdef _WIN32
	if (attr == NULL)
		*thread = CreateThread(NULL, 0, (w_func *)func, arg, 0, NULL);
	else
		*thread = CreateThread(NULL, attr->stacksize, (w_func *)func,
		                       arg, attr->w_dwCreationFlags, NULL);

	return *thread == NULL;
#else
	pthread_attr_t a;
	int ret;

	if ((ret = pthread_attr_init(&a)) != 0)
		return ret;

	if (attr != NULL) {
		if (attr->stacksize)
			pthread_attr_setstacksize(&a, attr->stacksize);
		if (attr->p_detachstate)
			pthread_attr_setdetachstate(&a, attr->p_detachstate);
		if (attr->p_guardsize)
			pthread_attr_setguardsize(&a, attr->p_guardsize);
#ifndef __ANDROID__
		if (attr->p_inheritsched)
			pthread_attr_setinheritsched(&a, attr->p_inheritsched);
#endif /* __ANDROID__ */
		if (attr->p_schedpolicy)
			pthread_attr_setschedpolicy(&a, attr->p_schedpolicy);
		if (attr->p_scope)
			pthread_attr_setscope(&a, attr->p_scope);
		if (attr->p_stack)
			pthread_attr_setstack(&a, attr->p_stackaddr,
			                      attr->p_stack);
	}

	ret = pthread_create(thread, &a, func, arg);
	pthread_attr_destroy(&a);

	return ret;
#endif /* _WIN32 */
}

int
cthread_join(cthread_t thread, void **retval)
{
#ifdef _WIN32
	if (WaitForSingleObject(thread, INFINITE) == WAIT_FAILED)
		return 0;

	return GetExitCodeThread(thread, (LPDWORD)retval) == WIN32_FAILURE;
#else
	return pthread_join(thread, retval);
#endif /* _WIN32 */
}

int
cthread_detach(cthread_t thread)
{
#ifdef _WIN32
	return CloseHandle(thread) == WIN32_FAILURE;
#else
	return pthread_detach(thread);
#endif /* _WIN32 */
}

int
cthread_equal(cthread_t t1, cthread_t t2)
{
#ifdef _WIN32
	return t1 == t2;
#else
	return pthread_equal(t1, t2);
#endif /* _WIN32 */
}

cthread_t
cthread_self(void)
{
#ifdef _WIN32
	return GetCurrentThread();
#else
	return pthread_self();
#endif /* _WIN32 */
}

void
cthread_exit(void *retval)
{
#ifdef _WIN32
	ExitThread((DWORD)(uintptr_t)retval);
#else
	pthread_exit(retval);
#endif /* _WIN32 */
}

int
cthread_mutex_init(cthread_mutex_t *mutex, cthread_mutexattr_t *attr)
{
#ifdef _WIN32
	if (attr == NULL)
		*mutex = CreateMutex(NULL, FALSE, NULL);
	else
		*mutex = CreateMutex(NULL, attr->w_bInitialOwner,
		                     attr->w_lpName);

	return *mutex == NULL;
#else
	pthread_mutexattr_t a;
	int ret;

	if ((ret = pthread_mutexattr_init(&a)) != 0)
		return ret;

	if (attr != NULL) {
		if (attr->p_pshared)
			pthread_mutexattr_setpshared(&a, attr->p_pshared);
		if (attr->p_type)
			pthread_mutexattr_settype(&a, attr->p_type);
#if (defined __linux__ || defined __FreeBSD__) && !defined __ANDROID__
		if (attr->p_robust)
			pthread_mutexattr_setrobust(&a, attr->p_robust);
#endif /* (__linux__ || __FreeBSD__) && !__ANDROID__ */
#ifndef __ANDROID__
		if (attr->p_protocol)
			pthread_mutexattr_setprotocol(&a, attr->p_protocol);
		if (attr->p_prioceiling)
			pthread_mutexattr_setprioceiling(&a,
			                                 attr->p_prioceiling);
#endif /* __ANDROID__ */
	}

	ret = pthread_mutex_init(mutex, &a);
	pthread_mutexattr_destroy(&a);

	return ret;
#endif /* _WIN32 */
}

int
cthread_mutex_lock(cthread_mutex_t *mutex)
{
#ifdef _WIN32
	cthread_mutex_t m;

	if (*mutex == CTHREAD_MUTEX_INITIALIZER) {
		m = CreateMutex(NULL, FALSE, NULL);
		if (InterlockedCompareExchangePointer(mutex, m, NULL) != NULL)
			CloseHandle(m);
	}

	return WaitForSingleObject(*mutex, INFINITE) == WAIT_FAILED;
#else
	return pthread_mutex_lock(mutex);
#endif /* _WIN32 */
}

int
cthread_mutex_trylock(cthread_mutex_t *mutex)
{
#ifdef _WIN32
	return WaitForSingleObject(*mutex, 0) == WAIT_FAILED;
#else
	return pthread_mutex_trylock(mutex);
#endif /* _WIN32 */
}

int
cthread_mutex_unlock(cthread_mutex_t *mutex)
{
#ifdef _WIN32
	return ReleaseMutex(*mutex) == WIN32_FAILURE;
#else
	return pthread_mutex_unlock(mutex);
#endif /* _WIN32 */
}

int
cthread_mutex_destroy(cthread_mutex_t *mutex)
{
#ifdef _WIN32
	return CloseHandle(*mutex) == WIN32_FAILURE;
#else
	return pthread_mutex_destroy(mutex);
#endif /* _WIN32 */
}

int
cthread_cond_init(cthread_cond_t *cond, cthread_condattr_t *attr)
{
#ifdef _WIN32
	if (attr == NULL)
		*cond = CreateEvent(NULL, FALSE, FALSE, NULL);
	else
		*cond = CreateEvent(NULL, attr->w_bManualReset,
		                    attr->w_bInitialState, attr->w_lpName);

	return *cond == NULL;
#else
	pthread_condattr_t a;
	int ret;

	if ((ret = pthread_condattr_init(&a)) != 0)
		return ret;

	if (attr != NULL) {
		if (attr->p_pshared)
			pthread_condattr_setpshared(&a, attr->p_pshared);
		if (attr->p_clock)
			pthread_condattr_setclock(&a, attr->p_clock);
	}

	ret = pthread_cond_init(cond, &a);
	pthread_condattr_destroy(&a);

	return ret;
#endif /* _WIN32 */
}

int
cthread_cond_signal(cthread_cond_t *cond)
{
#ifdef _WIN32
	return SetEvent(*cond) == WIN32_FAILURE;
#else
	return pthread_cond_signal(cond);
#endif /* _WIN32 */
}

int
cthread_cond_broadcast(cthread_cond_t *cond)
{
#ifdef _WIN32
	return SetEvent(*cond) == WIN32_FAILURE;
#else
	return pthread_cond_broadcast(cond);
#endif /* _WIN32 */
}

int
cthread_cond_wait(cthread_cond_t *cond, cthread_mutex_t *mutex)
{
#ifdef _WIN32
	if (cthread_mutex_unlock(mutex) == 1)
		return 1;

	if (WaitForSingleObject(*cond, INFINITE) == WAIT_FAILED)
		return 1;

	return cthread_mutex_lock(mutex);
#else
	return pthread_cond_wait(cond, mutex);
#endif /* _WIN32 */
}

int
cthread_cond_destroy(cthread_cond_t *cond)
{
#ifdef _WIN32
	return CloseHandle(*cond) == WIN32_FAILURE;
#else
	return pthread_cond_destroy(cond);
#endif /* _WIN32 */
}

int
cthread_rwlock_init(cthread_rwlock_t *rwlock)
{
#ifdef _WIN32
	*rwlock = CreateMutex(NULL, FALSE, NULL);

	return *rwlock == NULL;
#else
	return pthread_rwlock_init(rwlock, NULL);
#endif /* _WIN32 */
}

int
cthread_rwlock_rdlock(cthread_rwlock_t *rwlock)
{
#ifdef _WIN32
	return WaitForSingleObject(*rwlock, INFINITE) == WAIT_FAILED;
#else
	return pthread_rwlock_rdlock(rwlock);
#endif /* _WIN32 */
}

int
cthread_rwlock_wrlock(cthread_rwlock_t *rwlock)
{
#ifdef _WIN32
	return WaitForSingleObject(*rwlock, INFINITE) == WAIT_FAILED;
#else
	return pthread_rwlock_wrlock(rwlock);
#endif /* _WIN32 */
}

int
cthread_rwlock_unlock(cthread_rwlock_t *rwlock)
{
#ifdef _WIN32
	return ReleaseMutex(*rwlock) == WIN32_FAILURE;
#else
	return pthread_rwlock_unlock(rwlock);
#endif /* _WIN32 */
}

int
cthread_rwlock_destroy(cthread_rwlock_t *rwlock)
{
#ifdef _WIN32
	return CloseHandle(*rwlock) == WIN32_FAILURE;
#else
	return pthread_rwlock_destroy(rwlock);
#endif /* _WIN32 */
}
