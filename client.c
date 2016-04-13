#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "nanomsg/src/nn.h"
#include "nanomsg/src/reqrep.h"
#include "cache.h"
#include "tools.h"
#include <stdio.h>
#include <unistd.h>
#include "jsmn/jsmn.h"

struct cache_obj
{
  int sock;
  //Buffer_size should equal the buffer size or messages may be truncated
  uint32_t buffer_size;
  uint8_t buffer[1<<22];
};

//Searches an HTTP reply for a field.
//Pass NULL for field_name to return the body of the reply
uint8_t *parse_http_field(uint8_t *http_reply, uint8_t *field_name)
{
  uint8_t len = (field_name == NULL) ? 0 : strlen(field_name);
  uint8_t *head = http_reply;
  int counter=0;
  while (field_name == NULL || strncmp(head, field_name, len))
    {
      //If the first line is a newline, we've reached the end of the header.
      if (!strncmp(head, "\r\n",2)) break;
      //else go to next line
      while (strncmp(head, "\r\n",2))
	{
	  head += 1;
	}
      head += 2;
    }
  //If we got the end and we're searching for body, return the body
    if (!strncmp(head,"\r\n",2) && field_name == NULL) return head + 2;
    //If we got to the correct field, return it
    else if (!strncmp(head, field_name,len)) return head;
    //Else return null
    else return NULL;
}

//Receives an html code from the server
void cache_recv(cache_t cache)
{
  uint8_t *buf = NULL;
  int result = nn_recv (cache->sock, &buf, NN_MSG, 0);
  if (result < 1) {
    return;
  }
  char code[1];
  sscanf(buf, "%*s %c",code);
  if (code[0] != '2')
    printf("Client: Received %s\n",buf);
  nn_freemsg (buf);
}

//Helper function for cache_get; receives the reply and passes the body to cache_get
char *cache_get_receive(cache_t cache)
{
  uint8_t *buf = cache->buffer;
  int result = nn_recv(cache->sock, cache->buffer, cache->buffer_size, 0);
  if (result < 1)
    {
      printf(KRED "get_receive got nothing\n" RESET);
      return NULL;
    }
  uint8_t code[4]; code[3] == 0;
  sscanf(buf, "%*s %3s", code);
  //If the code indicates success, return the body
  if (!strncmp(code,"200",3))
    {
      return parse_http_field(buf,NULL);
    }
    else if (!strncmp(code,"204",3))
      {
	return NULL;
      }
  else {
    printf(KRED"Error:\n"RESET"%s\n",buf);
    return NULL;
  }
}

int cache_send(cache_t cache,const char* msg)
{
  int msg_len = strlen(msg)+1;
  assert(nn_send(cache->sock, msg, msg_len, 0) > 0);
  return 1;
}

val_type cache_get(cache_t cache, key_type key, uint32_t *size)
{
  //Send the GET request
  char msg[strlen(key) + 32];
  sprintf(msg, "GET /%s",key);
  cache_send(cache, msg);

  //Receive and parse a reply
  char *ret = cache_get_receive(cache); //JSON tuple { key: k, value: v }
  if (ret == NULL) return NULL;
  jsmn_parser parser;
  jsmntok_t tokens[5];
  jsmn_init(&parser);
  int num_tokens = jsmn_parse(&parser, ret, strlen(ret), tokens, 6);
  char *recv_key = ret + tokens[2].start;
  ret[tokens[2].end] = 0;
  char *val = ret + tokens[4].start;
  ret[tokens[4].end] = 0;

  //Make sure the key is correct, and return the value
  if (strcmp(key, recv_key) == 0) 
    {
      *size = strlen(val)+1; //Include the null terminator so that set/get have the same size
      return val;
    }

  //If the key is incorrect, report an error and return null
  else {
	printf(KRED "cache_get returned a value with the wrong key: %s\n"RESET, recv_key);
	nn_freemsg(ret);
	return NULL;
       }
}

//Takes a key and sends a value and size.
//If the value is null, sends 0 bytes
uint8_t cache_set(cache_t cache,key_type key,val_type value, uint32_t size)
{
  char msg[size + strlen(key) + 32];
  if (((uint8_t*)value)[size-1] != 0)
    {
      printf("Value was not a string. Setting the last character of value to null.\n");
      ((uint8_t*)value)[size-1] = 0;
    }

  //msg = PUT /k/v
  sprintf(msg, "PUT /%s/%s",(char*) key,(char*) value);

  //Send the message
  cache_send(cache, msg);
  cache_recv(cache);
  return 0;
}

uint8_t cache_delete(cache_t cache, key_type key)
{
  char msg[strlen(key) + 32];
  sprintf(msg,"DELETE /%s",key);
  cache_send(cache, msg);
  cache_recv(cache);

  return 0;
}




//Binds a socket on the server end and begins listening
cache_t create_cache (uint64_t maxmem, hash_func hash)
{
  cache_t cache = malloc(sizeof(struct cache_obj));

  //Make sure this complies with actual buffer size in struct cache_obj
  cache->buffer_size = 1<<22;

  cache->sock = nn_socket (AF_SP, NN_REQ);
  assert (cache->sock >= 0);
  printf("Client: Attempting to connect to %s...\n",server_addr);
  assert (nn_connect (cache->sock, server_addr) >= 0);
  char msg[64];
  sprintf(msg, "POST /memsize/%lu",maxmem);
  cache_send(cache,msg);
  cache_recv(cache);
  return cache;
}

uint8_t destroy_cache(cache_t cache)
{
  cache_send(cache, "POST /shutdown");
  nn_shutdown(cache->sock, 0);
  free(cache);
  return 0;
}


void make_address(char *url, char *server_port)
  {
    server_addr = calloc(8,32);
    sprintf(server_addr,"tcp://%s:%s",url,server_port);
    printf("address is %s\n",server_addr);
  }



uint64_t cache_space_used(cache_t cache)
{
  uint8_t *buf = NULL;
  cache_send(cache, "HEAD /k");
  int result = nn_recv(cache->sock, &buf, NN_MSG, 0);
  if (result < 1)
    {
      printf(KRED "HEAD got nothing\n" RESET);
      return 0;
    }
  uint8_t *html = buf;
  strtok(html, "\r\n");
  while (strncmp(html, "cache",4))
    {
      if (*html == '\r') {
	printf("Didn't find a cache-space-used entry\n");
	return 0;
      }
      html = strtok(NULL, "\r\n");
    }
  strtok(html, " ");
  html = strtok(NULL, " ");
  uint64_t ret = atoi(html);
  nn_freemsg(buf);
  return ret;
}


