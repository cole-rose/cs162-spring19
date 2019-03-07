#include <stdlib.h>
#include "wq.h"
#include "utlist.h"

/* Initializes a work queue WQ. */
void wq_init(wq_t *wq) {

<<<<<<< HEAD
  pthread_mutex_init(&(wq->lock), NULL);
  pthread_cond_init(&(wq->cv), NULL);

  /* TODO: Make me thread-safe! */
  wq->size = 0;
  printf("wq->size: %d\n", wq->size);
=======
  /* TODO: Make me thread-safe! */

  wq->size = 0;
>>>>>>> 21ae9e5292d8f10d81cd3e34aaaa0e4725b899a0
  wq->head = NULL;
}

/* Remove an item from the WQ. This function should block until there
 * is at least one item on the queue. */
int wq_pop(wq_t *wq) {

  /* TODO: Make me blocking and thread-safe! */

<<<<<<< HEAD
  pthread_mutex_lock(&(wq->lock));

// while queue is empty
 while (wq->size == 0){
     printf("size is 0, wait\n");
     pthread_cond_wait(&(wq->cv), &(wq->lock));
 }

  wq_item_t *wq_item = wq->head;
  if (wq_item == NULL){
      printf("wq_head is NULL. DAMN IT.\n");
//      return 1;
  }
=======
  wq_item_t *wq_item = wq->head;
>>>>>>> 21ae9e5292d8f10d81cd3e34aaaa0e4725b899a0
  int client_socket_fd = wq->head->client_socket_fd;
  wq->size--;
  DL_DELETE(wq->head, wq->head);

<<<<<<< HEAD
  pthread_mutex_unlock(&(wq->lock));

=======
>>>>>>> 21ae9e5292d8f10d81cd3e34aaaa0e4725b899a0
  free(wq_item);
  return client_socket_fd;
}

/* Add ITEM to WQ. */
void wq_push(wq_t *wq, int client_socket_fd) {

<<<<<<< HEAD
  pthread_mutex_lock(&(wq->lock));

  /* TODO: Make me thread-safe! */


=======
  /* TODO: Make me thread-safe! */

>>>>>>> 21ae9e5292d8f10d81cd3e34aaaa0e4725b899a0
  wq_item_t *wq_item = calloc(1, sizeof(wq_item_t));
  wq_item->client_socket_fd = client_socket_fd;
  DL_APPEND(wq->head, wq_item);
  wq->size++;
<<<<<<< HEAD

  pthread_mutex_unlock(&(wq->lock));

  pthread_cond_signal(&(wq->cv));
=======
>>>>>>> 21ae9e5292d8f10d81cd3e34aaaa0e4725b899a0
}
