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

typedef struct http_request_t {
	char method[8];
	char url[64]; // provavelmente suficiente
} http_req;

int handle_conn(int client);
http_req *parse_http_request(char *s);
int server_init(int portn);

const char *response_ok	       = "HTTP/1.1 200 OK\r\n";
const char *response_not_found = "HTTP/1.1 404 Not Found\r\n\r\n";

int main() {
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	int server_fd = server_init(4221);
	struct sockaddr_in client;
	uint client_len = sizeof(client);

	while (1) {
		int fd = accept(server_fd, (struct sockaddr *)&client, &client_len);
		if (fd < 0) {
			printf("accept() failed: %s\n", strerror(errno));
			return 1;
		}

		handle_conn(fd);
	}

	close(server_fd);

	return 0;
}

int server_init(int portn) {
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("socket creation failed: %s...\n", strerror(errno));
		return 1;
	}

	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEPORT failed: %s \n", strerror(errno));
		return 1;
	}

	struct sockaddr_in serv_addr = {
	    .sin_family = AF_INET,
	    .sin_port	= htons(portn),
	    .sin_addr	= {htonl(INADDR_ANY)},
	};

	if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
		close(server_fd);
		printf("bind() failed: %s \n", strerror(errno));
		return 1;
	}

	int connection_backlog = 10;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("listen() failed: %s \n", strerror(errno));
		return 1;
	}

	return server_fd;
}

int handle_conn(int client) {
	char buffer[2048];
	ssize_t bytes_received = recv(client, buffer, 2048, 0);

	if (bytes_received <= 0) {
		printf("Not received data: %s\n", strerror(errno));
		return 0;
	}

	printf("Client connected %d\n", client);

	http_req *req = parse_http_request(buffer);

	if (strcmp(req->url, "/")) {
		char resp[1024];
		char *content =
		    "<!DOCTYPE html>"
		    "<html lang='en'>"
		    "<head>"
		    "<meta charset='UTF-8'>"
		    "<meta name='viewport' content='width=device-width,initial-scale=1'><title>teste</title>"
		    "<link href='css/style.css' rel='stylesheet'></head>"
		    "<body>"
		    "<p>fala maluco</p>"
		    "</body> </html> ";

		unsigned long content_size = strlen(content);
		sprintf(resp, "%sContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s", response_ok, content_size,
			content);

		send(client, resp, strlen(resp), 0);
	}

	int only_echo_or_more = strncmp(req->url, "/echo/", 6);
	// comparação com base nos valores de retorno da strncmp
	if (only_echo_or_more == 1) {
		char resp[1024];
		sprintf(resp, "%sContent-Type: text/plain\r\nContent-Length: 4\r\n\r\n%s", response_ok, "echo");

		send(client, resp, strlen(resp), 0);
	} else if (only_echo_or_more == 0) {
		char *path = req->url + 6; // +6 porque tem que excluir o "echo", então a gente anda com o pointer
		char resp[1024];
		sprintf(resp, "%sContent-Type: text/plain\r\nContent-Length: %ld\r\n\r\n%s", response_ok, strlen(path),
			path);

		send(client, resp, strlen(resp), 0);
	} else {
		send(client, response_not_found, strlen(response_not_found), 0);
	}

	free(req);
	close(client);
	return 0;
}

http_req *parse_http_request(char *s) {
	http_req *req = malloc(sizeof(http_req));

	int first_space = 0, second_space = 0;

	char *p = s;
	int i	= 0;
	while (*(p + i)) {
		if (*(p + (i + 1)) == ' ') {
			if (first_space == 0) {
				first_space = i + 1;
				i += 2;
				p = p + i;
				continue;
			} else {
				second_space = i + 1;
				break;
			}
		}

		++i;
	}

	strncpy(req->method, s, first_space);
	strncpy(req->url, p, second_space);

	return req;
}
