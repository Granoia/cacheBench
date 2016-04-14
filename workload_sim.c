#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "cache.h"
#include "tools.h"
#define BILLION 1000000000
#define outputFilename "out.txt"


void sim_get(cache_t cache, uint8_t *key_ls, uint32_t set_count)
{
  printf("get\n");
  uint32_t val_size;
  uint32_t roll = rand() % (set_count * 2);  //gives a 1/2 chance to miss, should be adjusted
  sprintf((char*)key_ls, "%d", roll);
  cache_get(cache, key_ls, &val_size);
  return;
}



//ETC workload consists of the following distribution for size of set values:
//40%   2, 3, and 11 bytes
//3%    3-10 bytes
//7%    12-100 bytes
//45%   100-500 bytes
//4.8%  500-1000 bytes
//.2%   1000bytes-1MB
//This distribution is reflected here by the resulting actions from the roll (each possible value is worth .2%)
void sim_set(cache_t cache, uint32_t key_num, uint8_t *val_ls, uint8_t *key_ls)
{
  printf("set\n");
  uint32_t size;
  uint32_t roll = rand() % 500;
  sprintf((char*)key_ls, "%d", key_num);
  if (roll == 0)
    {
      size = (rand() % ((1<<20)-1010)) + 1000;     //in this case, size is a random integer between 1000 and 2^20 - 10
      val_ls[size] = 0;
      cache_set(cache, key_ls, val_ls, size+1);
      val_ls[size] = 41;
      //      printf("sim_set rolled 0. Set a value of size %i.\n",size);
      return;
    }
  else if (roll <= 25)
    {
      size = (rand() % 500) + 501;
      val_ls[size] = 0;
      cache_set(cache, key_ls, val_ls, size+1);
      val_ls[size] = 41;
      // printf("sim_set rolled %i. Set a value of size %i.\n",roll,size);
      return;
    }
  else if (roll <= 225)
    {
      size = (rand() % 400) + 101;
      val_ls[size] = 0;
      cache_set(cache, key_ls, val_ls, size+1);
      val_ls[size] = 41;
      //   printf("sim_set rolled %i. Set a value of size %i.\n",roll,size);
      return;
    }
  else if (roll <= 260)
    {
      size = (rand() % 88) + 12;
      val_ls[size] = 0;
      cache_set(cache, key_ls, val_ls, size+1);
      val_ls[size] = 41;
      //  printf("sim_set rolled %i. Set a value of size %i.\n",roll,size);
      return;
    }
  else if (roll <= 275)
    {
      size = (rand() % 6) + 4;
      val_ls[size] = 0;
      cache_set(cache, key_ls, val_ls, size+1);
      val_ls[size] = 41;
      //  printf("sim_set rolled %i. Set a value of size %i.\n",roll,size);
      return;
    }
  else
    {
      uint8_t three_sided_die = rand() % 3;
      switch (three_sided_die)
	{
	case 0:
	  size = 2;
	  break;
	case 1:
          size = 3;
	  break;
	case 2:
          size = 11;
	  break;
        default:
	  printf("What happened here?\n");
	  return;
	}
      val_ls[size] = 0;
      cache_set(cache, key_ls, val_ls, size+1);
      val_ls[size] = 41;
      //   printf("sim_set rolled %i. Set a value of size %i.\n",roll,size);
      return;
    }
}	    





//paper observes a 30:1 get:set ratio in the ETC workload, which is reflected here by the resulting actions from the roll
void simulate(cache_t cache, uint32_t iterations, uint8_t *val_ls, uint8_t *key_ls)
{
  uint32_t i;
  uint32_t set_count = 1;
  sim_set(cache,0,val_ls,key_ls);
  for(i=1; i<iterations; i++)
    {
      uint8_t roll = rand() % 31;
      if (!roll) {
	set_count++;
	sim_set(cache, set_count, val_ls, key_ls);
      }
      else {sim_get(cache, key_ls, set_count);}
    }
  return;
}





//main will need to preallocate a key list and a value list as an initialization step so that this is not factored into the timer for the cache
//value list should be 1MB long, and thus cache_sets can quickly change the desired character to the null terminator and submit the appropriate size parameter


int main(int argc, char** argv)
{
  if (argc < 3)
    {
      printf("workload_sim: <server address> <cache_size (MB)> <iterations (2^n)>\n");
      return -1;
    }
  server_addr = argv[1];
  int cache_size = atoi(argv[2]);
  int iterations = (1 << 12) + (1 << atoi(argv[3]));
  srand(iterations);


  //data initialization  
  uint8_t *val_ls = malloc(1<<20);
  memset(val_ls, 41, 1<<20);
  uint8_t *key_ls = malloc(20);   //this is just a short buffer where the iteration number can be converted into a string using itoa() i mean sprintf()

  uint64_t maxmem = (1<<20)*cache_size;
  cache_t test_cache = create_cache(maxmem, NULL);

  struct timespec start,end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  
  simulate(test_cache, iterations, val_ls, key_ls);

  clock_gettime(CLOCK_MONOTONIC, &end);


  double elapsed = (end.tv_sec * BILLION + end.tv_nsec) - (start.tv_sec * BILLION + start.tv_nsec);
  double avg = elapsed / (double)(iterations);

  FILE *fileout = fopen(outputFilename, "a");
  fprintf(fileout, "%f\n",avg);
  fclose(fileout);
  printf("Average time per operation was %f\n",avg);
  return 0;
}
