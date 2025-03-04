/* Copyright (C) 2002-2025 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <errno.h>
#include "pthreadP.h"
#include <atomic.h>
#include <shlib-compat.h>

int
___pthread_key_delete (pthread_key_t key)
{
  int result = EINVAL;

  if (__glibc_likely (key < PTHREAD_KEYS_MAX))
    {
      unsigned int seq = __pthread_keys[key].seq;

      if (__builtin_expect (! KEY_UNUSED (seq), 1)
	  && ! atomic_compare_and_exchange_bool_acq (&__pthread_keys[key].seq,
						     seq + 1, seq))
	/* We deleted a valid key.  */
	result = 0;
    }

  return result;
}
versioned_symbol (libc, ___pthread_key_delete, pthread_key_delete,
		  GLIBC_2_34);
libc_hidden_ver (___pthread_key_delete, __pthread_key_delete)
#ifndef SHARED
strong_alias (___pthread_key_delete, __pthread_key_delete)
#endif

#if OTHER_SHLIB_COMPAT (libpthread, GLIBC_2_0, GLIBC_2_34)
compat_symbol (libpthread, ___pthread_key_delete, pthread_key_delete,
	       GLIBC_2_0);
#endif
