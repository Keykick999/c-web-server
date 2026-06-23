#include <string.h>
#include "client.h"

void initClient(Client *client, int client_fd)
{
    client->client_fd = client_fd;

    memset(client->buffer, 0, sizeof(client->buffer));
    memset(client->headers, 0, sizeof(client->headers));
    memset(client->body, 0, sizeof(client->body));

    client->header_len = 0;
    client->body_len = 0;
    client->contentLength = -1;

    client->request.method = NULL;
    client->request.path = NULL;
    client->request.version = NULL;

    client->request.headers = create_map();
    client->request.body = create_map();

    client->request.header_count = 0;

    client->next = NULL;
}

void destroyClient(Client *client)
{
    if (client == NULL)
        return;

    destroyHttpRequest(&client->request);
}