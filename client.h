#ifndef CLIENT_H
#define CLIENT_H

#include "http_request.h"

#define MAX_HEADERS 4096
#define MAX_BODY 4096

typedef struct Client {

    int client_fd;

    char buffer[MAX_HEADERS];
    char headers[MAX_HEADERS];
    char body[MAX_BODY];

    int body_len;
    int header_len;
    int contentLength;

    HttpRequest request;

    struct Client *next;

} Client;

void initClient(Client *client, int client_fd);
void destroyClient(Client *client);

#endif