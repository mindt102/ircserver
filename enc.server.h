#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include "shared_func.h"
#include "json.h"

// char message[1024];
long int temp[1024]; // do something
int e;               // public key
int d;               // private key
int m;               // m = p * q
static const char use[51] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@$&*()_+=-0987654321|?.,";

// Base64
static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '-', '_'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

void build_decoding_table()
{

    decoding_table = malloc(256);

    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char)encoding_table[i]] = i;
}

void base64_cleanup()
{
    free(decoding_table);
}

char *base64_encode(unsigned char *data,
                    size_t input_length,
                    size_t *output_length)
{

    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = malloc(*output_length + 1);
    memset(encoded_data, '\0', *output_length);
    if (encoded_data == NULL)
        return NULL;

    for (int i = 0, j = 0; i < input_length;)
    {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
        // printf("%d\n", j);
    }

    // int i=0;
    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';
    encoded_data[*output_length] = '\0';

    // for (int i = 0; i <= strlen(encoded_data); i++)
    // {
    //     printf("j=%d, encoded=%c\n", i, encoded_data[i]);
    // }
    printf("Input length: %ld\n", input_length);
    printf("Output length: %ld\n", *output_length);
    printf("Encoded data: %s\n", encoded_data);
    return encoded_data;
}

unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length)
{

    if (decoding_table == NULL)
        build_decoding_table();
    if (input_length % 4 != 0)
        return NULL;
    *output_length = input_length / 4 * 3;

    if (data[input_length - 1] == '=')
        (*output_length)--;
    if (data[input_length - 2] == '=')
        (*output_length)--;
    unsigned char *decoded_data = malloc(*output_length);
    if (decoded_data == NULL)
        return NULL;

    for (int i = 0, j = 0; i < input_length;)
    {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6) + (sextet_b << 2 * 6) + (sextet_c << 1 * 6) + (sextet_d << 0 * 6);

        if (j < *output_length)
            decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length)
            decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length)
            decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}
// End of Base64

// a ^ b mod n
int Calculate(int a, int b, int n)
{
    long long x = 1, y = a;
    while (b > 0)
    {
        if (b % 2 == 1)
        {
            x = (x * y) % n; // multiplying with base
        }
        y = (y * y) % n; // squaring the base
        b /= 2;
    }
    return x % n;
}
int gcd(int a, int b)
{
    if (b == 0)
        return a;
    a %= b;
    return gcd(b, a);
}

int coprime(int a, int b)
{

    if (gcd(a, b) == 1)
        return 1; // true
    else
        return 2; // false
}

char *encrypt(char *raw_message)
{
    // char *encryp = (char *)malloc(1024); // to store encryp msg
    long int pt1, pt2; // pointer1 , pointer2
    long int len = strlen(raw_message);
    int *encrypted_msg = (int *)malloc(len * sizeof(int));
    int i = 0;
    while (i < len)
    {
        pt1 = raw_message[i];
        pt1 = pt1;
        int k = Calculate(pt1, e, m); // pt1 ^ e mod m
        temp[i] = k;
        pt2 = k;
        encrypted_msg[i] = pt2;
        printf("pt2 = %ld, raw[%d] = %c\n", pt2, i, raw_message[i]);
        i++;
    }

    size_t message_size = strlen(raw_message);
    char *encode_data = base64_encode((char *)encrypted_msg, (len + 1) * sizeof(int), &len);
    free(encrypted_msg);
    printf("Encrypt result: %s\n", encode_data);
    return encode_data;
}

