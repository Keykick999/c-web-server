# Mini Tomcat in C

TCP Socket 기반 HTTP Server 구현 프로젝트

## Features

- TCP Server
- HTTP Request Parsing
- Request Line Parsing
- Header Parsing
- Body Parsing
- HashMap 기반 Header/Body 저장

## Example

Request

GET /hello HTTP/1.1
Host: localhost

Response

Hello World

## Build

gcc main.c hashmap.c http_request.c http_parser.c -o server

## Run

./server