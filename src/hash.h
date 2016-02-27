/* Creation date: 2005-06-24 21:22:09
 * Authors: Don
 * Change log:
 */

/* Copyright (c) 2005 Don Owens
   All rights reserved.

   This code is released under the BSD license:

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

     * Redistributions in binary form must reproduce the above
       copyright notice, this list of conditions and the following
       disclaimer in the documentation and/or other materials provided
       with the distribution.

     * Neither the name of the author nor the names of its
       contributors may be used to endorse or promote products derived
       from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _CFU_HASH_H_
#define _CFU_HASH_H_

#include <sys/types.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct    cmark_hash_table        cmark_hash_table;

typedef u_int32_t (*CMarkHashFunc)        (const void * key,
                                           size_t       length);

typedef void      (*CMarkHashFreeFunc)    (void       * data);

typedef int       (*CMarkHashRemoveFunc)  (void       * key,
                                           size_t       key_size,
                                           void       * data,
                                           size_t       data_size,
                                           void       * arg);

typedef int       (*CMarkHashForeachFunc) (void       * key,
                                           size_t       key_size,
                                           void       * data,
                                           size_t       data_size,
                                           void       * arg);

cmark_hash_table *
cmark_hash_new               ();

cmark_hash_table *
cmark_hash_new_with_free_fn  (CMarkHashFreeFunc    ff);

int
cmark_hash_set_hash_function (cmark_hash_table     * ht,
                              CMarkHashFunc          hf);

void
cmark_hash_clear             (cmark_hash_table     * ht);

size_t
cmark_hash_foreach_remove    (cmark_hash_table     * ht,
                              CMarkHashRemoveFunc    r_fn,
                              CMarkHashFreeFunc      ff,
                              void                 * arg);

size_t
cmark_hash_foreach           (cmark_hash_table     * ht,
                              CMarkHashForeachFunc   fe_fn,
                              void                 * arg);

int
cmark_hash_destroy           (cmark_hash_table     * ht);

size_t
cmark_hash_num_entries       (cmark_hash_table     * ht);

int
cmark_hash_lock              (cmark_hash_table     * ht);

int
cmark_hash_unlock            (cmark_hash_table     * ht);

void *
cmark_hash_get               (cmark_hash_table     * ht,
                              const char           * key);

int
cmark_hash_exists            (cmark_hash_table     * ht,
                              const char           * key);

void *
cmark_hash_put               (cmark_hash_table     * ht,
                              const char           * key,
                              void                 * data);

void *
cmark_hash_delete            (cmark_hash_table     * ht,
                              const char           * key);

void **
cmark_hash_keys              (cmark_hash_table     * ht,
                              size_t               * num_keys,
                              int                    fast);

#ifdef __cplusplus
}
#endif

#endif
