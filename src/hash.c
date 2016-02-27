/* Creation date: 2005-06-24 21:22:40
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

#include <assert.h>
#include <ctype.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"

#define DEFAULT_SIZE 8

typedef struct cmark_hash_event_flags {
	int resized:1;
	int pad:31;
} cmark_hash_event_flags;

typedef struct cmark_hash_entry {
	void *key;
	size_t key_size;
	void *data;
	size_t data_size;
	struct cmark_hash_entry *next;
} cmark_hash_entry;

struct cmark_hash_table {
	size_t num_buckets;
	size_t entries; /* Total number of entries in the table. */
	cmark_hash_entry **buckets;
	pthread_mutex_t mutex;
	CMarkHashFunc hash_func;
	size_t each_bucket_index;
	cmark_hash_entry *each_chain_entry;
	float high;
	float low;
	CMarkHashFreeFunc free_fn;
	unsigned int resized_count;
	cmark_hash_event_flags event_flags;
};

/* Perl's hash function */
static u_int32_t
hash_func(const void *key, size_t length) {
	register size_t i = length;
	register unsigned int hv = 0; /* could put a seed here instead of zero */
	register const unsigned char *s = (const unsigned char *)key;
	while (i--) {
		hv += *s++;
		hv += (hv << 10);
		hv ^= (hv >> 6);
	}
	hv += (hv << 3);
	hv ^= (hv >> 11);
	hv += (hv << 15);

	return hv;
}

/* makes sure the real size of the buckets array is a power of 2 */
static unsigned int
hash_size(unsigned int s) {
	unsigned int i = 1;
	while (i < s) i <<= 1;
	return i;
}

static inline void *
hash_key_dup(const void *key, size_t key_size) {
	void *new_key = malloc(key_size);
	memcpy(new_key, key, key_size);
	return new_key;
}

static inline void *
hash_key_dup_lower_case(const void *key, size_t key_size) {
	char *new_key = (char *)hash_key_dup(key, key_size);
	size_t i = 0;
	for (i = 0; i < key_size; i++) new_key[i] = tolower(new_key[i]);
	return (void *)new_key;
}

/* returns the index into the buckets array */
static inline unsigned int
hash_value(cmark_hash_table *ht, const void *key, size_t key_size, size_t num_buckets) {
	unsigned int hv = 0;

	if (key)
		hv = ht->hash_func(key, key_size);

	/* The idea is the following: if, e.g., num_buckets is 32
	   (000001), num_buckets - 1 will be 31 (111110). The & will make
	   sure we only get the first 5 bits which will guarantee the
	   index is less than 32.
	*/
	return hv & (num_buckets - 1);
}

static inline void
lock_hash(cmark_hash_table *ht) {
	if (!ht) return;
	pthread_mutex_lock(&ht->mutex);
}

static inline void
unlock_hash(cmark_hash_table *ht) {
	if (!ht) return;
	pthread_mutex_unlock(&ht->mutex);
}

static int
hash_rehash(cmark_hash_table *ht) {
	size_t new_size, i;
	cmark_hash_entry **new_buckets = NULL;

	lock_hash(ht);
	new_size = hash_size(ht->entries * 2 / (ht->high + ht->low));
	if (new_size == ht->num_buckets) {
		unlock_hash(ht);
		return 0;
	}
	new_buckets = (cmark_hash_entry **)calloc(new_size, sizeof(cmark_hash_entry *));

	for (i = 0; i < ht->num_buckets; i++) {
		cmark_hash_entry *he = ht->buckets[i];
		while (he) {
			cmark_hash_entry *nhe = he->next;
			unsigned int hv = hash_value(ht, he->key, he->key_size, new_size);
			he->next = new_buckets[hv];
			new_buckets[hv] = he;
			he = nhe;
		}
	}

	ht->num_buckets = new_size;
	free(ht->buckets);
	ht->buckets = new_buckets;
	ht->resized_count++;

	unlock_hash(ht);
	return 1;
}

static cmark_hash_table *
hash_new(void) {
	cmark_hash_table *ht;

	ht = (cmark_hash_table *)malloc(sizeof(cmark_hash_table));
	memset(ht, '\000', sizeof(cmark_hash_table));

	ht->num_buckets = DEFAULT_SIZE;
	ht->entries = 0;
	ht->buckets = (cmark_hash_entry **)calloc(DEFAULT_SIZE, sizeof(cmark_hash_entry *));
	pthread_mutex_init(&ht->mutex, NULL);
	
	ht->hash_func = hash_func;
	ht->high = 0.75;
	ht->low = 0.25;
	
	return ht;
}

