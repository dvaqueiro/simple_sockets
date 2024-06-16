#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>

int main (int argc, char *argv[])
{
    struct sockaddr_in server;
    int socket_fd, err = 0;
    char recv_buff[100] = "";
    char send_buff[100] = "";

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(8008);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    err = connect(socket_fd, (struct sockaddr *)&server, sizeof(server));
    if (err == -1) {
        printf("Error: connect\n");
        return err;
    }

    while (1) {
        printf("\nclient:");
        fgets(send_buff, sizeof(send_buff), stdin);
        send(socket_fd, send_buff, sizeof(send_buff), 0);
        recv(socket_fd, recv_buff, sizeof(recv_buff), 0);
        printf("[server]: %s", recv_buff);
        if (strcmp(send_buff, "bye\n") == 0) {
            break;
        }
    }

    close(socket_fd);

    return err;
}
