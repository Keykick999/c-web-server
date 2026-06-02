#include "http_parser.h"

#include <string.h>
#include <stdlib.h>

static char* parseRequestLine(
    HttpRequest* request,
    char* buffer
) {

    char* header = strstr(
        buffer,
        "\r\n"
    );

    char request_line[1024];

    int idx = 0;

    for (
        char* p = buffer;
        p < header;
        p++
    ) {
        request_line[idx++] = *p;
    }

    request_line[idx] = '\0';

    char* method = strtok(
        request_line,
        " "
    );

    char* path = strtok(
        NULL,
        " "
    );

    char* version = strtok(
        NULL,
        " "
    );

    request->method = strdup(method);
    request->path = strdup(path);
    request->version = strdup(version);

    return header + 2;
}

static char* parseHeaders(
    HttpRequest* request,
    char* headerStart
) {

    char* line_start = headerStart;

    while (1) {

        if (*line_start == '\r') {
            break;
        }

        char* line_end = strstr(
            line_start,
            "\r\n"
        );

        if (line_end == NULL) {
            break;
        }

        char* delimiter = strstr(
            line_start,
            ":"
        );

        if (delimiter == NULL) {
            break;
        }

        char key[128];
        char value[1024];

        int idx = 0;

        for (
            char* cp = line_start;
            cp < delimiter;
            cp++
        ) {
            key[idx++] = *cp;
        }

        key[idx] = '\0';

        idx = 0;

        for (
            char* cp = delimiter + 2;
            cp < line_end;
            cp++
        ) {
            value[idx++] = *cp;
        }

        value[idx] = '\0';

        put(
            request->headers,
            key,
            value
        );

        line_start = line_end + 2;
    }

    return line_start + 2;
}

static void parseBody(
    HttpRequest* request,
    char* bodyStart
) {

    if (*bodyStart == '\0') {
        return;
    }

    char* line_start = bodyStart;

    while (1) {

        char* equal = strstr(
            line_start,
            "="
        );

        if (equal == NULL) {
            break;
        }

        char* line_end = strstr(
            line_start,
            "&"
        );

        if (line_end == NULL) {
            line_end =
                line_start +
                strlen(line_start);
        }

        char key[128];
        char value[1024];

        int idx = 0;

        for (
            char* cp = line_start;
            cp < equal;
            cp++
        ) {
            key[idx++] = *cp;
        }

        key[idx] = '\0';

        idx = 0;

        for (
            char* cp = equal + 1;
            cp < line_end;
            cp++
        ) {
            value[idx++] = *cp;
        }

        value[idx] = '\0';

        put(
            request->body,
            key,
            value
        );

        if (*line_end == '\0') {
            break;
        }

        line_start = line_end + 1;
    }
}

HttpRequest parseHttpRequest(
    char* buffer
) {
    HttpRequest request;

    request.method = NULL;
    request.path = NULL;
    request.version = NULL;

    request.headers =
        create_map();

    request.body =
        create_map();

    char* headerStart =
        parseRequestLine(
            &request,
            buffer
        );

    char* bodyStart =
        parseHeaders(
            &request,
            headerStart
        );

    parseBody(
        &request,
        bodyStart
    );

    return request;
}