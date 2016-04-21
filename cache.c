#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "cache.h"
#include "list.h"
#include "tools.h"
#include "slabs.h"
#include <assert.h>

typedef struct cache_obj 
{
  hash_func hasher;
  uint32_t size;
  manager_t slab_manager;
  linked_list_t* buckets; //Array of linked list pointers
  uint32_t num_buckets;
  uint64_t maxmem;
  uint32_t num_values;
} cache_obj;

void print_cache(cache_t cache)
{
  print_slabs(cache->slab_manager);
}

void print_buckets(cache_t cache)
{
  int i = 0;
  //for each bucket
  for(i; i<cache->num_buckets; i++)
    {
      printf("Bucket %i: ",i);
      linked_list_t current_LL = cache->buckets[i];
      node* curr_node = current_LL->head;
      //print the key and timer of each node in the bucket
      while(curr_node != NULL)
	{
	  meta_t curr_meta = curr_node->obj;
	  uint8_t *curr_key = curr_meta->key;
	  uint8_t timer = curr_meta->timer;
	  printf("(k: %s, t: %i) ", curr_key, timer);
	  curr_node = curr_node->next;
	}
      printf("NULL.\n");
    }
  return;
}




//Shift-Add-XOR hash (i found it on the internet)
uint64_t hash(key_type key)
{
  const char* p = key;
  int key_length = strlen(key);
  uint32_t h = 0;
  int i = 0;
  
  for (i; i<key_length; i++)
    {
      h ^= (h << 5) + (h >> 2) + p[i];
    }

  return h;
}
  

/*// ---((old hash function))---

{
  uint64_t ret = 1;
  uint8_t counter = 0;
  while(counter < 32) {
    if (key[counter] == '\0') return ret;
    ret *= key[counter];
    counter++;
  }
  printf("Invalid key: Key is too long or is not a string.\n");
  return 0;
  }*/



//Does a linear search of the keyspace and returns a metadata struct or NULL
meta_t get_meta(cache_t cache, key_type key)
{
  uint32_t bucket = cache->hasher(key) % cache->num_buckets;
  meta_t meta = LL_find(cache->buckets[bucket], key);
  return meta;
}

//Create a new cache object with the given capacity
//Cache size starts at 0 and new slabs are added as needed
cache_t create_cache(uint64_t maxmem, hash_func hasher)
{
  if (maxmem < get_slab_size())
    {
      printf("Cache maximum size must be at least one slab size (%d). Making a cache of that size.\n",2 * get_slab_size());
      maxmem = 2*get_slab_size();
    }
  if (maxmem > 1LU << 32)
    {
      printf("Cache maximum size may not be greater than 4GB. Making a cache of 4GB.\n");
      maxmem = 1LU << 32;
    }

  cache_t cache = malloc(sizeof(cache_obj));
  cache->hasher = (hasher == NULL) ?  &hash : hasher;
  cache->size = 0;
  cache->num_values = 0;
  cache->maxmem = maxmem;
  cache->num_buckets = 30;

  cache->buckets = calloc(cache->num_buckets,sizeof(linked_list_t));
  int i=0;
  for (i; i < cache->num_buckets; i++) cache->buckets[i] = LL_create(meta_compare, meta_destroy);
  cache->slab_manager = initialize(NULL);
  return cache;
}


//Resizes the key space to maintain a load factor between 0.25 and 1.
void resize_keys(cache_t cache)
{
  printf("resizing keys\n");
  int i;
  double load_factor = (double)cache->num_values / (double)cache->num_buckets;
  int new_num_buckets;

  //Calculate whether we want to expand, contract, or stay the same.
  //Java thinks a load factor of 0.75 is good; stackoverflow suggests ln(2). We'll just keep it between 0.25 and 1.
  if (load_factor > 1.0) new_num_buckets = 2 * cache->num_buckets;
  else if (load_factor < 0.25) new_num_buckets = cache->num_buckets / 2;
  else {printf("Decided not to resize\n"); return;}
  if (new_num_buckets < 10) {printf("Decided not to resize\n"); return;} //As the hash table gets very small we run the risk of resizing it down to 0. So we minimize it at 10.

  linked_list_t *key_space = malloc(sizeof(linked_list_t) * new_num_buckets);

  //Create the new buckets
  for (i=0; i < new_num_buckets; i++) key_space[i] = LL_create(meta_compare, meta_destroy);

  //Copy the keys over
  node *current;
  meta_t old_meta, new_meta;
  uint64_t new_location;
  for (i=0; i < cache->num_buckets; i++)
    {
      current = cache->buckets[i]->head;

      //Copy each list of metas
      while (current != NULL)
	{
	  old_meta = current->obj;
	  new_location = cache->hasher(old_meta->key) % new_num_buckets;
	  new_meta = malloc(sizeof(meta_obj));
	  memcpy(new_meta, old_meta, sizeof(meta_obj));
	  LL_append(key_space[new_location], new_meta);
	}
    }

  //Free the old keyspace
  for (i=0; i < cache->num_buckets; i++) LL_destroy(cache->buckets[i]);
  printf("done resizing\n");
}


