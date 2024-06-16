#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8008
#define SOCKET_ERROR -1
#define MAX_CLIENTS 2
#define BACKLOG 5

typedef struct {
    struct sockaddr_in addr;
    int master_socket;
    int client_sockets[MAX_CLIENTS];
} server_t;

server_t *server_init(short port, int backlog);
void server_main_loop(server_t *server);
void server_accept_new_connection(server_t *server, char *welcome);
void server_handle_client_close(server_t *server, int client_socket_idx);
void server_handle_client_req(server_t *server, int client_socket_idx, char *buffer, int valread);
void server_print_sockets(server_t *server);
int check_or_exit(int err, const char *msg);

int main() {
    server_t *server;

    server = server_init(PORT, BACKLOG);
    server_main_loop(server);

    return 0;
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
server_t *server_init(short port, int backlog) {
    server_t *server = malloc(sizeof(server_t));
    if (server == NULL) {
        perror("Failed to allocate memory for server");
        exit(1);
    }

    server->master_socket = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        server->client_sockets[i] = 0;
    }

    check_or_exit(server->master_socket = socket(AF_INET, SOCK_STREAM, 0), "Failed to create master socket");
    check_or_exit(setsockopt(server->master_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)), "Failed to set master socket options");

    server->addr.sin_family = AF_INET;
    server->addr.sin_addr.s_addr = INADDR_ANY;
    server->addr.sin_port = htons(port);

    check_or_exit(bind(server->master_socket, (struct sockaddr*)&server->addr, sizeof(server->addr)), "Failed to bind master socket to address");

    check_or_exit(listen(server->master_socket, backlog), "Failed to put master socket on passive mode");
    printf("Waiting for connections on 0.0.0.0:%d\n", PORT);

    return server;
}

void server_print_sockets(server_t *server) {
    printf("ip: %s, port: %d\n", inet_ntoa(server->addr.sin_addr), ntohs(server->addr.sin_port));
    printf("[Master socket]: %d\n", server->master_socket);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        printf("[%d] -> %d\n", i, server->client_sockets[i]);
    }
}

int check_or_exit(int err, const char *msg) {
    if (err == SOCKET_ERROR) {
        perror(msg);
        exit(1);
    }
    return err;
}

void server_accept_new_connection(server_t *server, char *welcome) {
    int new_socket, new_con_accepted = 0;

    if ((new_socket = accept(server->master_socket, (struct sockaddr *)&server->addr, (socklen_t*)&server->addr)) == SOCKET_ERROR) {
        perror("accept");
        printf("Failed accepting connection\n");
    }
    //add new socket to array of sockets
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->client_sockets[i] == 0) {
            server->client_sockets[i] = new_socket;
            server_print_sockets(server);
            new_con_accepted = 1;
            send(new_socket, welcome, strlen(welcome), 0);
            printf(
                    "Connection accepted (socket: %d) from ip:%s port:%d\n",
                    new_socket,
                    inet_ntoa(server->addr.sin_addr),
                    ntohs(server->addr.sin_port)
                  );
            break;
        }
    }
    if (new_con_accepted == 0) {
        printf("No more connections allowed\n");
        close(new_socket);
    }
}

void server_handle_client_close(server_t *server, int client_socket_idx) {
    getpeername(server->client_sockets[client_socket_idx], (struct sockaddr*)&server->addr, (socklen_t*)&server->addr);
    printf(
            "Host disconnected (socked: %d), ip %s, port %d\n",
            server->client_sockets[client_socket_idx],
            inet_ntoa(server->addr.sin_addr),
            ntohs(server->addr.sin_port)
          );
    close(server->client_sockets[client_socket_idx]);
    server->client_sockets[client_socket_idx] = 0;
}

void server_handle_client_req(server_t *server, int client_socket_idx, char *buffer, int valread) {
    //Echo back the message that came in
    //set the string terminating NULL byte on the end of the data read
    buffer[valread] = '\0';
    if (strncmp("sleep", buffer, strlen(buffer) - 2) == 0) {
        sleep(5);
    }
    send(server->client_sockets[client_socket_idx], buffer, strlen(buffer), 0);
}

void server_main_loop(server_t *server) {
    int max_sd, activity, valread;
    fd_set readfds;
    char buffer[1025];
    char *welcome = "Welcome to the server\n";

    while (1) {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(server->master_socket, &readfds);

        //add child sockets to set
        max_sd = server->master_socket;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server->client_sockets[i] > 0) {
                FD_SET(server->client_sockets[i], &readfds);
            }
            if (server->client_sockets[i] > server->master_socket) {
                max_sd = server->client_sockets[i];
            }
        }

        //wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            printf("select error: %d, errno: %d\n", activity, errno);
        }

        //If something happened on the master socket, then its an incoming connection
        if (FD_ISSET(server->master_socket, &readfds)) {
            server_accept_new_connection(server, welcome);
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (FD_ISSET(server->client_sockets[i], &readfds)) {
                if ((valread = read(server->client_sockets[i], buffer, 1024)) == 0) {
                    //Somebody disconnected. Close the socket and mark as 0 in list for reuse
                    server_handle_client_close(server, i);
                } else {
                    server_handle_client_req(server, i, buffer, valread);
                }
            }
        }
    }
}
