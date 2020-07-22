#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"
#include "dict.h"
#include "utilize.h"
#include "server.h"

struct queue* queue_init();

void destroy_queue(struct queue* q);

void node_init(struct node* temp, uint32_t session_id, uint64_t start_offset, 
    uint64_t data_length, char* filename);

uint8_t query_exist(struct node* cursor, uint32_t seesion_id, 
    uint64_t start_offset, uint64_t data_length, char* filename);

#endif 