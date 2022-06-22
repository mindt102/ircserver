#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "shared_func.h"
#include "json.h"

void handle_request(char *request, int senderfd)
{
    /*
    This functions receive a request to encrypt or decrypt
    and then send back the result
    */
    // printf("Request: %s\n", request);

    // Decode the request from string to json
    JsonNode *request_json = json_decode(request);
    int receiver = json_find_member(request_json, "receiver")->number_;
    char *method = json_find_member(request_json, "method")->string_;
    char *request_message = json_find_member(request_json, "message")->string_;

    // TODO: Check if method is ENCRYPT => Encrypt the message
    // TODO: Check if method is DECRYPT => Decrypt the message

    char *response_message = request_message;

    // Create the response payload
    JsonNode *response_json = json_mkobject();
    JsonNode *client_fd_json = json_mknumber(receiver);
    JsonNode *method_json = json_mkstring(method);
    JsonNode *message_json = json_mkstring(response_message);
    json_append_member(response_json, "receiver", client_fd_json);
    json_append_member(response_json, "method", method_json);
    json_append_member(response_json, "message", message_json);

    // Encode the json response to string
    char *response_buffer = json_encode(response_json);
    printf("Response: %s\n", response_buffer);

    // Send back the encrypted or decrypted payload
    send(senderfd, response_buffer, strlen(response_buffer) + 1, 0);
}