/* see if this key matches the one in the hash entry */
/* uses the convention that zero means a match, like memcmp */

static inline int
hash_cmp(const void *key, size_t key_size, cmark_hash_entry *he) {
	if (key_size != he->key_size) return 1;
	if (key == he->key) return 0;
	return memcmp(key, he->key, key_size);
}

static inline cmark_hash_entry *
hash_add_entry(cmark_hash_table *ht, unsigned int hv, const void *key, size_t key_size,
	void *data, size_t data_size) {
	cmark_hash_entry *he = (cmark_hash_entry *)calloc(1, sizeof(cmark_hash_entry));

	assert(hv < ht->num_buckets);

	he->key = hash_key_dup(key, key_size);
	he->key_size = key_size;
	he->data = data;
	he->data_size = data_size;
	he->next = ht->buckets[hv];
	ht->buckets[hv] = he;
	ht->entries++;

	return he;
}

/*
 Returns one if the entry was found, zero otherwise.  If found, r is
 changed to point to the data in the entry.
*/
static int
hash_get_data(cmark_hash_table *ht, const void *key, size_t key_size, void **r,
	size_t *data_size) {
	unsigned int hv = 0;
	cmark_hash_entry *hr = NULL;

	if (!ht) return 0;

	if (key_size == (size_t)(-1)) {
		if (key) key_size = strlen(key) + 1;
		else key_size = 0;
	   
	}

	lock_hash(ht);
	hv = hash_value(ht, key, key_size, ht->num_buckets);

	assert(hv < ht->num_buckets);

	for (hr = ht->buckets[hv]; hr; hr = hr->next) {
		if (!hash_cmp(key, key_size, hr)) break;
	}

	if (hr && r) {
		*r = hr->data;
		if (data_size) *data_size = hr->data_size;
	}

	unlock_hash(ht);
	
	return (hr ? 1 : 0);
}

static void *
hash_delete_data(cmark_hash_table *ht, const void *key, size_t key_size) {
	unsigned int hv = 0;
	cmark_hash_entry *he = NULL;
	cmark_hash_entry *hep = NULL;
	void *r = NULL;

	if (key_size == (size_t)(-1)) key_size = strlen(key) + 1;
	lock_hash(ht);
	hv = hash_value(ht, key, key_size, ht->num_buckets);

	for (he = ht->buckets[hv]; he; he = he->next) {
		if (!hash_cmp(key, key_size, he)) break;
		hep = he;
	}

	if (he) {
		r = he->data;
		if (hep) hep->next = he->next;
		else ht->buckets[hv] = he->next;

		ht->entries--;
		free(he->key);
		if (ht->free_fn) {
			ht->free_fn(he->data);
			r = NULL; /* don't return a pointer to a free()'d location */
		}
		free(he);
	}

	unlock_hash(ht);

	if (ht->resized_count) {
		if ( (float)ht->entries/(float)ht->num_buckets < ht->low )
      hash_rehash(ht);
	}


	return r;
}

static void **
hash_keys_data(cmark_hash_table *ht, size_t *num_keys, size_t **key_sizes, int fast) {
	size_t *key_lengths = NULL;
	void **keys = NULL;
	cmark_hash_entry *he = NULL;
	size_t bucket = 0;
	size_t entry_index = 0;
	size_t key_count = 0;

	if (!ht) {
		*key_sizes = NULL;
		*num_keys = 0;
		return NULL;
	}

	lock_hash(ht);

	if (key_sizes) key_lengths = (size_t *)calloc(ht->entries, sizeof(size_t));
	keys = (void **)calloc(ht->entries, sizeof(void *));
	
	for (bucket = 0; bucket < ht->num_buckets; bucket++) {
		if ( (he = ht->buckets[bucket]) ) {
			for (; he; he = he->next, entry_index++) {
				if (entry_index >= ht->entries) break; /* this should never happen */

				if (fast) {
					keys[entry_index] = he->key;
				} else {
					keys[entry_index] = (void *)calloc(he->key_size, 1);
					memcpy(keys[entry_index], he->key, he->key_size);
				}
				key_count++;

				if (key_lengths) key_lengths[entry_index] = he->key_size;
			}
		}
	}

	unlock_hash(ht);

	if (key_sizes) *key_sizes = key_lengths;
	*num_keys = key_count;

	return keys;
}

