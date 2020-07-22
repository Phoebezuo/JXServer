#include "utilize.h"

// check a socket can be successfully build
int socket_check(int result, char* msg) {
    if (result == SOCKET_ERROR) {
        perror(msg); 
        exit(0); 
    }
    return result; 
}

// send functionallity include header and free malloced memory
void send_all(int clientsocket_fd, struct msg message, int len, uint8_t type) {
	uint8_t header = type << 4 | message.compression << 3 
		| message.req_compression << 2; 
	send_msg(clientsocket_fd, header, message.payload, message.payload_len, len);
	if (message.payload) { free(message.payload); }
}

// only send header, payload length and payload
void send_msg(int clientsocket_fd, uint8_t header, uint8_t* payload, 
	uint64_t internet_len, int host_len) {
	send(clientsocket_fd, &header, sizeof(uint8_t), 0);
	send(clientsocket_fd, &internet_len, sizeof(uint64_t), 0);
	send(clientsocket_fd, payload, host_len, 0);
}

// when the query exist, send empty response back 
void retrive_send_empty(int clientsocket_fd, struct msg message) {
	uint8_t retrive_header = RETRIVE_RESPONSE << 4 | 
		message.compression << 3 | message.req_compression << 2;
	uint64_t temp_payload_len = 0;
	send(clientsocket_fd, &retrive_header, sizeof(uint8_t), 0);
	send(clientsocket_fd, &temp_payload_len, sizeof(uint64_t), 0);
}

// whenever there is an error, send error message back 
void error_handler(int clientsocket_fd) {
	uint8_t error_header = ERROR_RESPONSE << 4; 
	uint64_t error_payload_len = 0x0; 
	send(clientsocket_fd, &error_header, sizeof(uint8_t), 0);
	send(clientsocket_fd, &error_payload_len, sizeof(uint64_t), 0);
	close(clientsocket_fd); 
}

// check file a given file is exist in a given directory
int file_exists(char* filename, char* directory) {

	// open the target directory
	struct dirent* dir;
	DIR* d = opendir(directory);

	// the direcotry is not exist
	if (!d) {
		perror("cannot open directory");
		return false; 
	}
	
	// initialize a buffer to store regular file name 
	int buffer_count = 0;
	char** buffer = malloc(sizeof(char) * MAX_BUFFER_SIZE);

	// read through the directory and store filename in buffer 
	while ((dir = readdir(d))) {
		// get regular file in the directory
		if (dir->d_type == DT_REG) {
			buffer[buffer_count++] = dir->d_name; 
		}
	}

	// find the target file in directory
	for (int i = 0; i < buffer_count; i++) {
		if (strcmp(buffer[i], filename) == 0) {
			free(buffer);
			return true; 
		}
	}

	closedir(d);
	free(buffer);
	return false; 
}

// get the file size in bytes from a given file in given directory
char* get_file_size(char* filename, char* directory, uint64_t* file_size) {

	// concentrate the path with directory and filename 
	int path_len = strlen(filename) + strlen(directory) + 3;
	char* path = malloc(sizeof(char) * path_len);
	strcpy(path, directory);
	strcat(path, "/");
	strcat(path, filename);

	// get the size of target file
	struct stat info;
	if (stat(path, &info) != 0) { return NULL; }
	*file_size = (uint64_t)info.st_size;
	return path; 
}

// combine the all the query parameters with content of file 
uint8_t* get_retrive_payload(uint64_t retrive_payload_len, 
	uint8_t* payload, uint8_t* content) {

	// initialize a response to store the return result
    uint8_t* response = malloc(sizeof(uint8_t) * retrive_payload_len);

	// loop through the query parameters, store in response	
    for (int i = 0; i < 20; i++) {
        response[i] = payload[i];
    }

	// loop through the request content, store in response
    int counter = 0;
    for (int i = 20; i < (int)retrive_payload_len; i++) {
        response[i] = content[counter++];
    }
    return response;
}

void load_query(uint8_t* result, uint32_t* session_id, uint64_t* start_offset, 
	uint64_t* data_length, char** filename, int filename_length) {

	// copy the content in result to different field 
	memcpy(&(*session_id), result, sizeof(uint32_t));
	memcpy(&(*start_offset), &result[4], sizeof(uint64_t));
	memcpy(&(*data_length), &result[12], sizeof(uint64_t));
	memcpy(*filename, &result[20], filename_length);

	// convert the value from internet order to host order 
	*session_id = htobe32(*session_id);
	*start_offset = htobe64(*start_offset);
	*data_length = htobe64(*data_length);
}

