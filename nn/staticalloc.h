/*
	Copyright 2022 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
	Miniature Mobile Robots group, Switzerland
	Author: Yves Piguet

	Licensed under the 3-Clause BSD License;
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at
	https://opensource.org/licenses/BSD-3-Clause
*/

/*
Simple implementation of malloc and free in a static block of memory.

Clients can #define malloc static_malloc and free static_free to use them
as stdlib replacements.
*/

#ifndef __STATICALLOC_H
#define __STATICALLOC_H

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(NULL)
#	define NULL 0
#endif

void *static_malloc(int n);

void static_free(void *p);

#if defined(__cplusplus)
}
#endif

#endif