static void
_cmark_hash_destroy_entry(cmark_hash_table *ht, cmark_hash_entry *he, CMarkHashFreeFunc ff) {
	if (ff) {
		ff(he->data);
	} else {
		if (ht->free_fn)
      ht->free_fn(he->data);
	}
	free(he->key);
	free(he);
}

static int
hash_destroy_with_free_fn(cmark_hash_table *ht, CMarkHashFreeFunc ff) {
	size_t i;
	if (!ht) return 0;

	lock_hash(ht);
	for (i = 0; i < ht->num_buckets; i++) {
		if (ht->buckets[i]) {
			cmark_hash_entry *he = ht->buckets[i];
			while (he) {
				cmark_hash_entry *hn = he->next;
				_cmark_hash_destroy_entry(ht, he, ff);
				he = hn;
			}
		}
	}
	free(ht->buckets);
	unlock_hash(ht);
	pthread_mutex_destroy(&ht->mutex);
	free(ht);

	return 1;
}

cmark_hash_table *
cmark_hash_new() {
	return hash_new();
}

cmark_hash_table * cmark_hash_new_with_free_fn(CMarkHashFreeFunc ff) {
	cmark_hash_table *ht = hash_new();
	if (ff)
    ht->free_fn = ff;
	return ht;
}

/* Sets the hash function for the hash table ht.  Pass NULL for hf to reset to the default */
int
cmark_hash_set_hash_function(cmark_hash_table *ht, CMarkHashFunc hf) {
	/* can't allow changing the hash function if the hash already contains entries */
	if (ht->entries) return -1;
	
	ht->hash_func = hf ? hf : hash_func;
	return 0;
}

int
cmark_hash_lock(cmark_hash_table *ht) {
	pthread_mutex_lock(&ht->mutex);
	return 1;
}

int
cmark_hash_unlock(cmark_hash_table *ht) {
	pthread_mutex_unlock(&ht->mutex);
	return 1;
}

/*
 Assumes the key is a null-terminated string, returns the data, or NULL if not found.  Note that it is possible for the data itself to be NULL
*/
void *
cmark_hash_get(cmark_hash_table *ht, const char *key) {
	void *r = NULL;
	int rv = 0;
	
	rv = hash_get_data(ht, (const void *)key, -1, &r, NULL);
	if (rv) return r; /* found */
	return NULL;
}

/* Returns 1 if an entry exists in the table for the given key, 0 otherwise */
int
cmark_hash_exists_data(cmark_hash_table *ht, const void *key, size_t key_size) {
	void *r = NULL;
	int rv = hash_get_data(ht, key, key_size, &r, NULL);
	if (rv) return 1; /* found */
	return 0;
}

/* Same as cmark_hash_exists_data(), except assumes key is a null-terminated string */
int
cmark_hash_exists(cmark_hash_table *ht, const char *key) {
	return cmark_hash_exists_data(ht, (const void *)key, -1);
}

/*
 Add the entry to the hash.  If there is already an entry for the
 given key, the old data value will be returned in r, and the return
 value is zero.  If a new entry is created for the key, the function
 returns 1.
*/
int
cmark_hash_put_data(cmark_hash_table *ht, const void *key, size_t key_size, void *data,
	size_t data_size, void **r) {
	unsigned int hv = 0;
	cmark_hash_entry *he = NULL;
	int added_an_entry = 0;
	
	if (key_size == (size_t)(-1)) {
		if (key) key_size = strlen(key) + 1;
		else key_size = 0;
	}
	if (data_size == (size_t)(-1)) {
		if (data) data_size = strlen(data) + 1;
		else data_size = 0;
		
	}

	lock_hash(ht);
	hv = hash_value(ht, key, key_size, ht->num_buckets);
	assert(hv < ht->num_buckets);
	for (he = ht->buckets[hv]; he; he = he->next) {
		if (!hash_cmp(key, key_size, he))
      break;
	}

	if (he) {
		if (r) *r = he->data;
		if (ht->free_fn) {
			ht->free_fn(he->data);
			if (r) *r = NULL; /* don't return a pointer to a free()'d location */
		}
		he->data = data;
		he->data_size = data_size;
	} else {
		hash_add_entry(ht, hv, key, key_size, data, data_size);
		added_an_entry = 1;
	}

	unlock_hash(ht);	

	if (added_an_entry) {
		if ( (float)ht->entries/(float)ht->num_buckets > ht->high ) hash_rehash(ht);
	}

	return added_an_entry;
}

