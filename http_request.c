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