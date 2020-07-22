#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include "dict.h"
#include "utilize.h"
#include "queue.h"

int server_init(struct sockaddr_in* address_ptr);

void load_config(char* argv, uint32_t* ip, uint16_t* port, char** directory);

void* connection_handler(void* arg); 

void echo_func(struct msg message, int clientsocket_fd, 
    struct dict* dictionary);

void directory_func(struct msg message, int clientsocket_fd, 
    struct dict* dictionary, char* directory);

void file_size_func(struct msg message, int clientsocket_fd, 
    struct dict* dictionary, char* directory);

void retrive_func(struct msg message, int clientsocket_fd, 
    struct dict* dictionary, char* directory, struct queue* q);

#endif