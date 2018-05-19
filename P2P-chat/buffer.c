#include "buffer.h"
#include "csapp.h"

void enqueue(linked_queue* queue,  char* buffer, size_t len) {
    if (queue == NULL) {
        app_error("NULL queue");
    } else if (buffer == NULL) {
        app_error("NULL buffer");
    }
    
    queue_node* new_node = (queue_node*) Malloc(sizeof(queue_node));
    new_node -> next = NULL;
    new_node -> data = (char*) Malloc(len);
    new_node -> data_len = len;
    memcpy(new_node -> data, buffer, len);
    
    if (queue -> tail == NULL) {
        queue -> head = new_node;
    } else {
        queue -> tail -> next = new_node;
    }
    queue -> tail = new_node;
}

ssize_t dequeue(linked_queue* queue, char* buffer, size_t len) {
    if (queue == NULL) {
        app_error("NULL queue");
    } else if (buffer == NULL) {
        app_error("NULL buffer");
    }

    queue_node* cur_head = queue->head;
    if(cur_head == NULL) {
        return -1;
    }
    
    if (cur_head -> data_len <= len) {
        memcpy(buffer, cur_head->data, cur_head->data_len);
        if(cur_head == queue -> tail) {
            queue -> head = queue -> tail = NULL;
        }
        queue -> head = cur_head -> next;
        free(cur_head -> data);
        free(cur_head);
        return 0;
    } else {
        memcpy(buffer, cur_head -> data, len);
        memmove(cur_head -> data, cur_head -> data + len, cur_head -> data_len - len);
        cur_head -> data_len -= len;
        cur_head -> data = Realloc(cur_head -> data, cur_head -> data_len);
        return cur_head -> data_len;
    }
}

void set_empty_queue(linked_queue* queue) {
        queue->head = NULL;
        queue->tail = NULL;
}
