#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

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
  server_addr.sin_addr.s_addr = INDDR_ANY;

  bind(
    listen_fd,
    (struct socket_addr*) &server_addr,
    sizeof(server_addr)
  );

  // tcp 연결 가능하게 -> 소켓 listen 상태로 변경
  listen(
    listen_fd,
    128 // backlog-queue 크기
  );

  // tcp 연결 수립 -> connected socket 커널에 생성
  // -> accept로 accept큐에 있는 connected socket의 fd 값 가져옴  
  int client_fd = accept(
    listen_fd,
    NULL,
    NULL
  );




}