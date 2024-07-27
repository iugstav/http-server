#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef uint64_t u64;
typedef int64_t i64;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint8_t u8;
typedef int8_t i8;

void *handle_conn(void *client);

const char *response_ok	       = "HTTP/1.1 200 OK\r\n";
const char *response_not_found = "HTTP/1.1 404 Not Found\r\n\r\n";

int main() {
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	int server_fd;
	uint client_addr_len;
	struct sockaddr_in client_addr;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}

	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEPORT failed: %s \n", strerror(errno));
		return 1;
	}

	struct sockaddr_in serv_addr = {
	    .sin_family = AF_INET,
	    .sin_port	= htons(4221),
	    .sin_addr	= {htonl(INADDR_ANY)},
	};

	if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}

	int connection_backlog = 10;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}

	client_addr_len = sizeof(client_addr);

	while (1) {
		int fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

		int *pthread_fd = &fd;
		pthread_t proc;
		pthread_create(&proc, NULL, handle_conn, pthread_fd);
	}

	close(server_fd);

	return 0;
}

void *handle_conn(void *sock) {
	int client = *((int *)sock);

	char buffer[2048];
	ssize_t bytes_received = recv(client, buffer, 2048, 0);

	if (bytes_received <= 0) {
		printf("Not received data: %s\n", strerror(errno));
		return NULL;
	}

	printf("Client connected %d\n", client);

	char *url_path = strtok(buffer, " ");
	url_path       = strtok(NULL, " "); // avança no pointer

	int only_echo_or_more = strncmp(url_path, "/echo/", 6);
	if (only_echo_or_more == 1) {
		char resp[1024];
		sprintf(resp, "%sContent-Type: text/plain\r\nContent-Length: 4\r\n\r\n%s", response_ok, "echo");

		send(client, resp, strlen(resp), 0);
	} else if (only_echo_or_more == 0) {
		char *path = url_path + 6; // +6 porque tem que excluir o "echo", então a gente anda com o pointer
		char resp[1024];
		sprintf(resp, "%sContent-Type: text/plain\r\nContent-Length: %ld\r\n\r\n%s", response_ok, strlen(path),
			path);

		send(client, resp, strlen(resp), 0);
	} else {
		send(client, response_not_found, strlen(response_not_found), 0);
	}

	return NULL;
}
