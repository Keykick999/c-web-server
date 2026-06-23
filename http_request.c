#include "http_request.h"

#include <stdlib.h>

void destroyHttpRequest(
    HttpRequest* request
) {

    free(request->method);
    free(request->path);
    free(request->version);

    destroy_map(
        request->headers
    );

    destroy_map(
        request->body
    );
}

void initHttpRequest(HttpRequest *request) {

    request->method = NULL;
    request->path = NULL;
    request->version = NULL;

    request->headers = create_map();
    request->body = create_map();
}