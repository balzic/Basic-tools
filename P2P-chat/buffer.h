#include <stdlib.h>
#include <string.h>

typedef struct queue_node {
    char* data;
    size_t data_len;
    struct queue_node* next;
} queue_node;

typedef struct linked_queue {
    queue_node* head;
    queue_node* tail;
} linked_queue;

/**
 * Adds len bytes from buffer into the tail of the queue
 **/
void enqueue(linked_queue* queue,  char* buffer, size_t len);

/**
 * Removes upto len bytes from the head of the queue into the buffer
 * Returns number of remaining bytes of data in the head of the queue
 * Returns -1 if the head of the queue is empty
 **/
ssize_t dequeue(linked_queue* queue, char* buffer, size_t len);


/**
 * Initialises the values of a newly allocated queue structure.
 * NOTE: Using this function to _reset_ a linked queue can result in
 * memory leaks.
 **/
void set_empty_queue(linked_queue* queue);
