#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include "client.h"

#define MAX_LENGTH 255
#define IRC_DEFAULT_PORT 8784
#define ENC_DEFAULT_PORT 4443
#define DEFAULT_HOST "localhost"

int main(int argc, char **argv)
{
    // Remove stdout buffer => Always print
    setbuf(stdout, NULL);

    int irc_port = IRC_DEFAULT_PORT;
    char irc_host[MAX_LENGTH] = DEFAULT_HOST;
    int enc_port = ENC_DEFAULT_PORT;
    char enc_host[MAX_LENGTH] = DEFAULT_HOST;

    if (argc > 1)
    {
        strncpy(irc_host, argv[1], MAX_LENGTH);
    }

    if (argc > 2)
    {
        irc_port = atoi(argv[2]);
    }

    int ircfd, encryptfd;
    ircfd = connect_to_server(irc_host, irc_port);
    encryptfd = connect_to_server(enc_host, enc_port);
    printf("> ");

    struct pollfd stdin_pollfd;
    stdin_pollfd.fd = fileno(stdin);
    stdin_pollfd.events = POLLIN;

    char id[10];
    char message[1024];

    int connected = 1;
    int irc_status, enc_status;
    while (connected)
    {
        char username[MAX_LENGTH];
        irc_status = recv(ircfd, message, sizeof(message), 0);
        if (irc_status > 0)
        {
            int valid_msg = validate_mesage(message, ircfd);
            if (valid_msg)
            {
                // Handle messange receiving logic
                handle_server_response(message, encryptfd);
                printf("> ");
            }
        }
        else if (irc_status == 0)
        {
            printf("\rChat server disconnected.\n");
            break;
        }

        enc_status = recv(encryptfd, message, sizeof(message), 0);
        if (enc_status > 0)
        {
            int valid_msg = validate_mesage(message, ircfd);
            if (valid_msg)
            {
                // Handle message receiving logic
                handle_encryption_response(message, ircfd, username);
            }
        }
        else if (enc_status == 0)
        {
            printf("\rEncryption server disconnected.\n");
            break;
        }

        if (poll(&stdin_pollfd, 1, 0) > 0)
        {
            if (stdin_pollfd.revents & POLLIN)
            {
                fgets(message, sizeof(message), stdin);
                if (strcmp(message, "/quit\n") == 0)
                {
                    connected = 0;
                    break;
                }

                // Handle message sending logic
                handle_send_request(encryptfd, message, id, username);

                if (connected)
                {
                    printf("> ");
                }
            }
        }
    }
    close(ircfd);
    return 0;
}
