#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include "irc.server.h"

#define MAX_LENGTH 255
#define DEFAULT_PORT 8784

int main(int argc, char **argv)
{
    // Remove stdout buffer => Always print to stdout
    setbuf(stdout, NULL);

    // Initialize arrays to store all clients' file descriptor
    int clientfds[MAX_CLIENT], authed_clients[MAX_CLIENT];
    int sockfl;
    memset(clientfds, 0, sizeof(clientfds));
    memset(authed_clients, 0, sizeof(authed_clients));

    // Define the port
    int port = DEFAULT_PORT;
    if (argc > 1)
    {
        port = atoi(argv[1]);
    }

    // Connect to another IRC server
    if (argc > 2)
    {
        int remoteport = atoi(argv[3]);
        char hostname[MAX_LENGTH];
        strncpy(hostname, argv[2], MAX_LENGTH);

        int remotefd = connect_to_server(hostname, remoteport);

        sockfl = fcntl(remotefd, F_GETFL, 0);
        sockfl |= O_NONBLOCK;
        fcntl(remotefd, F_SETFL, sockfl);

        clientfds[0] = remotefd;
        authed_clients[0] = 1;

        JsonNode *init_payload = json_mkobject();
        json_append_member(init_payload, "method", json_mkstring("INIT"));
        char* buffer = json_encode(init_payload);
        send(remotefd, buffer, strlen(buffer) + 1, 0);
    }

    int sockfd, clen, clientfd;
    struct sockaddr_in saddr, caddr;
    clen = sizeof(caddr);

    // Connect to the authentication server
    int authfd = connect_to_server("localhost", 4444);
    sockfl = fcntl(authfd, F_GETFL, 0);
    sockfl |= O_NONBLOCK;
    fcntl(authfd, F_SETFL, sockfl);
    clientfds[1] = authfd;
    // int authfd = 0;

    // Connect to the encryption server
    int encryptfd = connect_to_server("localhost", 4443);
    sockfl = fcntl(encryptfd, F_GETFL, 0);
    sockfl |= O_NONBLOCK;
    fcntl(encryptfd, F_SETFL, sockfl);
    clientfds[2] = encryptfd;

    // Create the main socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Error creating socket\n");
        return 1;
    }

    // Make socket become non blocking
    setsockopt(sockfd, SOL_SOCKET,
               SO_REUSEADDR, &(int){1},
               sizeof(int));
    sockfl = fcntl(sockfd, F_GETFL, 0);
    sockfl |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, sockfl);

    // Configure the address and port
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(port);

    // Bind the socket to the address and port
    if (bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        perror("Error binding");
        return 1;
    }

    // Make socket ready to listen to incomming connection
    if (listen(sockfd, 5) < 0)
    {
        perror("Error listening");
        return 1;
    }

    // Prepare poll to check for data in stdin
    struct pollfd stdin_pollfd;
    stdin_pollfd.fd = fileno(stdin);
    stdin_pollfd.events = POLLIN;

    int running = 1;
    char message[1024];

    printf("Listening on port %d...\n", port);

    while (running)
    {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(sockfd, &set);
        int maxfd = sockfd;

        for (int i = 0; i < MAX_CLIENT; i++)
        {
            if (clientfds[i] > 0)
                FD_SET(clientfds[i], &set);
            if (clientfds[i] > maxfd)
                maxfd = clientfds[i];
        }

        select(maxfd + 1, &set, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &set))
        {
            clientfd = accept(sockfd, (struct sockaddr *)&caddr, &clen);

            char ip_addr[32];
            printf("Client %s:%d connected\n", inet_ntop(caddr.sin_family, &caddr.sin_addr.s_addr, ip_addr, sizeof(ip_addr)), ntohs(caddr.sin_port));

            int clientfl = fcntl(clientfd, F_GETFL, 0);
            clientfl |= O_NONBLOCK;
            fcntl(clientfd, F_SETFL, clientfl);

            for (int i = 0; i < MAX_CLIENT; i++)
            {
                if (clientfds[i] == 0)
                {
                    clientfds[i] = clientfd;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENT; i++)
        {
            if (clientfds[i] > 0 && FD_ISSET(clientfds[i], &set))
            {
                int read_status;
                read_status = recv(clientfds[i], message, sizeof(message), 0);
                if (read_status > 0)
                {
                    char buffer[1024];
                    int valid_msg = 1;
                    while (message[strlen(message) - 1] != '}')
                    {
                        recv(clientfds[i], buffer, sizeof(buffer), 0);
                        if (strlen(message) + strlen(buffer) < 1024)
                        {
                            strncat(message, buffer, strlen(buffer));
                        }
                        else
                        {
                            valid_msg = 0;
                            break;
                        }
                    }
                    if (!json_validate(message))
                    {
                        valid_msg = 0;
                    }
                    if (!valid_msg)
                    {
                        strcpy(message, "Invalid message\n");
                        send(clientfds[i], message, strlen(message) + 1, 0);
                        continue;
                    }

                    // Handle all server logic
                    server_handler(message, clientfds, authed_clients, clientfds[i], authfd, encryptfd);
                }
                else if (read_status == 0)
                {
                    printf("\rClient %d has disconnected.\n", clientfds[i]);
                    close(clientfds[i]);
                    clientfds[i] = 0;
                    authed_clients[i] = 0;
                }
            }
        }
    }
    close(sockfd);
    return 0;
}