/*
 Same as cmark_hash_put_data(), except the key is assumed to be a
 null-terminated string, and the old value is returned if it existed,
 otherwise NULL is returned.
*/
void *
cmark_hash_put(cmark_hash_table *ht, const char *key, void *data) {
	void *r = NULL;
	if (!cmark_hash_put_data(ht, (const void *)key, -1, data, 0, &r)) {
		return r;
	}
	return NULL;
}

void
cmark_hash_clear(cmark_hash_table *ht) {
	cmark_hash_entry *he = NULL;
	cmark_hash_entry *hep = NULL;
	size_t i = 0;

	lock_hash(ht);
	for (i = 0; i < ht->num_buckets; i++) {
		if ( (he = ht->buckets[i]) ) {
			while (he) {
				hep = he;
				he = he->next;
				free(hep->key);
				if (ht->free_fn) ht->free_fn(hep->data);
				free(hep);
			}
			ht->buckets[i] = NULL;
		}
	}
	ht->entries = 0;

	unlock_hash(ht);

	if (ht->resized_count) {
		if ( (float)ht->entries/(float)ht->num_buckets < ht->low )
      hash_rehash(ht);
	}
}

void *
cmark_hash_delete(cmark_hash_table *ht, const char *key) {
	return hash_delete_data(ht, key, -1);
}

void **
cmark_hash_keys(cmark_hash_table *ht, size_t *num_keys, int fast) {
	return hash_keys_data(ht, num_keys, NULL, fast);
}

size_t
cmark_hash_foreach_remove(cmark_hash_table *ht, CMarkHashRemoveFunc r_fn, CMarkHashFreeFunc ff,
					   void *arg) {
	cmark_hash_entry *entry = NULL;
	cmark_hash_entry *prev = NULL;
	size_t hv = 0;
	size_t num_removed = 0;
	cmark_hash_entry **buckets = NULL;
	size_t num_buckets = 0;
	
	if (!ht) return 0;

	lock_hash(ht);

	buckets = ht->buckets;
	num_buckets = ht->num_buckets;
	for (hv = 0; hv < num_buckets; hv++) {
		entry = buckets[hv];
		if (!entry) continue;
		prev = NULL;

		while (entry) {
			if (r_fn(entry->key, entry->key_size, entry->data, entry->data_size, arg)) {
				num_removed++;
				if (prev) {
					prev->next = entry->next;
					_cmark_hash_destroy_entry(ht, entry, ff);
					entry = prev->next;
				} else {
					buckets[hv] = entry->next;
					_cmark_hash_destroy_entry(ht, entry, NULL);
					entry = buckets[hv];
				}
			} else {
				prev = entry;
				entry = entry->next;
			}
		}
	}

	unlock_hash(ht);

	return num_removed;
}

size_t
cmark_hash_foreach(cmark_hash_table *ht, CMarkHashForeachFunc fe_fn, void *arg) {
	cmark_hash_entry *entry = NULL;
	size_t hv = 0;
	size_t num_accessed = 0;
	cmark_hash_entry **buckets = NULL;
	size_t num_buckets = 0;
	int rv = 0;
	
	if (!ht) return 0;

	lock_hash(ht);

	buckets = ht->buckets;
	num_buckets = ht->num_buckets;
	for (hv = 0; hv < num_buckets && !rv; hv++) {
		entry = buckets[hv];

		for (; entry && !rv; entry = entry->next) {
			num_accessed++;
			rv = fe_fn(entry->key, entry->key_size, entry->data, entry->data_size, arg);
		}
	}

	unlock_hash(ht);

	return num_accessed;
}

int
cmark_hash_destroy(cmark_hash_table *ht) {
	return hash_destroy_with_free_fn(ht, NULL);
}

size_t
cmark_hash_num_entries(cmark_hash_table *ht) {
	if (!ht) return 0;
	return ht->entries;
}
