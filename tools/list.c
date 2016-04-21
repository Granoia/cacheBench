#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include "list.h"



//Creates a new linked list
//The equality function is a boolean equivalency function for the object type of the list
linked_list_t LL_create(eq_func eq, destroy_func destroy){
  linked_list_t lst = malloc(sizeof(struct linked_list));
  lst->head = NULL;
  lst->eq = eq;
  lst->destroy = destroy;
  return lst;
}


//appends a node with the given key to the end of the given LL
void LL_append(linked_list_t LL, const void *obj_tmp)
{
  void *obj = (void*)obj_tmp;
  node* new = malloc(sizeof(struct node));
  new->next = NULL;
  new->obj = obj;

  //traverses linearly to the end of the LL
  node* current = LL->head;
  node* prev = NULL;
  while (current != NULL)
  {
    prev = current;
    current = current->next;
  }
  if (prev == NULL) LL->head = new;
  else prev->next = new;
  return;
}

//Returns the first object in the list that eqs the given object
void* LL_find(linked_list_t LL, const void *obj)
{
  node* current = LL->head;
  while (current != NULL)
    {
      if (LL->eq(current->obj, obj)) return current->obj;
      current = current->next;
    }
  return NULL;
}



//deletes a node with the given obj from the given LL
void LL_delete(linked_list_t LL, const void *obj){

    node* current = LL->head;

    if (current == NULL) return;

    //covers case where the root is the node to be deleted
    if (LL->eq(current->obj, obj))
      {
	LL->head = current->next;
	LL->destroy(current->obj);
        free(current);
        return;
      }

    node* previous = current;

    //covers general case by linear searching through the LL until we hit the node with the given key and deleting it 
    while (current != NULL)
    {
      previous = current;
      current = current->next;
      if (current == NULL) return;
      if (LL->eq(current->obj, obj))
      {
	previous->next = current->next;
	LL->destroy(current->obj);
	free(current);
	return;
      }
    }

    printf("Not sure how you got down here.\n");
    return;
}


void LL_destroy(linked_list_t LL)
{
  node *current = LL->head;
  while(current != NULL)
  {
    node *next = current->next;
    LL->destroy(current->obj);
    free(current);
    current = next;
  }
  free(LL);
}
