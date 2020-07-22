#include "server.h"

void echo_func(struct msg message, int clientsocket_fd, 
	struct dict* dictionary) {

	// convert payload length from internt order to host order 
	int len = be64toh(message.payload_len);

	// it will be an error if the length is smaller or equal to zero 
	if (len <= 0) { return; }	

	// receive bytes of payload according to the host payload length 
	message.payload = malloc(len * sizeof(uint8_t));
	assert(recv(clientsocket_fd, message.payload, len, 0) > 0);

	// send back respond 
	if (message.compression == true) {
		if (message.req_compression == true) {
			// change the require compression bit to false during send back 
			message.req_compression = false;
		} 
	} else {
		if (message.req_compression == true) {
			// set corresponding compreesion and require compression bit 
			message.compression = true;
			message.req_compression = false;

			// compressed the original request 
			int result_len = 0;
			uint8_t* result_hex = encode(dictionary, message.payload, 
				len, &result_len);
			len = result_len;

			// update the internet payload length after compression 
			message.payload_len = htobe64(result_len);
			memcpy(message.payload, result_hex, sizeof(uint8_t) * result_len);
			free(result_hex);
		} 
	}
	
	// send echo response back to client 
	send_all(clientsocket_fd, message, len, ECHO_RESPONSE);
}

void directory_func(struct msg message, int clientsocket_fd, 
	struct dict* dictionary, char* directory) {

	// it will be an error if the payload length is not equal to zero 
	if (be64toh(message.payload_len) != 0) { return; }

	// initialize a buffer to store result of payload 
	char* buffer = malloc(sizeof(char) * MAX_BUFFER_SIZE); 
	for (int i = 0; i < MAX_BUFFER_SIZE; i++) { buffer[i] = NULL_BYTE; }
	int buffer_count = 0; 

	// open the target directory
	struct dirent* dir;
	DIR* d = opendir(directory);

	// it will be an error if direcotry is not exist 
	if (!d) {
		error_handler(clientsocket_fd);
		return; 
	}

	// read through the directory and store filename in buffer 
	while ((dir = readdir(d))) {
		// get regular file in the directory
		if (dir->d_type == DT_REG) {

			// copy every single char of the file name to buffer 
			for (int i = 0; i < (int)strlen(dir->d_name); i++) {
				buffer[buffer_count + i] = dir->d_name[i];
			}

			// increment the buffer by the number of single file name 
			buffer_count += (int)strlen(dir->d_name);

			// each file name is sepeated by a null 
			buffer[buffer_count++] = NULL_BYTE;
		}
	}
	closedir(d);

	// when directory is empty, buffer contain only a null 
	if (buffer_count == 0) { 
		buffer[buffer_count++] = NULL_BYTE; 
	}
	
	// send back response
	int len = 0; 
	if (message.req_compression == true) {

		// set corresponding compreesion and require compression bit 
		message.compression = true;
		message.req_compression = false;

		// compress the response
		int result_len = 0;
		uint8_t* result_hex = encode(dictionary, (uint8_t*)buffer, 
			buffer_count, &result_len);
		len = result_len;

		// convert result len to internet payload length 
		message.payload_len = htobe64(result_len);

		// copy the result hex to the payload 
		message.payload = malloc(sizeof(uint8_t) * result_len);
		memcpy(message.payload, result_hex, sizeof(uint8_t) * result_len);
		free(result_hex);
		free(buffer);
	} else {
		// convert result len to internet payload length 
		len = buffer_count;
		message.payload_len = htobe64(buffer_count);

		// copy the buffer to the payload 
		message.payload = malloc(sizeof(uint8_t) * buffer_count);
		memcpy(message.payload, buffer, buffer_count);
		free(buffer);
	}

	// send dictectory response back to client 
	send_all(clientsocket_fd, message, len, DIRECTORY_RESPONSE);
}

