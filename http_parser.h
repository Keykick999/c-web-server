#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "http_request.h"

HttpRequest parseHttpRequest(
    char* buffer
);

#endif