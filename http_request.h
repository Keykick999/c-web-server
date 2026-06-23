#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "hashmap.h"

typedef struct HttpRequest {

    char* method;
    char* path;
    char* version;

    HashMap* headers;
    int header_count;

    HashMap* body;

} HttpRequest;

void destroyHttpRequest(
    HttpRequest* request
);

void initHttpRequest(
    HttpRequest *request
);

#endif