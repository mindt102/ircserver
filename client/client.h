#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "shared_func.h"
#include "json.h"

#define BUFFER_SIZE 1024

void handle_send_request(int encryptfd, char *message, char *clientid, char *usrname)
{
    /*
    This function take user's input, create a payload and send the payload to the encryption server.

    Payload structure:
    {
        "receiver": receiver,
        "method": "ENCRYPT",
        "message": message
    }
    - receiver is either 0 or 1. 0 stands for BROADCAST method, 1 stands for UNICAST method.
    - message: depends on the users'commands. Can be username, password to login & register, or message content to send to others.
    */

    // TODO: Parse message to check for register or login command
    char *tempMsg = message;
    // printf("%s\n", tempMsg);

    // TODO:
    // => Create register payload
    // => Send to encryption server
    if (strcmp(tempMsg, "REGISTER\n") == 0)
    {
        char get_usr[BUFFER_SIZE], get_pass[BUFFER_SIZE];
        printf("-------------------------------------\n");
        printf("REGISTER SCREEN\n");
        printf("Input username: ");
        fgets(get_usr, BUFFER_SIZE, stdin);
        get_usr[strlen(get_usr) - 1] = '\0';
        // printf("Username: %s, len: %ld\n", get_usr, strlen(get_usr));
        printf("Input password: ");
        fgets(get_pass, BUFFER_SIZE, stdin);
        get_pass[strlen(get_pass) - 1] = '\0';

        JsonNode *register_payload = json_mkobject();
        JsonNode *method_register = json_mkstring("REGISTER");
        JsonNode *method_username = json_mkstring(get_usr);
        JsonNode *method_password = json_mkstring(get_pass);
        json_append_member(register_payload, "method", method_register);
        json_append_member(register_payload, "username", method_username);
        json_append_member(register_payload, "password", method_password);
        char *register_temp = json_encode(register_payload);
        // printf("Raw register: %s\n", register_temp);
        request_encryption_server("ENCRYPT", register_temp, encryptfd, 1);
    }
    // TODO: Handle login commands
    // => Create login payload
    // => Send to encryption server
    else if (strcmp(tempMsg, "LOGIN\n") == 0)
    {

        char get_usr[BUFFER_SIZE], get_pass[BUFFER_SIZE];
        printf("-------------------------------------\n");
        printf("LOGIN SCREEN\n");
        printf("Input username: ");
        fgets(get_usr, BUFFER_SIZE, stdin);
        get_usr[strlen(get_usr) - 1] = '\0';
        printf("Input password: ");
        fgets(get_pass, BUFFER_SIZE, stdin);
        get_pass[strlen(get_pass) - 1] = '\0';

        JsonNode *login_payload = json_mkobject();
        JsonNode *method_login = json_mkstring("LOGIN");
        JsonNode *method_username = json_mkstring(get_usr);
        JsonNode *method_password = json_mkstring(get_pass);
        json_append_member(login_payload, "method", method_login);
        json_append_member(login_payload, "username", method_username);
        json_append_member(login_payload, "password", method_password);
        char *login_temp = json_encode(login_payload);

        // printf("Raw login: %s\n", login_temp);
        request_encryption_server("ENCRYPT", login_temp, encryptfd, 1);
    }

    else
    {
        if (strlen(usrname) == 0)
        {
            printf("Type LOGIN or REGISTER to start.\n");
            return;
        }
        // Create the payload for sending message to others
        JsonNode *payload_json = json_mkobject();                    // {}
        JsonNode *method_json = json_mkstring("MESSAGE");            // "MESSAGE"
        JsonNode *username_json = json_mkstring(usrname);            // "admin"
        JsonNode *message_json = json_mkstring(message);             // "randomtext"
        json_append_member(payload_json, "method", method_json);     // {"method": "MESSAGE"}
        json_append_member(payload_json, "username", username_json); // {"method": "MESSAGE", "username": "admin"}
        json_append_member(payload_json, "message", message_json);   // {"method": "MESSAGE", "username": "admin", "message": "randomtext"}

        // Send the payload to encrypt server
        char *payload_buffer = json_encode(payload_json);
        // printf("Raw request: %s\n", payload_buffer);
        request_encryption_server("ENCRYPT", payload_buffer, encryptfd, 0);
    }
}

void handle_server_response(char *payload, int encryptfd)
{
    /*
    This function take the payload from the
    IRC server and send to the encryption server
    */

    // printf("Payload from server: %s\n", payload);
    // Extract the encrypted message from the payload
    JsonNode *payload_json = json_decode(payload);
    char *message = json_find_member(payload_json, "message")->string_;
    // printf("Encrypted message: %s\n", message);

    // Call this function to send the message to the encryption server
    // printf("Send to decrypt: %s\n", message);
    request_encryption_server("DECRYPT", message, encryptfd, 0);
}

void handle_encryption_response(char *payload, int ircfd, char *usrname)
{
    /*
    This function handle the payload received from the encryption server.
    The method of the incoming method can either be:
    - DECRYPT => The payload is the response of a DECRYPT request, contains the message to print
    - ENCRYPT => The payload is the response of a ENCRYPT request, contains encrypted payload to send to IRC server
    */

    // Decode the payload to json

    // printf("Payload from encryption: %s\n", payload);
    JsonNode *payload_json = json_decode(payload);
    int receiver = json_find_member(payload_json, "receiver")->number_;
    char *method = json_find_member(payload_json, "method")->string_;
    char *message = json_find_member(payload_json, "message")->string_;
    // Check if method is ENCRYPT
    if (strcmp(method, "ENCRYPT") == 0)
    {
        // Make the outgoing payload
        JsonNode *outgoing_payload = json_mkobject();
        char *outgoing_method = (receiver == 0) ? "BROADCAST" : "UNICAST";
        JsonNode *method_json = json_mkstring(outgoing_method);
        JsonNode *message_json = json_mkstring(message);
        json_append_member(outgoing_payload, "method", method_json);
        json_append_member(outgoing_payload, "message", message_json);

        // Send the outgoing payload to IRC server
        char *buffer = json_encode(outgoing_payload);
        // printf("Sending to IRC: %s\n", buffer);
        send(ircfd, buffer, strlen(buffer) + 1, 0);
    }
    //  Otherwise if method is DECRYPT
    else if (strcmp(method, "DECRYPT") == 0)
    {
        // Decode the incoming message into json
        // printf("Decrypted message: %s\n", message);
        JsonNode *message_json = json_decode(message);
        // Extract the username and content
        char *RspnMethod = json_find_member(message_json, "method")->string_;
        // Display the received message to stdout
        if (strcmp(RspnMethod, "MESSAGE") == 0)
        {
            char *message_content = json_find_member(message_json, "message")->string_;
            char *usrname_content = json_find_member(message_json, "username")->string_;
            printf("\r%s: %s> ", usrname_content, message_content);
        }
        else if ((strcmp(RspnMethod, "LOGIN") == 0) || (strcmp(RspnMethod, "REGISTER") == 0))
        {
            JsonNode *login_json = json_decode(message);
            char *status_content = json_find_member(login_json, "status")->string_;
            if (strcmp(status_content, "SUCCESS") == 0)
            {
                char *username_content = json_find_member(login_json, "username")->string_;
                strcpy(usrname, username_content);
                printf("\rSUCCESS: %s\n", username_content);
            }
            else if (strcmp(status_content, "FAIL") == 0)
            {
                char *error_content = json_find_member(login_json, "error")->string_;
                printf("\rFAILED: %s\n", error_content);
            }
            printf("> ");
        }
    }
}
