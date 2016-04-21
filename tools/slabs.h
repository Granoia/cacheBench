#pragma once

#include <stdint.h>

struct manager_obj;
typedef struct manager_obj* manager_t;

void *get_address(manager_t man, uint32_t val_size);

//Finds out which slab val_ptr is in, and deallocates value from that slab
//Accounts for values that take up multiple slab slots
void man_delete(manager_t man, uint8_t *val_ptr, uint32_t size);


//Returns which slab class a value of size <size> would fit in most tightly.
//If no slab class fits, returns the largest slab class.
uint32_t get_slab_class(manager_t man, uint32_t size);


//Creates a new slab manager. slab_sizes is the list of possible slab sizes; NULL defaults to powers of 2.
manager_t initialize(uint32_t *slab_sizes);


//Frees all resources related  tothe manager
void man_destroy(manager_t man);


//Adds a blank slab to the list of slabs.
void slab_add(manager_t m, uint32_t slab_class);

void print_slabs(manager_t man);

uint32_t get_slab_size();
