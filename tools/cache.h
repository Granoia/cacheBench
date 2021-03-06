#pragma once

#include <inttypes.h>

//Our key buffer stores a fix sized key.
//This value can be safely modified, but will affect overhead.
#define MAX_KEY_LEN (64)

struct cache_obj;
typedef struct cache_obj *cache_t;

typedef const uint8_t *key_type;
typedef const void *val_type;

// For a given key string, return a pseudo-random integer:
typedef uint64_t (*hash_func)(key_type key);

typedef void (*rpmt_func)(cache_t cache);

// Create a new cache object with a given maximum memory capacity.
cache_t create_cache(uint64_t maxmem, hash_func hasher);

// Add a <key, value> pair to the cache.
// If key already exists, it will overwrite the old value.
// If maxmem capacity is exceeded, sufficient values will be removed
// from the cache to accomodate the new value.
uint8_t cache_set(cache_t cache, key_type key, val_type val, uint32_t val_size);

// Retrieve the value associated with key in the cache, or NULL if not found
val_type cache_get(cache_t cache, key_type key, uint32_t *val_size);

// Delete an object from the cache, if it's still there
uint8_t cache_delete(cache_t cache, key_type key);

// Compute the total amount of memory used up by all cache values (not keys)
uint64_t cache_space_used(cache_t cache);

// Destroy all resource connected to a cache object
uint8_t destroy_cache(cache_t cache);

//For the cache server
char *server_addr;
