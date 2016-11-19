#include <stdlib.h>
#include "util.h"
#include "blacklist.h"

void sfree(void *ptr)
{
  free (ptr);
  return ;
}
void *smalloc(int size)
{
  void *ret = malloc(size);
  if (ret == NULL) {
    assert(0);
  }
  memset(ret, 0, size);
  return ret;
}

void *smalloc_msg(int size, char* str, int id)
{
  void *ret = malloc(size);
  if (ret == NULL) {
    printf("%s %d", str, id);
    assert(0);
  }
  memset(ret, 0, size);
  return ret;
}


int init_blacklist(char *blacklist)
{
  blacklist_init(NULL, blacklist, NULL, 0, NULL, 0, 0);
  return EXIT_SUCCESS;
}

pthread_mutex_t *new_mutex()
{
  pthread_mutex_t *lock = malloc(sizeof(pthread_cond_t));
  if (lock == NULL) {
    printf("Failed to allocate lock.\n");
    exit(-1);
  }
  pthread_mutex_init(lock, NULL);
  return lock;
}

pthread_cond_t *new_cond()
{
  pthread_cond_t *cond = malloc(sizeof(pthread_cond_t));
  if (cond == NULL) {
    printf("Failed to allocate condition variable.\n");
    exit(-1);
  }
  pthread_cond_init(cond, NULL);
  return cond;
}
