#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "nanomsg/src/nn.h"
#include "nanomsg/src/pair.h"
#include "cache.h"
#include <stdio.h>
#include <unistd.h>
#include "jsmn/jsmn.h"

//Requests are parsed in the following way:
//GET /k
//PUT /k/v
//DELETE /k
//HEAD /k
//POST /shutdown
//POST /memsize/value


cache_t global_cache = NULL;
int global_sock;
uint8_t cache_has_been_set = 0;

struct request_obj
{
  char *buf; //The original buffer received
  uint8_t request_type;  //enum for request type
  char key[200]; //The location of the start of key str in buf
  char *value; //The location of the start of val str in buf
  uint64_t size; //The size of the request
};

typedef struct request_obj *request_t;


//Sends a message back to the client
void cache_reply(request_t req, char *head, char *body)
{
  int msg_len = strlen(head) + strlen(body) + 256;
  char msg[msg_len];
  sprintf(msg,  "HTTP/1.1 %s\r\n"
	        "Content-Type: application/json\r\n"
	        "Date: Date goes here\r\n"
	        "cache-space-used: %lu\r\n"
	        "\r\n"
	        "%s\n",

          head,
	  cache_space_used(global_cache),
	  body
	  );

  nn_send(global_sock, msg, msg_len, 0);
}

//Takes a key and sends { key: k, value: v }
//If the value is null, sends an error
void take_get_request(request_t req)
{
  val_type value = cache_get(global_cache, req->key, (uint32_t*)&req->size);

  if (value != NULL)
    {
     char msg[req->size + strlen(req->key) + 32];
     sprintf(msg, "{ \"key\" : \"%s\", \"value\" : \"%s\" }",req->key,(char*)value);
     cache_reply(req, "200 OK", msg);

    }

  //Send error if null
  else {
    cache_reply(req, "204 No Content", "Value not found in cache."); //Resource not found
  }
}

//Returns just the header
void take_head_request(request_t req)
{ 
  cache_reply(req,"200 OK", "");
}


//Handles a request object by calling the relevant cache operation and responding to the client
void handle_request(request_t req)
{
  uint8_t failure;
  switch (req->request_type)
    {
    case 4:
      //POST /memsize/value
      if (strcmp(req->key, "memsize") == 0)
	{
	  if (!cache_has_been_set) 
	    {
	      destroy_cache(global_cache);
	      global_cache = create_cache(atoi(req->value),NULL);
              cache_reply(req, "201 Created",""); //Created
	    }
	  else 
	    {
              cache_reply(req, "500 Internal Server Error","Cache was not created because \"cache_has_been_set\" was true. Make sure you've destroyed all previous caches\n");
	    }
	}


      //POST /shutdown
      else if (strcmp(req->key, "shutdown") == 0)
	{
	  failure = destroy_cache(global_cache);
	  global_cache = NULL;
	  cache_has_been_set = 0;
	  if (!failure) cache_reply(req, "204 No Content","");
	  else {
	    cache_reply(req, "500 Internal Server Error", "Shutdown failed; cache was not destroyed. Aborting server.\n");
	    exit(-1);
	  }
	}
      else cache_reply(req, "400 Bad Request","Usage: POST /shutdown || POST /memsize/value\n");
      return;

    case 1:
      //DELETE /k
      failure = cache_delete(global_cache, req->key);
      if (!failure) cache_reply(req, "204 No Content","");
      else 
	{
          char err[strlen(req->key)+64];
	  sprintf(err, "Delete of key %s failed for some reason.\n",req->key);
          cache_reply(req, "500 Internal Server Error",err);
	}
      return;

    case 2:
      //GET /k
      take_get_request(req);
      return;

    case 5:
      //PUT /k/v
      failure = cache_set(global_cache, req->key, req->value, req->size);
      if (!failure) 
	{
	  cache_has_been_set = 1;
          cache_reply(req, "204 No Content","");
	}
      else cache_reply(req, "500 Internal Server Error","");
      return;

    case 3:
      //HEAD /k
      //Return a header with HTTP version, Date, Accept, Cache Space Used
      take_head_request(req);
      return;
     
    default:
      cache_reply(req,"400 Bad Request","");
      return;
    }

}

//Fills in a request object by parsing the HTTP string
request_t parse_request(request_t req, uint8_t *buf)
{
  char request_string[16];
  //Scans the buffer for three strings separated by space or /

  sscanf(buf,"%s /%[^/]/",request_string, req->key);
  req->value = buf + strlen(request_string) + strlen(req->key) + 3;
  req->size = strlen(req->value)+1;
  //printf("%s scanned into %s  %s  %s\n",buf,request_string,req->key,req->value);
  if (strcmp(request_string, "DELETE") == 0) req->request_type = 1;
  else  if (strcmp(request_string, "GET") == 0) req->request_type = 2;
  else if (strcmp(request_string, "HEAD") == 0) req->request_type = 3;
  else if (strcmp(request_string, "POST") == 0) req->request_type = 4;
  else if (strcmp(request_string, "PUT") == 0) req->request_type = 5;
  else req->request_type = 0; //bad request

  return req;
}


//Receives a complete request from the client
void recv(int sock)
{
  uint8_t *buf = NULL;
  int result = nn_recv (sock, &buf, NN_MSG, 0);
  if (result < 0)
    {
      switch (errno)
	{
	case EBADF: printf("EBADF\n"); return;
	case ENOTSUP: printf("ENOTSUP\n"); return;
	case EFSM: printf("EFSM\n"); return;
	case EAGAIN: printf("EGAGAIN\n"); return;
	case EINTR: printf("EINTR\n"); return;
	case ETIMEDOUT: return;//printf("ETIMEDOUT\n"); return;
	case ETERM: printf("ETERM\n"); return;
	default: printf("what\n"); return;
	}

    }

  //Initialize a request object
  struct request_obj r;
  request_t request = &r;
  parse_request(request, buf);

  //Handle the request
  handle_request(request);

  nn_freemsg (buf);
}


//Continuously receives requests and passes them on for processing
int send_recv(const char *url)
{
  int sock = nn_socket (AF_SP, NN_PAIR);
  assert (sock >= 0);
  assert(nn_bind (sock, url) >= 0);
  int to = 100;
  assert (nn_setsockopt (sock, NN_SOL_SOCKET, NN_RCVTIMEO, &to, sizeof (to)) >= 0);
  while(1)
    {
      recv(sock);
    }
  return nn_shutdown(sock, 0);
}


//./server -t <port> -m <memory>
//Default port: 8075
//Default maxmem: 67 MB
int main (const int argc, const char **argv)
{
    char addr[64];
    memset(addr,0,64);
    strcat(addr,"tcp://0.0.0.0:");
    char *port = NULL;
    uint64_t maxmem = 0;
    int i=0;
    for (i; i < argc; i++)
      {
	if (!strcmp(argv[i],"-t"))
	  {
	    port = (char*)argv[i+1];
	    if (strlen(port) > 5) {
	      printf("Usage: %s (-t port) (-m maxmem)\n",argv[0]);
	      return -1;
	    }
	    continue;
	  }
	else if (!strcmp(argv[i], "-m"))
	  {
	    maxmem = atoi(argv[i+1]);
	    continue;
	  }
      }
    if (port == NULL) port = "50000";
    if (maxmem == 0) maxmem = 1<<26;
    strcat(addr, port);
    global_cache = create_cache(maxmem, NULL);
    printf("Starting server at %s\n",addr);
    return send_recv(addr);
}
