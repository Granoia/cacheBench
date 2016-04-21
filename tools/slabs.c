#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "tools.h"
#include "slabs.h"
#include "list.h"

typedef struct slab{
  uint8_t memory[1<<20]; //1 MB slabs
  uint32_t class;
  uint32_t status[512]; //16384 bits of allocation status
  uint8_t isFull;
} slab_obj;

typedef slab_obj* slab_t;

uint32_t get_slab_size()
{
  return sizeof(slab_obj);
}

//Destroy function for the linked list
void destroy_slab(const void *obj)
{
  free((slab_t)obj);
}


struct manager_obj{
  uint8_t full;
  uint32_t* classes;
  linked_list_t slabs;
  uint32_t num_slabs;
};


void bit_string(uint32_t bits)
{
  int i, j;
  for (i=31; i >= 0; i--)
    {
      j = (bits & (1<<i)) != 0;
      printf("%d",j);
    }
}

void toggle_bit(uint32_t *status, uint32_t bit)
{
  uint32_t *chunk = status + (bit / 32);
  uint32_t index = bit % 32;
  chunk[0] ^= (0x80000000 >> index);
  //  printf("done\n");
}


//Sets an entry's allocation status to 0 by xoring it
void slab_delete(slab_t s, uint32_t item)
{
  toggle_bit(s->status, item);
  s->isFull = 0;
}

void man_destroy(manager_t man)
{
  LL_destroy(man->slabs);
  free(man->classes);
  free(man);
}

//Returns which slab class a value of size <size> would fit in most tightly
uint32_t get_slab_class(manager_t man, uint32_t size)
{
  uint32_t *classes = man->classes;
  int i=0;
  while (classes[i] != 0) 
  {
    if (classes[i] >= size) return classes[i];
    i++;
  }
  return classes[i-1];
}

//Returns 0 if there are no free indices; or (index+1) of the first free index.
uint32_t get_free_index(struct slab *s)
{
  uint32_t i,j;
  for (i=0; i < 512; i++)
  {
    if (s->status[i] == ~0) 
      {
	continue; //status is full of 1's
      }
    else if (s->status[i] == 0) 
    {
      return 32*i + 1; //status is full of 0's
    }
    if (s->status[i] == ~0) return 0;
    j =  __builtin_clz(~s->status[i]); //get #leading 1's
    return 32*i+j + 1;
  } 
  return 0;
}

slab_t allocate_slab(uint32_t slab_class)
{
  slab_t s = malloc(sizeof(slab_obj));
  s->class = slab_class;
  int i;
  //Set the relevant allocation bits to 0	  
  for (i=0; i < (1<<20) / slab_class / 32; i++)
  {
     s->status[i] = 0;
  }
  int remainder = (1<<20) - i*32*slab_class; //the remaining space
  remainder /= slab_class;  //The number of entries left (individual allocation bits)
  if (remainder > 31) {
    printf("Something happened in the slab allocation calculation\n");
    exit(-1);
  }
  int j;
  for (j=remainder; j < 32; j++) s->status[i] |= 1 << (31-j);
  //Set the irrelevant allocations bits 1; they can never be changed
  for (i++; i < 512; i++)
  {
    s->status[i] = ~0;
  }
  s->isFull = 0;
  return s;
}


void slab_add(manager_t man, uint32_t slab_class)
{
  LL_append(man->slabs, allocate_slab(slab_class));
  man->num_slabs++;
}


//Returns a slab with free entries or NULL if there is none
slab_t get_slab(manager_t man, uint32_t slab_class)
{
  node *current = man->slabs->head;
  slab_t slab;
  while (current != NULL)
    {
      slab = current->obj;
      if (slab->class == slab_class && !slab->isFull)
      {
	return slab;
      }
      current = current->next;
    }
  return NULL;
}


//Find function for the linked list
uint8_t find_val(const void *obj1, const void *obj2)
{
  slab_t s = (slab_t)obj1;
  uint8_t *val_ptr = (uint8_t*)obj2;
  uint8_t *slab_add = (uint8_t*)s;
  if (slab_add <= val_ptr && slab_add + sizeof(slab_obj) > val_ptr)
    {
      return 1;
    }
  return 0;
}





//Deletes a value from its slab
void man_delete(manager_t man, uint8_t *val_ptr, uint32_t size)
{
  //The find function searches for a slab containing the val address
  slab_t to_adjust = LL_find(man->slabs, val_ptr);
  //failed to find the slab
  if (to_adjust == NULL) 
    {
      printf("man_delete failed to find a slab containing that address\n");
      return;
    }
  //Find the exact index of the value
  uint32_t i = (val_ptr - to_adjust->memory); //This is how many bytes are between 0th entry and val
  i /= to_adjust->class;                      //This is the index of value

  //Find out whether the value takes up multiple index slots
  //uint8_t num_slots = 1;
  //while (to_adjust->class * num_slots < size) num_slots++;
  slab_delete(to_adjust, i);

  //  for (num_slots; num_slots > 0; num_slots--)
  //  {
  //    slab_delete(to_adjust, i+num_slots-1);
  //  }
}

//Returns an available address for val_size, and sets that address to Allocated.
void *get_address(manager_t man, uint32_t val_size)
{
  uint32_t slab_class = get_slab_class(man, val_size);
  slab_t s = get_slab(man, slab_class); //Returns a slab with free entries or NULL
  if (s == NULL) 
    {
      return NULL;
    }
  uint32_t i = get_free_index(s);
  if (i == 0) {
    s->isFull = 1;
    return get_address(man, val_size);
  }
  toggle_bit(s->status, i-1);
  return s->memory + s->class * (i-1);
}



//initializes a slab manager and its slabs. The number of classes and how much each class holds is currently hardcoded, but given our implementation it is easy to change.
manager_t initialize(uint32_t *classes){
    manager_t ret = malloc(sizeof(struct manager_obj));
    ret->full = 0;
    if (classes == NULL)
    {
       ret->classes = malloc(16 * 32);

       //makes slab classes from 64 bytes to 1MB up by powers of 2 each time    
       int i = 6;
       for(i; i<=20; i++){
         ret->classes[i-6] = 1 << i;
       }
       ret->classes[15] = 0;
    }
    else ret->classes = classes;

    ret->num_slabs = 0;

    //allocates the array is the slabs, holds the values
    ret->slabs = LL_create(find_val, destroy_slab);
    return ret;
}


void print_slabs(manager_t man)
{
  printf("Man has %u slabs:\n",man->num_slabs);
  node *current = man->slabs->head;
  slab_t slab;
  while (current != NULL)
    {
      slab = current->obj;
      printf("Class %d, status ",slab->class);
      bit_string(slab->status[0]);
      printf(" ... ");
      bit_string(slab->status[511]);
      printf("\n");

      current = current->next;
    }
}