void file_size_func(struct msg message, int clientsocket_fd, 
	struct dict* dictionary, char* directory) {

	// convert payload length from internt order to host order 
	int len = be64toh(message.payload_len);

	// it will be an error if the length is smaller or equal to zero 
	if (len <= 0) { return; }

	// receive bytes of payload according to the host payload length 
	message.payload = malloc(len * sizeof(uint8_t));
	assert(recv(clientsocket_fd, message.payload, len, 0) > 0);

	// send error message if the file is not exist 
	char* filename = (char*)message.payload;
	if (file_exists(filename, directory) == false) {
		error_handler(clientsocket_fd);
		return;
	}

	// get the number of bytes in the requesting file 
	uint64_t file_size = 0;
	char* path = get_file_size(filename, directory, &file_size);
	free(filename);
	free(path);

	if (message.req_compression == true) {
		// set corresponding compreesion and require compression bit 
		message.compression = true;
		message.req_compression = false;

		// set the filesize header
		uint8_t filesize_header = FILE_SIZE_RESPONSE << 4 | 
			message.compression << 3 | message.req_compression << 2; 

		// convert uint64_t to uint8_t array with size 8 
		uint64_t number = htobe64(file_size);
		uint8_t* response = (uint8_t*)&number;

		// compressed the original response
		int result_len = 0;
		uint8_t* result = encode(dictionary, response, 8, &result_len);
		message.payload_len = htobe64(result_len);

		// send file size response back to client 
		send_msg(clientsocket_fd, filesize_header, result, 
			message.payload_len, result_len);
		free(result);
	} else {

		// set the filesize header 
		uint8_t filesize_header = FILE_SIZE_RESPONSE << 4 | 
			message.compression << 3 | message.req_compression << 2; 

		// convert uint64_t to uint8_t array with size 8 
		uint64_t number = htobe64(file_size);
		uint8_t* response = (uint8_t*)&number;
		message.payload_len = htobe64(8);

		// send file size response back to client 
		send_msg(clientsocket_fd, filesize_header, response, 
			message.payload_len, 8);
	}
}

void retrive_func(struct msg message, int clientsocket_fd, 
	struct dict* dictionary, char* directory, struct queue* q) {

	// convert payload length from internt order to host order 
	int len = be64toh(message.payload_len);

	// it will be an error if the length is smaller or equal to zero 
	if (len <= 0) { return; }

	// receive bytes of payload according to the host payload length 
	message.payload = malloc(len * sizeof(uint8_t));
	assert(recv(clientsocket_fd, message.payload, len, 0) > 0);

	// initialize each request differnt field 
	uint32_t session_id = 0;	
	uint64_t start_offset = 0;
	uint64_t data_length = 0; 
	uint8_t* result = NULL; 
	int result_len = 0;

	if (message.compression == true) {
		// decompress the payload when client send in compressed version 
		result = decode(dictionary, message.payload, len, &result_len); 
	} else {
		// there is no need to decompress the payload
		result = message.payload;
		result_len = len;
	}

	// initialize filename with the size of rest payload 
	int filename_length = result_len - sizeof(session_id) - 
		sizeof(start_offset) - sizeof(data_length);
	char* filename = malloc(sizeof(char) * filename_length);

	// load payload data to different field 
	load_query(result, &session_id, &start_offset, &data_length, 
		&filename, filename_length); 

	// use linked list to store the histroy query 
	pthread_mutex_lock(&q->lock);
	struct node* cursor = q->head;

	// loop though the histroy query, find if the new request is exist before 
	while (cursor->next) {
		// as soon as it has found, send back empty response
		if (query_exist(cursor, session_id, start_offset, 
			data_length, filename) == true) { 
			message.compression = false; 
			message.req_compression = false; 
			retrive_send_empty(clientsocket_fd, message);
			pthread_mutex_unlock(&q->lock);
			return; 
		} 
		cursor = cursor->next;
	}

	// check the latest query in the list 
	if (query_exist(cursor, session_id, start_offset, 
		data_length, filename) == true) { 
		// if so, sendd back empty response 
		message.compression = false; 
		message.req_compression = false; 
		retrive_send_empty(clientsocket_fd, message);
		pthread_mutex_unlock(&q->lock);
		return; 
	} 

	// the request is not exist in the history, create a new log into list 
	cursor->next = malloc(sizeof(struct node));
	node_init(cursor->next, session_id, start_offset, data_length, filename); 
	pthread_mutex_unlock(&q->lock);

	// send error message if the file is not exist 
	if (file_exists(filename, directory) == false) {
		error_handler(clientsocket_fd);
		return;
	}

	// get the size of request range 
	uint64_t file_size = 0; 
	char* path = get_file_size(filename, directory, &file_size);

	// retrive file have bad range
	if (file_size < (start_offset + data_length)) {
		error_handler(clientsocket_fd);
		return;
	}

	// store the file content in content
	FILE* fptr = fopen(path, "r");
	assert(fptr != NULL);
	fseek(fptr, start_offset, SEEK_SET);
	uint8_t* content = malloc(sizeof(uint8_t) * data_length);
	fread(content, 1, data_length, fptr);
	fclose(fptr);

	// merge data into one response 
	uint64_t response_len = data_length + sizeof(session_id) + 
		sizeof(start_offset) + sizeof(data_length); 
	uint8_t* response = get_retrive_payload(response_len, result, content);
	
	if (message.req_compression == true) {
		// set corresponding compreesion and require compression bit 
		message.compression = true; 
		message.req_compression = false; 
		uint8_t retrive_header = RETRIVE_RESPONSE << 4 | 
			message.compression << 3 | message.req_compression << 2; 
		
		// compress the response 
		int ret_len = 0;
		uint8_t* ret = encode(dictionary, response, response_len, &ret_len);
		message.payload_len = htobe64(ret_len);

		// send retrive response back to client 
		send_msg(clientsocket_fd, retrive_header, ret, 
			message.payload_len, ret_len);
		free(response);
		free(ret);
	} else {
		// set corresponding compression and require compression bit 
		message.compression = false;
		message.req_compression = false;
		uint8_t retrive_header = RETRIVE_RESPONSE << 4 | 
			message.compression << 3 | message.req_compression << 2; 
		
		// convert the result len to internet payload length 
		message.payload_len = htobe64(response_len);

		// send retrive response back to client 
		send_msg(clientsocket_fd, retrive_header, response, 
			message.payload_len, response_len);
		free(response);
	}

	// free the malloced memory 
	free(content);
	free(filename);
	free(path);
	free(message.payload);
}

