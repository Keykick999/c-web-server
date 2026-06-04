#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "http_parser.h"
#include "http_request.h"
#include <pthread.h>

int main() {
  // 소켓 생성
  int listen_fd = socket(
    AF_INET,
    SOCK_STREAM,
    0
  );

  // 소켓에 서버 주소 설정
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(8080);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  bind(
    listen_fd,
    (struct sockaddr*) &server_addr,
    sizeof(server_addr)
  );

  // tcp 연결 가능하게 -> 소켓 listen 상태로 변경
  listen(
    listen_fd,
    128 // backlog-queue 크기
  );

  while (1) {
     // tcp 연결 수립 -> connected socket 커널에 생성
     // -> accept로 accept큐에 있는 connected socket의 fd 값 가져옴  
    int client_fd = accept(
      listen_fd,
      NULL,
      NULL
    );

    char buffer[4096];

    int n = recv(
      client_fd,
      buffer,
      sizeof(buffer) -1,
      0
    );

    // 상대방 연결 종로
    if (n == 0) {

    }
    // 에러
    else if (n < 0) {

    }
    // 정상 적으로 데이터 읽음
    else {
      buffer[n] = '\0';

      HttpRequest request = parseHttpRequest(buffer);
      print_map(request.headers);
      print_map(request.body);

      if (strcmp(request.method, "GET") == 0 && strcmp(request.path, "/hello") == 0 
      && strcmp(request.version, "HTTP/1.1") == 0) {
        char response[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello World";
        
        send(
          client_fd,
          response,
          strlen(response),
          0
        );

        close(client_fd);
        destroyHttpRequest(&request);
      }
    }
  }

}