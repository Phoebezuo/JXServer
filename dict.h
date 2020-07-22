#ifndef DICT_H
#define DICT_H

#include "common.h"
#include "utilize.h"
#include "queue.h"
#include "server.h"

int import_dictionary(struct dict** dictionary);

void set_bit(uint8_t* buf, int index, int value);

uint8_t get_bit(uint8_t* array, int index);

void bitarray_init(uint8_t* content, int file_size, uint8_t** value_array, 
    int** index_array);

void dict_init(struct dict** dictionary, uint8_t** value_array, 
    int** index_array);

void destroy_dict(struct dict* dictionary);

uint8_t* encode(struct dict* dictionary, uint8_t* origin_hex, 
    int origin_len, int* result_len);

uint8_t* decode(struct dict* dictionary, uint8_t* origin_hex, 
    int origin_len, int* result_len);

#endif