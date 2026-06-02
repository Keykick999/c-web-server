#include <stdio.h>

#include "http_parser.h"

int main() {
    char buffer[] =
    "POST /login HTTP/1.1\r\n"
    "Host: localhost:8080\r\n"
    "User-Agent: Mozilla/5.0\r\n"
    "Accept: */*\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: 27\r\n"
    "Connection: keep-alive\r\n"
    "\r\n"
    "username=kim&password=123";

    HttpRequest request =
        parseHttpRequest(buffer);

    printf("method: %s\n", request.method);
    printf("path: %s\n", request.path);
    printf("version: %s\n", request.version);

    print_map(request.headers);
    print_map(request.body);
}