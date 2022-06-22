#pragma once
#ifndef MAX_CLIENT
#define MAX_CLIENT 100
#endif

int connect_to_server(char *hostname, int port);
int validate_mesage(char *msg, int sender);
void request_encryption_server(char *method, char *message, int encryptfd, int receiver);