//Allocates a new slab
//This does not resize the hash table
void expand_cache(cache_t cache, uint32_t slab_class)
{
  if (cache->maxmem - cache->size > get_slab_size())
    {
      slab_add(cache->slab_manager, slab_class);
      cache->size += get_slab_size();
      return;
    }
  else return;
}

meta_t create_meta(key_type key, uint32_t val_size)
{
  meta_t next_meta = malloc(sizeof(meta_obj));
  strcpy(next_meta->key, key);
  next_meta->size = val_size;
  next_meta->allocated = 1;
  next_meta->timer = 0;
  return next_meta;
}

void update_timers(cache_t cache)
{
  int i = 0;
  //for each bucket
  for(i; i<cache->num_buckets; i++)
    {
      linked_list_t current_LL = cache->buckets[i];
      node* curr_node = current_LL->head;
      //increment the timer of each node in the bucket
      while(curr_node != NULL)
	{
	  meta_t curr_meta = curr_node->obj;
	  uint8_t* curr_key = curr_meta->key;
	  uint8_t timer = curr_meta->timer;
	  //	  printf("(k: %s, t: %i -> ",curr_key,timer);
	  if (timer <= 255)
	    {
	      curr_meta->timer ++;
	    }
	  //	  printf("%i)\n", timer);
	  curr_node = curr_node->next;
	}
      //printf("Got out of update_timers loop.\n");
    }
  return;
}

    




//slab_class is the slab class of the incoming value
//first, try to evict from that slab class.
//if that fails, try to free space from the closest-sized slab class
//return the address of the evicted value, and ensure that **all** of the appropriate slab allocation bits are set
void *cache_evict(cache_t cache, uint32_t slab_class)
{
  uint8_t max_timer = 0;
  meta_t max_meta = NULL;
  int i = 0;
  //for each bucket
  for(i; i<cache->num_buckets; i++)
    {
      linked_list_t current_LL = cache->buckets[i];
      node* curr_node = current_LL->head;
      //increment the timer of each node in the bucket
      while(curr_node != NULL)
	{
	  meta_t curr_meta = curr_node->obj;
	  uint8_t* curr_key = curr_meta->key;
	  uint8_t timer = curr_meta->timer;
	  if (timer > max_timer && slab_class == curr_meta->slab_class)
	    {
	      max_timer = timer;
	      max_meta = curr_meta;
	    }
	  curr_node = curr_node->next;
	}
    }
  if (max_meta == NULL)
    {
      printf("No slabs of the desired class.\n");
      return NULL;
    }
  val_type max_addr = max_meta->address;
  cache_delete(cache, max_meta->key);
  return;; //max_addr;
}

//Does everything necessary to produce a free space, and returns the address of that space.
//Returns null if unable (value was bigger than maxmem perhaps).
void *free_space(cache_t cache, uint32_t slab_class)
{
  //First try to find a free space in an existing slab
  void *ret = get_address(cache->slab_manager, slab_class);
  if (ret != NULL) return ret;

  //if get_address returns null then we need to add a slab or evict.
  //first we try adding a slab
  if (cache->size < cache->maxmem)
    {
      expand_cache(cache, slab_class);
      return get_address(cache->slab_manager, slab_class);
    }
  else return NULL;
}