char *decrypt(char *encoded_message) // Base64 message
{
    char *decryp = malloc(sizeof(char) * 1024); // to store decryp msg

    long int pt1, pt2; // pointer1, pointer2
    int i = 0;
    size_t message_size = strlen(encoded_message);
    size_t *decoded_size = (size_t *)malloc(sizeof(size_t));
    int *encrypted_msg = (int *)malloc((message_size + 1) * sizeof(int));

    encrypted_msg = (int *)base64_decode(encoded_message, message_size, decoded_size);
    printf("Decoded size: %ld\n", *decoded_size);

    for (i; i < ((*decoded_size) / 4 - 1); i++)
    {
        pt2 = encrypted_msg[i];
        int k = Calculate(pt2, d, m); // ct ^ d mod m
        pt1 = k;
        decryp[i] = pt1;
        printf("pt2 = %ld, decryp[%d] = %c\n", pt2, i, decryp[i]);
    }
    decryp[i] = '\0';
    free(encrypted_msg);
    printf("Decrypt result: %s\n", decryp);
    return decryp;
}

void Gen_key()
{
    int p = 31;
    int q = 23;
    long int n = (p - 1) * (q - 1);
    m = p * q;
    for (int i = 2; i < n; i++)
    {
        for (int j = 2; j < 10; j++)
        {
            if (coprime(i, n) == 1 && coprime(i, m) == 1 && j != i && (j * i) % n == 1)
            {
                d = i; // found private key
                e = j; // found public key
                break;
            }
        }
    }
}

void handle_request(char *request, int senderfd)
{
    /*
    This functions receive a request to encrypt1 or decrypt
    and then send back the result
    */
    // printf("\nRequest: %s\n", request);

    // Decode the request from string to json
    JsonNode *request_json = json_decode(request);
    int receiver = json_find_member(request_json, "receiver")->number_;
    char *method = json_find_member(request_json, "method")->string_;
    char *request_message = json_find_member(request_json, "message")->string_;

    // Encrypt the message
    // TODO: Check if method is ENCRYPT => Encrypt the message
    // TODO: Check if method is DECRYPT => Decrypt the message
    int compare;
    char *check = "ENCRYPT";
    compare = strcmp(check, method);
    printf("Request message: %s\n", request_message);

    if (compare == 0)
    {
        Gen_key();
        char response_message[1024];
        // char *response_message = encrypt(request_message);
        char *temp = encrypt(request_message);
        strcpy(response_message, temp);
        printf("Response message: %s, len %ld\n", response_message, strlen(response_message));
        // Create the response payload
        JsonNode *response_json = json_mkobject();
        JsonNode *message_json = json_mkstring(response_message);
        JsonNode *client_fd_json = json_mknumber(receiver);
        JsonNode *method_json = json_mkstring(method);
        json_append_member(response_json, "message", message_json);
        json_append_member(response_json, "receiver", client_fd_json);
        json_append_member(response_json, "method", method_json);

        // Encode the json response to string
        char *buffer = json_encode(response_json);
        printf("Encrypted: %s\n", buffer);

        // Send back the encrypted or decrypted payload
        send(senderfd, buffer, strlen(buffer) + 1, 0);
        // free(response_message);
        free(temp);
        // memset(message, sizeof(message),0);
    }
    else
    {

        // char response_message[1024];
        char *response_message = decrypt(request_message);
        // char *temp = decrypt(request_message);
        // memset(response_message, '\0', 1024);
        // strcpy(response_message, temp);
        // response_message[328] = '\0';
        printf("Response message: %s\n", response_message);
        // Create the response payload
        JsonNode *response_json = json_mkobject();
        JsonNode *client_fd_json = json_mknumber(receiver);
        JsonNode *method_json = json_mkstring(method);
        JsonNode *message_json = json_mkstring(response_message);
        json_append_member(response_json, "message", message_json);
        json_append_member(response_json, "receiver", client_fd_json);
        json_append_member(response_json, "method", method_json);

        // Encode the json response to string
        char *response_buffer = json_encode(response_json);
        printf("Decrypted for %d: %s\n", senderfd, response_buffer);

        // Send back the encrypted or decrypted payload
        send(senderfd, response_buffer, strlen(response_buffer) + 1, 0);
        // free(response_message);
    }
}