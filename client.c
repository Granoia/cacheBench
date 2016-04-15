#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "nanomsg/src/nn.h"
#include "nanomsg/src/reqrep.h"
#include "cache.h"
#include <stdio.h>
#include <unistd.h>
#include "jsmn/jsmn.h"

#define buffer_size (1048576) //1 << 20 (1 MB)

struct cache_obj
{
  int sock;
  //Buffer_size should equal the buffer size or messages may be truncated
  uint8_t buffer[buffer_size];
};


//Receives from the server
//Nonblocking; returns 0 if nothing was received
int cache_recv(cache_t cache)
{
  int result = nn_recv (cache->sock, cache->buffer, buffer_size, NN_DONTWAIT);
  if (result < 0 && errno == EAGAIN) return 0;
  if (result < 0)
    {
      switch (errno)
	{
	case EBADF: printf("EBADF\n"); return 0;
	case ENOTSUP: printf("ENOTSUP\n"); return 0;
	case EFSM: printf("EFSM\n"); return 0;
	case EINTR: printf("EINTR\n"); return 0;
	case ETIMEDOUT: printf("ETIMEDOUT\n"); return 0;
	case ETERM: printf("ETERM\n"); return 0;
	}
    }
  return 1;
}

//Sends a message to the server
int cache_send(cache_t cache,const char* msg)
{
  int msg_len = strlen(msg)+1;
  assert(nn_send(cache->sock, msg, msg_len, 0) > 0 && "Client: Send failed");
  return 1;
}

  //Send the GET request
val_type cache_get(cache_t cache, key_type key, uint32_t *size)
{
  sprintf(cache->buffer, "GET /%s",key);
  cache_send(cache, cache->buffer);
  return NULL;
}

//Takes a key and sends a value and size.
uint8_t cache_set(cache_t cache,key_type key,val_type value, uint32_t size)
{
  if (((uint8_t*)value)[size-1] != 0)
    {
      printf("Client: Value was not a string. Aborting.\n");
      assert(0);
    }

  //msg = PUT /k/v
  sprintf(cache->buffer, "PUT /%s/%s",(char*) key,(char*) value);

  //Send the message
  cache_send(cache, cache->buffer);
  return 0;
}


//Binds a socket on the server end and begins listening
cache_t create_cache (uint64_t maxmem, hash_func hash)
{
  cache_t cache = malloc(sizeof(struct cache_obj));

  cache->sock = nn_socket (AF_SP, NN_REQ); 
  assert (cache->sock >= 0 && "Client create socked failed");
  assert (nn_connect (cache->sock, server_addr) >= 0 && "Client connect failed");

  if (maxmem > 0) {
  sprintf(cache->buffer, "POST /memsize/%lu",maxmem);
  cache_send(cache,cache->buffer);
  }
  return cache;
}