//Add a <key,value> pair to the cache
//If key already exists, overwrite the old value
//If maxmem capacity is exceeded, values will be removed
uint8_t cache_set(cache_t cache, key_type key, val_type val, uint32_t val_size)
{
  if (cache == NULL)
    {
      printf("Null cache\n");
      return 5;
    }
  //Check for edge cases when we don't want to go through with the set
  if (strlen(key) > 32)
    {
      printf("Key is too long.\n");
      return 1;
    }

  if (val_size > get_slab_size())
    {
      printf("Value is too large to fit in a single slab. Consider splitting the value up.\n");
      return 2;
    }

  if (val_size > cache->maxmem)
    {
      printf("Value is too large for this cache.\n");
      return 3;
    }

  if (((uint8_t*)val)[val_size - 1] != 0)
    {
      printf("That value is not a string; the last entry is not 0\n");
      return 6;
    }

  //Delete the value if it's already in the cache.
  meta_t old = get_meta(cache,key);
  if (old != NULL) cache_delete(cache, key);

  //Resize or evict if there's not enough space
  //From here on we treat the slab class as the val size
  uint32_t slab_class = get_slab_class(cache->slab_manager, val_size);
  void *address = free_space(cache, slab_class);
  if (address == NULL)
    {
      //printf("Slab class was full. Evicting.\n");
      cache_evict(cache,slab_class);
      //printf("Attempting add again\n");
      address = free_space(cache, slab_class);
      if (address == NULL)
	{
	  printf("Something went wrong. (After eviction there was still no free space)\n");
	  return 4;
	}
    } 

  //Create a new meta object and pair it to a slab address, and copy the value over
  meta_t next_meta = create_meta(key,val_size);
  next_meta->address = address;
  memcpy(next_meta->address, val, val_size);
  next_meta->slab_class = slab_class;

  //Update bucket timers for LRU eviction
  update_timers(cache);

  //Add the meta to the appropriate bucket for key hashing
  linked_list_t bucket = cache->buckets[cache->hasher(key) % cache->num_buckets];
  LL_append(bucket, next_meta);
  cache->num_values++;

  return 0;
}



//Retrieve the value associated with key, or NULL if not found
val_type cache_get(cache_t cache, key_type key, uint32_t *val_size)
{
  if (cache == NULL) return NULL;
  meta_t key_loc = get_meta(cache,key);
  //Return NULL if the value isn't in the cache
  if (key_loc == NULL)
    {
      *val_size = 0;
      return NULL;
    }

  //Increment LRU and return the address of the value
  else 
    {
      *val_size = key_loc->size;
      key_loc->timer = 0;
      assert(strlen((uint8_t*)key_loc->address)+1 == key_loc->size && "Get returned a value of the wrong length\n");
      return key_loc->address;
    }
}



//Delete an object from the cache if it's still there
uint8_t cache_delete(cache_t cache, key_type key)
{
  if (cache == NULL) return -1;
  meta_t key_loc = get_meta(cache, key);
  //Do nothing if the key wasn't found
  if (key_loc == NULL) return -1;

  //Pass the value location to the slab manager and let it find the proper slab to delete from
  man_delete(cache->slab_manager, key_loc->address, key_loc->size);
  //Find the key in the hash table and free it
  uint32_t bucket = cache->hasher(key) % cache->num_buckets;
  LL_delete(cache->buckets[bucket], key);
  cache->num_values--;
  update_timers(cache);
  return 0;
}

//Compute the total amount of memory used up by all cache values (not keys)
uint64_t cache_space_used(cache_t cache)
{
  if (cache == NULL) return 0;
  int i=0;
  uint64_t size = 0;
  for (i; i < cache->num_buckets; i++)
    {
      node *current = cache->buckets[i]->head;
      while (current != NULL)
      {
        size += ((meta_t)current->obj)->size;
	current = current->next;
      }
    }
  return size;
}


//Free all resources associated with the cache
uint8_t destroy_cache(cache_t cache)
{
  if (cache == NULL) return -1;
  //Free slabs
  man_destroy(cache->slab_manager);
  int i=0;
  //Free the hash list
  for (i; i < cache->num_buckets; i++) LL_destroy(cache->buckets[i]);
  //Free the cache object
  free(cache);
  return 0;
}



  

/*
int main()
{
  uint32_t size = 1<<21;
  cache_t c = create_cache(size, NULL);
  char* key1 = "one";
  char* key2 = "two";
  char* key3 = "three";
  char* key4 = "FOUR!";
  uint8_t* value = malloc(64);
  cache_set(c, key1, value, 64);
  cache_set(c, key2, value, 64);
  cache_set(c, key3, value, 64);
  cache_set(c, key4, value, 64);
  uint32_t test_size;
  cache_get(c, key1, &test_size);
  print_buckets(c);
  cache_evict(c,64);
  print_buckets(c);
  return 0;
}
*/