void* connection_handler(void* arg) {
	struct connection_data* data = (struct connection_data*) arg;

	// load the corresponding data from the connection data 
	struct dict* dictionary = data->dictionary;
	int clientsocket_fd = data->socket_fd;
	char* directory = data->directory; 
	struct queue* query = data->query; 

	while (true) {
		// initialize the message 
		struct msg message = {0};

		// recevie one byte for the message header 
		uint8_t header; 
		if (recv(clientsocket_fd, &header, sizeof(uint8_t), 0) <= 0) { 
			free(arg);
			return NULL; 
		}

		// receive 8 byte for the payload length 
		if (recv(clientsocket_fd, &message.payload_len, 
			sizeof(uint64_t), 0) <= 0) { 
			free(arg);
			return NULL; 
		} 

		// bit shift in order to store in the struct 
		message.type = header >> 4;
		message.compression = header >> 3;
		message.req_compression = header >> 2;

		if (message.type == ECHO_REQUEST) { 

			// handle echo functionallity 
			echo_func(message, clientsocket_fd, dictionary);
			
		} else if (message.type == DIRECTORY_REQUEST) { 

			// handle directory functionallity 
			directory_func(message, clientsocket_fd, dictionary, directory);

		} else if (message.type == FILE_SIZE_REQUEST) { 
			
			// handle file size functionallity
			file_size_func(message, clientsocket_fd, dictionary, directory);

		} else if (message.type == RETRIVE_REQUEST) { 
			
			// handle retrive functionallity
			retrive_func(message, clientsocket_fd, dictionary, 
				directory, query);

		} else if (message.type == SHUTDOWN_REQUEST) { 

			// it is error if the the shutdown request length is not zero 
			if (be64toh(message.payload_len) != 0) {
				return NULL; 
			}

			// free malloced memory
			if (message.payload) { free(message.payload); }
			free(data);
			destroy_queue(query);
			destroy_dict(dictionary);

			// close the socket 
			close(clientsocket_fd);
			return NULL; 

		} else { 
			// handle error functionallity when it request something else 
			error_handler(clientsocket_fd); 
			if (message.payload) { free(message.payload); }
			return NULL; 
		}
	}
	return NULL;
}

// initialize server with acquire ip address and port from struct sockaddr_in 
int server_init(struct sockaddr_in* address_ptr) {
	int serversocket_fd = -1;
	socket_check(serversocket_fd = socket(AF_INET, SOCK_STREAM, 0), 
		"socket failed"); 
	socket_check(bind(serversocket_fd, (struct sockaddr*)address_ptr, 
		sizeof(struct sockaddr_in)), "bind failed"); 
	socket_check(listen(serversocket_fd, SOMAXCONN), "listen failed");
	return serversocket_fd;
}

// load the config file from the argument 
void load_config(char* argv, uint32_t* ip, uint16_t* port, char** directory) {
    // read the ip and port in a binary file 
	FILE* fptr = fopen(argv, "rb"); 
	assert(fptr != NULL);
	fread(&(*ip), sizeof(uint32_t), 1, fptr); 
	fread(&(*port), sizeof(uint16_t), 1, fptr); 

	// use struct stat to get the size of directory length 
	struct stat info;
	stat(argv, &info);
	int directory_len = (int)info.st_size - sizeof(uint32_t) - sizeof(uint16_t);

	// malloc the size of directory with corresponding length 
	*directory = malloc(sizeof(char) * (directory_len+1)); 
	fread(*directory, sizeof(char), directory_len, fptr); 

	// add a EFO to the string 
	(*directory)[directory_len] = '\0';
	fclose(fptr);
}