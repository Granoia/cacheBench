#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "cache.h"
#include "tools.h"




void sim_get(cache_t cache, uint8_t *key_ls, uint32_t current_iteration)
{
  uint32_t *val_size;
  uint32_t roll = rand() % (current_iteration * 2);  //gives a 1/2 chance to miss, can be adjusted depending on what the hit/miss ratio reported in the paper is but I haven't looked it up yet
  char *key = itoa(roll, key_ls, 2);
  cache_get(cache, key, &val_size);
  //printf("Running sim_get.\n");
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
  uint32_t roll = rand() % 500;
  char *key = itoa(key_num, key_ls, 2    //makes the iteration number into a string, which is then used as the key
  if (roll == 0)
    {
      uint32_t size = (rand() % ((1<<20)-1010)) + 1000     //in this case, size is a random integer between 1000 and 2^20 - 10
      val_ls[size] = 0;
      cache_set(cache, key_ls, val_ls, size);
      val_ls[size] = 41;
      //printf("sim_set rolled 0. Set a value of size %i.\n",size);
      return;
    }
  else if (roll <= 25)
    {
      uint32_t size = (rand() % 500) + 501;
      val_ls[size] = 0;
      cache_set(cache, key_ls, val_ls, size);
      val_ls[size] = 41;
      //printf("sim_set rolled %i. Set a value of size %i.\n",roll,size);
      return;
    }
  else if (roll <= 225)
    {
      uint32_t size = (rand() % 400) + 101;
      val_ls[size] = 0;
      cache_set(cache, key_ls, val_ls, size);
      val_ls[size] = 41;
      //printf("sim_set rolled %i. Set a value of size %i.\n",roll,size);
      return;
    }
  else if (roll <= 260)
    {
      uint32_t size = (rand() % 88) + 12;
      val_ls[size] = 0;
      cache_set(cache, key_ls, val_ls, size);
      val_ls[size] = 41;
      //printf("sim_set rolled %i. Set a value of size %i.\n",roll,size);
      return;
    }
  else if (roll <= 275)
    {
      uint32_t size = (rand() % 6) + 4;
      val_ls[size] = 0;
      cache_set(cache, key_ls, val_ls, size);
      val_ls[size] = 41;
      //printf("sim_set rolled %i. Set a value of size %i.\n",roll,size);
      return;
    }
  else
    {
      uint8_t three_sided_die = rand() % 3;
      if (three_sided_die = 0) {uint32_t size = 2}
      if (three_sided_die = 1) {uint32_t size = 3}
      if (three_sided_die = 2) {uint32_t size = 11}
      val_ls[size] = 0;
      cache_set(cache, key_ls, val_ls, size);
      val_ls[size] = 41;
      //printf("sim_set rolled %i. Set a value of size %i.\n",roll,size);
      return;
    }
}	    





//paper observes a 30:1 get:set ratio in the ETC workload, which is reflected here by the resulting actions from the roll
void simulate(cache_t cache, uint32_t iterations, uint8_t *val_ls, uint8_t *key_ls)
{
  uint32_t i = 0;
  for(i; i<iterations; i++)
    {
      uint8_t roll = rand() % 31;
      if (roll == 0) {sim_set(cache, i, val_ls, key_ls);}
      else {sim_get(cache, key_ls, i);}
    }
  return;
}





//main will need to preallocate a key list and a value list as an initialization step so that this is not factored into the timer for the cache
//value list should be 1MB long, and thus cache_sets can quickly change the desired character to the null terminator and submit the appropriate size parameter


int main(int argc, char** argv)
{
  if (argc < 3)
    {
      printf("workload_sim: <cache_size (MB)> <iterations (2^n)>\n");
      return -1;
    }
  
  int cache_size = atoi(argv[1])
  int iterations = 1 << atoi(argv[2]);

  srand(iterations);


  //data initialization  
  uint8_t *val_ls = malloc(1<<20);
  memset(val_ls, 41, 1<<20);
  uint8_t *key_ls = malloc(20);   //this is just a short buffer where the iteration number can be converted into a string using itoa()

  uint64_t maxmem = (1<<20)*cache_size;
  cache_t test_cache = create_cache(maxmem, NULL);

  struct timespec start,end;
  
  clock_gettime(CLOCK_MONOTONIC, &start);
  
  simulate(test_cache, iterations, val_ls, key_ls);

  clock_gettime(CLOCK_MONOTONIC, &end);


  float elapsed = (end.tv_sec * BILLION + end.tv_nsec) - (start.tv_sec * BILLION + start.tv_nsec);
  float avg = elapsed / (double)(iterations);

  fileout = fopen(outputFilename, "a");
  fprintf(fileout, "%f\n",avg);
  fclose(fileout);
  printf("Average time per operation was %f\n",avg);
  return 0;
}
