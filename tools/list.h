#include <inttypes.h>

//Equality function between objects in the list
typedef uint8_t (*eq_func) (const void *obj1, const void *obj2);

//Frees all resources associated to an object in the list
typedef void (*destroy_func) (const void *obj);

typedef struct node{
  void *obj;
  struct node *next;
} node;

struct linked_list{
  node* head;
  eq_func eq;
  destroy_func destroy;
};

typedef struct linked_list* linked_list_t;

linked_list_t LL_create(eq_func, destroy_func);

void LL_append(linked_list_t LL, const void *obj);

void LL_delete(linked_list_t LL, const void *obj);

void* LL_find(linked_list_t LL, const void *obj);

void LL_destroy(linked_list_t LL);
