#ifndef UTILIZE_H
#define UTILIZE_H

#include "common.h"
#include "dict.h"
#include "queue.h"
#include "server.h"

int socket_check(int result, char* msg);

void send_all(int clientsocket_fd, struct msg message, int len, uint8_t type);

void send_msg(int clientsocket_fd, uint8_t header, uint8_t* payload, 
    uint64_t internet_len, int host_len);

void retrive_send_empty(int clientsocket_fd, struct msg message);

void error_handler(int clientsocket_fd);

int file_exists(char* filename, char* directory);

char* get_file_size(char* filename, char* directory, uint64_t* file_size);

uint8_t* get_retrive_payload(uint64_t retrive_payload_len, uint8_t* payload, 
    uint8_t* content);

void load_query(uint8_t* result, uint32_t* session_id, uint64_t* start_offset, 
    uint64_t* data_length, char** filename, int filename_length);

#endif