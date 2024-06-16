#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8008
#define BACKLOG 128

typedef struct {
    int socket;
} server_t;

int server_listen(server_t *server);

int server_accept(server_t *server);

int main() {
    int err = 0;
    server_t server = { 0 };

    err = server_listen(&server);
    if (err) {
        printf("Failed to listen on address 0.0.0.0:%d\n", PORT);
        return err;
    }

    while (1) {
        err = server_accept(&server);
        if (err) {
            printf("Failed accepting connetion\n");
            return err;
        }
    }

    return err;
}

/* The `socket(2)` syscall creates an endpoint for communication
 * and returns a file descriptor that refers to that endpoint.
 *
 * It takes three arguments (the last being just to provide greater
 * specificity):
 * -    domain (communication domain)
 *      AF_INET              IPv4 Internet protocols
 *
 * -    type (communication semantics)
 *      SOCK_STREAM          Provides sequenced, reliable,
 *                           two-way, connection-based byte
 *                           streams.
 */
int server_listen(server_t *server) {
    int err = 0;
    err = (server->socket = socket(AF_INET, SOCK_STREAM, 0));
    if (err == -1) {
        perror("Socket");
        printf("Failed to create socket\n");
        return err;
    }

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    err = bind(server->socket, (struct sockaddr*)&addr, sizeof(addr));
    if (err == -1) {
        perror("bind");
        printf("Failed to bind socket to address\n");
        return err;
    }

    err = listen(server->socket, BACKLOG);
    if (err == -1) {
        perror("listen");
        printf("Failed to put socket in passive mode\n");
        return err;
    }

    return err;
}

int server_accept(server_t *server) {
    int err = 0;
    int conn_fd;
    socklen_t client_len;
    struct sockaddr_in client_addr;
    char recv_buff[100] = "";

    client_len = sizeof(client_addr);
    err = (conn_fd = accept(server->socket, (struct sockaddr*)&client_addr, &client_len));
    if (err == -1) {
        perror("accept");
        printf("Failed accepting connection\n");
        return err;
    }

    printf("Client connected!\n");
    while (1) {
        recv(conn_fd, recv_buff, sizeof(recv_buff), 0);
        printf("\n[client]: %s", recv_buff);
        send(conn_fd, recv_buff, sizeof(recv_buff), 0);
        if (strcmp(recv_buff, "bye\n") == 0) {
            break;
        }
    }
    printf("Client disconnected!\n");

    err = close(conn_fd);
    if (err == -1) {
        perror("close");
        printf("Failed to close connection\n");
        return err;
    }

    return err;
}
