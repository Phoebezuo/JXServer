#include "common.h"
#include "dict.h"
#include "utilize.h"
#include "queue.h"
#include "server.h"

int main(int argc, char** argv) {
	assert(argc == 2); 

	// open the config file
    uint32_t ip = 0; 
	uint16_t port = 0; 
    char* directory = NULL; 
    load_config(argv[1], &ip, &port, &directory);

	// import dictionary from config file 
	struct dict* dictionary = malloc(sizeof(struct dict) * DICT_SIZE);
	socket_check(import_dictionary(&dictionary), "import failed"); 

	// initialize a queue to store the history for retrive 
	struct queue* query = queue_init();

	// initialize the server with field from config file 
    struct sockaddr_in address;	
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = ip;
	address.sin_port = port;
	int serversocket_fd = server_init(&address);
	int clientsocket_fd = -1;

	// while loop to handle multiple client connections 
	while (true) {
		uint32_t addrlen = sizeof(struct sockaddr_in);
		socket_check(clientsocket_fd = accept(serversocket_fd, 
			(struct sockaddr*) &address, &addrlen), "accept failed");
		
		// store info in a struct in order to use in different thread 
		struct connection_data* data = malloc(sizeof(struct connection_data));
		data->socket_fd = clientsocket_fd;
		data->dictionary = dictionary;
		data->directory = directory;
		data->query = query;	

		// put the workload in a thread
		pthread_t thread;
		pthread_create(&thread, NULL, connection_handler, data);
	}

	// shutdown the server 
	destroy_dict(dictionary);
	destroy_queue(query);
	close(serversocket_fd);
	exit(0);
}