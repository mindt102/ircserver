#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sqlite3.h>
#include "shared_func.h"
#include "json.h"
#include "auth.sqlite.h"

#if defined(__APPLE__)
#define COMMON_DIGEST_FOR_OPENSSL
#include <CommonCrypto/CommonDigest.h>
#define SHA1 CC_SHA1
#else
#include <openssl/sha.h>
#endif

void sha256_string(char *string, char *outputBuffer)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, string, strlen(string));
    SHA256_Final(hash, &sha256);
    int i = 0;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

void response(int sender, JsonNode *request_json, JsonNode *method, int isError, char *msg)
{
    int receiver = json_find_member(request_json, "receiver")->number_;

    JsonNode *response_json = json_mkobject();
    JsonNode *status;
    JsonNode *msgJson = json_mkstring(msg);
    JsonNode *receiverJs = json_mknumber(receiver);

    if (isError == 0)
    {
        status = json_mkstring("SUCCESS");
        json_append_member(response_json, "username", msgJson);
    }
    else
    {
        status = json_mkstring("FAIL");
        json_append_member(response_json, "error", msgJson);
    }

    json_append_member(response_json, "receiver", receiverJs);
    json_append_member(response_json, "method", method);
    json_append_member(response_json, "status", status);

    char *response_buffer = json_encode(response_json);
    send(sender, response_buffer, strlen(response_buffer) + 1, 0);
}

void handleRegister(int sender, JsonNode *request_json)
{
    JsonNode *method = json_mkstring("REGISTER");
    char *username = json_find_member(request_json, "username")->string_;
    char *password = json_find_member(request_json, "password")->string_;
    User *user = selectUser(username);
    if (user->username == NULL)
    {
        // SHA password
        char hash[65];
        sha256_string(password, hash);
        int sttInsert = insertUser(username, hash);
        if (sttInsert > 0)
        {
            return response(sender, request_json, method, 1, "Something went wrong when insert");
        }
        return response(sender, request_json, method, 0, username);
    }
    else
    {
        response(sender, request_json, method, 1, "Username already exists");
    }
}

void handleLogin(int sender, JsonNode *request_json)
{
    JsonNode *method = json_mkstring("LOGIN");
    char *username = json_find_member(request_json, "username")->string_;
    char *password = json_find_member(request_json, "password")->string_;
    User *user = selectUser(username);
    if (user->username == NULL)
    {
        return response(sender, request_json, method, 1, "User not found!");
    }
    else
    {
        char hash[65];
        sha256_string(password, hash);
        if (strcmp(hash, user->password) == 0)
        {
            return response(sender, request_json, method, 0, username);
        }
        else
        {
            return response(sender, request_json, method, 1, "User not found!");
        }
    }
}

void server_handler(char *payload, int sender)
{
    JsonNode *request_json = json_decode(payload);
    char *method = json_find_member(request_json, "method")->string_;
    if (strcmp(method, "REGISTER") == 0)
    {
        handleRegister(sender, request_json);
    }
    else if ((strcmp(method, "LOGIN") == 0))
    {
        handleLogin(sender, request_json);
    }
}