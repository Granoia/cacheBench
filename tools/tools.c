#include <stdlib.h>
#include <inttypes.h>
#include "tools.h"


void meta_destroy(const void *obj)
{
  meta_t m = (meta_t)obj;
  free(m);
}

uint8_t meta_compare(const void *obj1, const void *obj2)
{

  meta_t m = (meta_t)obj1;
  uint8_t *k = (uint8_t*) obj2;
  return strcmp(m->key, k) == 0;

}
