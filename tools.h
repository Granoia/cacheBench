#pragma once

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define RESET "\033[0m"



typedef struct meta_obj
{
  uint8_t key[32];
  uint8_t* address;
  uint32_t size;
  uint8_t allocated;
  uint8_t timer;
  uint32_t slab_class;
} meta_obj;

typedef meta_obj* meta_t;

void meta_destroy(const void *obj);

uint8_t meta_compare(const void *obj1, const void *obj2);
