#include <sys/socket.h>
#include <assert.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

// Usage: ./generate-config <dotted quad ipv4 address xxx.xxx.xxx.xxx> <port number> <target directory>
// For example: ./generate-config 127.0.0.1 8888 /this/is/my/directory

int main(int argc, char ** argv) {
    FILE* fptr= fopen("config_file.bin", "wb"); 

    // IPv4, first 4 bytes 
    struct in_addr inaddr;
    inet_pton(AF_INET, argv[1], &inaddr);
    fwrite(&(inaddr.s_addr), sizeof(inaddr.s_addr), 1, fptr);

    // port number, next 2 bytes 
    uint16_t port = htons((uint16_t)strtol(argv[2], NULL, 10));
    fwrite(&port, sizeof(port), 1, fptr);

    // Remainder: directory as non-NULL terminated ASCII
    fwrite(argv[3], strlen(argv[3]), 1, fptr);
    fclose(fptr);
    return 0;
}
