#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "shared_func.h"
#include "json.h"

void server_handler(char *payload, int *clientfds, int sender)
{
    printf("Sender: %d - Payload: %s", sender, payload);
}