#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include "enc.server.h"

#define DEFAULT_PORT 4443

int main(int argc, char **argv)
{
    setbuf(stdout, NULL);

    int clientfds[MAX_CLIENT];
    memset(clientfds, 0, sizeof(clientfds));

    int port = DEFAULT_PORT;
    if (argc > 1)
    {
        port = atoi(argv[1]);
    }

    int sockfd, clen, clientfd;
    struct sockaddr_in saddr, caddr;
    clen = sizeof(caddr);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Error creating socket\n");
        return 1;
    }

    setsockopt(sockfd, SOL_SOCKET,
               SO_REUSEADDR, &(int){1},
               sizeof(int));

    int sockfl = fcntl(sockfd, F_GETFL, 0);
    sockfl |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, sockfl);

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        perror("Error binding");
        return 1;
    }

    if (listen(sockfd, 5) < 0)
    {
        perror("Error listening");
        return 1;
    }

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
                    handle_request(message, clientfds[i]);
                }
                else if (read_status == 0)
                {
                    printf("\rClient %d has disconnected.\n", clientfds[i]);
                    close(clientfds[i]);
                    clientfds[i] = 0;
                }
            }
        }
    }
    close(sockfd);
    return 0;
}
