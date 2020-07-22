#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>


// ------------------------------------------ //
// comment below if your system is linux      //
// uncomment below if your system is macOS    //
// ------------------------------------------ //
#include <libkern/OSByteOrder.h>           //
#define htobe64(x) OSSwapHostToBigInt64(x) //
#define htobe32(x) OSSwapHostToBigInt32(x) //
#define be64toh(x) OSSwapBigToHostInt64(x) //
// ------------------------------------------ //


// define mainly for server
#define SOCKET_ERROR (-1)
#define NULL_BYTE (0x00)

// define mainly for dictionary
#define DICT_SIZE (256)
#define BYTE_SIZE (8)
#define MAX_BUFFER_SIZE (100)
#define VALUE_MAX_SIZE (256 / BYTE_SIZE)
#define OFFSET(x) ((BYTE_SIZE - 1 - x % BYTE_SIZE))

// define mainly for handle different request
#define ECHO_REQUEST (0x0) // 0000
#define ECHO_RESPONSE (0x1) // 0001
#define DIRECTORY_REQUEST (0x2) // 0010
#define DIRECTORY_RESPONSE (0x3) // 0011
#define FILE_SIZE_REQUEST (0x4) // 0100
#define FILE_SIZE_RESPONSE (0x5) // 0101
#define RETRIVE_REQUEST (0x6) // 0110
#define RETRIVE_RESPONSE (0x7) // 0111
#define SHUTDOWN_REQUEST (0x8) // 1000
#define ERROR_RESPONSE (0xf) // 1111

struct msg {
    uint8_t type: 4;
	uint8_t compression: 1;
	uint8_t req_compression: 1;
	uint64_t payload_len;
	uint8_t* payload;
};

struct connection_data {
	int socket_fd;
	struct dict* dictionary;
	char* directory;
	struct queue* query;
};

struct dict {
	uint8_t key;
	int value_len;
	uint8_t value[VALUE_MAX_SIZE];
};

struct queue {
    struct node* head;
    pthread_mutex_t lock;
};

struct node {
    struct node* next;
    uint32_t session_id;
	uint64_t start_offset;
	uint64_t data_length;
	char* filename;
};

#endif
