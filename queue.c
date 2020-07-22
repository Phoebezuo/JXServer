#include "queue.h"

struct queue* queue_init() {
	// initialize a queue 
	struct queue* q = malloc(sizeof(struct queue));
	pthread_mutex_init(&q->lock, NULL);

	// assign node to queue and return 
    q->head = malloc(sizeof(struct node));
	node_init(q->head, 0, 0, 0, NULL); 
	return q;
}

void destroy_queue(struct queue* q) {
    struct node* cursor = q->head;

    // loop through all nodes in queue
    while (cursor->next) {
        struct node* next_node = cursor->next;
        free(cursor->filename);
        free(cursor);
        cursor = next_node;
    }

    // loop through the last one in queue
    if (cursor) {
        free(cursor->filename);
        free(cursor);
    }

    // destroy lock of queue 
    pthread_mutex_destroy(&q->lock);
    free(q);
}

void node_init(struct node* temp, uint32_t session_id, uint64_t start_offset, 
    uint64_t data_length, char* filename) {

    // initialize a node with given filed 
    temp->next = NULL;
    temp->session_id = session_id;
    temp->start_offset = start_offset;
    temp->data_length = data_length;

    if (filename) {
        // if pass in a filename, store it in node 
        temp->filename = strdup(filename);
    } else {
        // set the filename in struct to NULL when there is no filename
        temp->filename = NULL;
    }
}

// a query is define to be exist if all its field are same 
uint8_t query_exist(struct node* cursor, uint32_t session_id, 
    uint64_t start_offset, uint64_t data_length, char* filename) {
	return cursor->session_id == session_id && 
        cursor->start_offset == start_offset && 
        cursor->data_length == data_length && 
        strcmp(cursor->filename, filename) == 0;
    }