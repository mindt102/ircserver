#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "shared_func.h"
#include "json.h"

void broadcast(char *message, int *clientfds, int encryptfd, int authfd, int senderfd)
{
    /*
    This function broadcast a message to all clients except for
    the sender, the encryption server and the authentication server
    */

    // Loop through each file descriptor
    for (int i = 0; i < MAX_CLIENT; i++)
    {
        int clientfd = clientfds[i];
        // Send the message if file the descriptor belongs to a valid client
        if (clientfd != 0 && clientfd != encryptfd && clientfd != authfd && clientfd != senderfd)
        {
            send(clientfd, message, strlen(message) + 1, 0);
        }
    }
}

void handle_authentication_response(char *payload, int encryptfd)
{
    /*
    This function handle the responses from the authentication server and send to encryption server
    Incoming payload structure:
    {
        "method": "LOGIN" or "REGISTER",
        "status": "SUCCESS" or "FAIL",
        "username" or "error": "admin" or "Incorrect username"
    }
    */

    //HaTrang: TODO: Create the payload and send to the encryption server to ENCRYPT
    JsonNode *payload_json = json_decode(payload);
    int receiver = json_find_member(payload_json, "receiver")->number_;
    char *method = json_find_member(payload_json, "method")->string_;
    char *status = json_find_member(payload_json, "status")->string_;

    JsonNode *message_json = json_mkobject();
    JsonNode *method_json = json_mkstring(method);
    JsonNode *status_json = json_mkstring(status);
    json_append_member(message_json, "method", method_json);
    json_append_member(message_json, "status", status_json);

    if (status == "FAIL"){
        char *error = json_find_member(payload_json,"error")->string_;
        JsonNode *error_json = json_mkstring(error);
        json_append_member(message_json, "error", error_json);
    }
    else if (status == "SUCCESS"){
        char *username = json_find_member(payload_json,"username")->string_;
        JsonNode *username_json = json_mkstring(username);
        json_append_member(message_json, "username", username_json);
    }
    char *message = json_encode_string(message_json);
    
    request_encryption_server("ENCRYPT", message, encryptfd, receiver);
    
}

void handle_unicast_request(char *payload, int encryptfd, int senderfd)
{
    //HaTrang TODO: call function to DECRYPT
    JsonNode *payload_json = json_decode(payload);
    char *message = json_find_member(payload_json, "message")->string_;
    request_encryption_server("DECRYPT", message, encryptfd, senderfd);
}

void handle_client_request(char *payload, int encryptfd, int clientfd)
{
    /*
    This function take the login or register request from the client and send to authentication server.

    Incoming payload structure:
    {
        "method": method,
        "username": username,
        "password": password
    }
    - method is either LOGIN or REGISTER
    - username is the username of the client
    - password is the password of the client
    */
    JsonNode *payload_json = json_decode(payload);
    char *method = json_find_member(payload_json, "method")->string_;
    char *message = json_find_member(payload_json, "message")->string_;
    // printf("Decrypted message: %s\n", message);
    // TODO: Creat the payload and send to authentication server
}

void handle_encryption_response(char *payload, int *clientfds, int encryptfd, int authfd)
{
    /*
    This function handle the payload received from the encryption server.
    The method of the incoming method can either be:
    - DECRYPT => The payload is the response of a DECRYPT request, contains the login or register message to send to authentication server
    - ENCRYPT => The payload is the response of a ENCRYPT request, contains encrypted payload to send to the client
    */

    // Decode the incoming payload
    JsonNode *payload_json = json_decode(payload);
    int receiver = json_find_member(payload_json, "receiver")->number_;
    char *method = json_find_member(payload_json, "method")->string_;
    char *message = json_find_member(payload_json, "message")->string_;

    if (strcmp(method, "DECRYPT") == 0)
    {
        // Client request to login or register
        handle_client_request(message, encryptfd, receiver);
    }
    else if (strcmp(method, "ENCRYPT") == 0)
    {
        // The encrypted payload of the login or register result
        // TODO: Create the payload and send to the receiver (i.e. the client)
        // 1. Extract the encrypted message
        // 2. Extract the receiver
        // 3. Create the payload and send the encrypted message to the receiver
    }
}

void server_handler(char *payload, int *clientfds, int senderfd, int authfd, int encryptfd)
{
    /*
    This function is the main function to handle all incoming requests to IRC server.
    Request can come from:
    - Authentication server: Unencrypted, login or register result
    - Encryption server: Unencrypted, encrypted or decrypted result
    - Client: Encrypted, can have the method of:
        + BROADCAST => Broadcast this message to all other clients
        + UNICAST => Send this message to encryption server to decrypt
    */

    // printf("Received payload from %d: %s", senderfd, payload);
    JsonNode *received_payload = json_decode(payload);
    char *method = json_find_member(received_payload, "method")->string_;

    if (senderfd == authfd)
    {
        /* Payload comes from authentication server */
        handle_authentication_response(payload, encryptfd);
    }
    else if (senderfd == encryptfd)
    {
        /* Payload comes from authentication server */
        handle_encryption_response(payload, clientfds, encryptfd, authfd);
    }
    else
    {
        /* Encrypted payload comes from clients */
        if (strcmp(method, "BROADCAST") == 0)
        {
            printf("Broadcast payload: %s\n", payload);
            broadcast(payload, clientfds, encryptfd, authfd, senderfd);
        }
        else if (strcmp(method, "UNICAST") == 0)
        {
            char *encrypted_message = json_find_member(received_payload, "message")->string_;
            handle_unicast_request(encrypted_message, encryptfd, senderfd);
            // printf("Encrypted message: %s\n", encrypted_message);
        }
    }
}
