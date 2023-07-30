/* Copyright © 2023 Ratakor. See LICENSE file for license details.
 * Copyright © 2020 Elijah Stone. MIT License for defer macros.
 */

#ifndef LIBRE_EVIL_H
#define LIBRE_EVIL_H

#include <stdint.h>
#include "ubik.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define try(func)     _try((void *)(uintptr_t)(func))
#define itry(func)    (int)(uintptr_t)try(func)
#define throw(err)    goto jmp##err
#define catch(err)    if (0) jmp##err:
#ifndef try_errfunc
#define try_errfunc() die(1, 0)
#endif /* try_errfunc */

static inline void *
_try(void *rv)
{
	if (rv == (void *)0 || rv == (void *)-1) {
		try_errfunc();
	}

	return rv;
}

#define defer_init(n)                                                         \
	unsigned char _ndeferrals = 0;                                        \
	void *_defer_return_loc = 0, *_deferrals[n] = {0}
#define defer(block)     _defer(block, __LINE__)
#define defer_fini()     do { _defer_fini(__LINE__) ; } while (0)
#define defer_return(rv) do { _defer_fini(__LINE__) return (rv); } while (0)

#define _defer_concat(a, b) a ## b
#define _defer(block, n)                                                      \
	do {                                                                  \
		_deferrals[_ndeferrals++] = && _defer_concat(_defer_ini, n);  \
		if (0) {                                                      \
_defer_concat(_defer_ini, n):                                                 \
			block;                                                \
			if (_ndeferrals)                                      \
				goto *_deferrals[--_ndeferrals];              \
			else                                                  \
				goto *_defer_return_loc;                      \
		}                                                             \
	} while (0)
#define _defer_fini(n)                                                        \
	if (_ndeferrals) {                                                    \
		_defer_return_loc = && _defer_concat(_defer_fini, n);         \
		goto *_deferrals[--_ndeferrals];                              \
	} _defer_concat(_defer_fini, n):

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LIBRE_EVIL_H */
