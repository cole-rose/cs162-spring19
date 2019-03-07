#include <stdlib.h>
#include "wq.h"
#include "utlist.h"

/* Initializes a work queue WQ. */
void wq_init(wq_t *wq) {

    pthread_mutex_init(&(wq->lock), NULL);
    pthread_cond_init(&(wq->cv), NULL);

    /* TODO: Make me thread-safe! */
    wq->size = 0;
    printf("wq->size: %d\n", wq->size);
    wq->head = NULL;
}

/* Remove an item from the WQ. This function should block until there
 * is at least one item on the queue. */
int wq_pop(wq_t *wq) {

    /* TODO: Make me blocking and thread-safe! */

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
    int client_socket_fd = wq->head->client_socket_fd;
    wq->size--;
    DL_DELETE(wq->head, wq->head);

    pthread_mutex_unlock(&(wq->lock));

    free(wq_item);
    return client_socket_fd;
}

/* Add ITEM to WQ. */
void wq_push(wq_t *wq, int client_socket_fd) {

    pthread_mutex_lock(&(wq->lock));

    /* TODO: Make me thread-safe! */


    wq_item_t *wq_item = calloc(1, sizeof(wq_item_t));
    wq_item->client_socket_fd = client_socket_fd;
    DL_APPEND(wq->head, wq_item);
    wq->size++;

    pthread_mutex_unlock(&(wq->lock));

    pthread_cond_signal(&(wq->cv));
}