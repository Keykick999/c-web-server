#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "http_request.h"

char* parseRequestLine(
    HttpRequest* request,
    char* buffer
);

char* parseHeaders(
    HttpRequest* request,
    char* headerStart
);

void parseBody(
    HttpRequest* request,
    char* bodyStart
);

HttpRequest parseHttpRequest(
    char* buffer
);

#endif