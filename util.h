#ifndef _UTIL_
#define _UTIL_

typedef struct list_t {
  struct list_t *next;
  struct list_t *end;
  void* data;
} list_t;


list_t *new_list();

void list_destroy(list_t *list);

int list_empty(list_t *head);

int list_append(list_t *head, void *data);

void *list_remove(list_t *head);

#endif /* _UTIL_ */
