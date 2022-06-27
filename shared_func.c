#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "json.h"
#include "shared_func.h"

int connect_to_server(char *hostname, int port)
{
    int sockfd;
    struct sockaddr_in saddr;

    struct hostent *host_ptr;
    if ((host_ptr = gethostbyname(hostname)) == NULL)
    {
        perror("Error Resolving.\n");
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error creating socket.\n");
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    memcpy((char *)&saddr.sin_addr.s_addr, host_ptr->h_addr_list[0], host_ptr->h_length);
    saddr.sin_port = htons(port);

    setsockopt(sockfd, SOL_SOCKET,
               SO_REUSEADDR, &(int){1},
               sizeof(int));

    if (connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        perror("Error connecting.\n");
        return 1;
    }

    int sockfl = fcntl(sockfd, F_GETFL, 0);
    sockfl |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, sockfl);

    char ip_addr[32];
    printf("Connected to %s\n", inet_ntop(host_ptr->h_addrtype, host_ptr->h_addr_list[0], ip_addr, sizeof(ip_addr)));

    return sockfd;
}

int validate_mesage(char *msg, int sender)
{
    char buffer[1024];
    int valid_msg = 1;
    while (msg[strlen(msg) - 1] != '}')
    {
        recv(sender, buffer, sizeof(buffer), 0);
        if (strlen(msg) + strlen(buffer) < 1024)
        {
            strncat(msg, buffer, strlen(buffer));
        }
        if (!json_validate(msg))
        {
            valid_msg = 0;
        }
        else
        {
            valid_msg = 0;
            break;
        }
    }
    return valid_msg;
}

void request_encryption_server(char *method, char *message, int encryptfd, int receiver)
{
    /*
    This function creates the payload to send to the encryption server

    Payload structure:
    {
        "receiver": receiver,
        "method": method,
        "message": message
    }
    - receiver is an integer to indicate the receiver(s)
    - method is either:
        + ENCRYPT => Request encryption server to encrypt the message
        + DECRYPT => Request encryption server to decrypt the message
    - message is the message to be encrypted or decrypted
    */

    // Create the payload
    JsonNode *request_json = json_mkobject();
    JsonNode *receiver_json = json_mknumber(receiver);
    JsonNode *method_json = json_mkstring(method);
    JsonNode *raw_message_json = json_mkstring(message);
    json_append_member(request_json, "receiver", receiver_json);
    json_append_member(request_json, "method", method_json);
    json_append_member(request_json, "message", raw_message_json);

    // Send the payload to the encryption server
    char *request_buffer = json_encode(request_json);
    // printf("Request payload: %s\n", request_buffer);
    send(encryptfd, request_buffer, strlen(request_buffer) + 1, 0);
}
