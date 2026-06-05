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
#include "hashmap.h"

#define MAX_HEADERS 4096
#define MAX_BODY 4096


// 작업 스레드가 처리할 작업
void* worker(void* args) {
  int client_fd = *(int*)args;
  free(args); // 메모리 누수 방지

  char buffer[MAX_HEADERS] = {0};
  char headers[MAX_HEADERS] = {0};
  char body[MAX_BODY] = {0};
  int body_len = 0;
  int header_len = 0;
  int n;
  int contentLength = -1; // content-length 헤더 값
  HttpRequest request;

  request.method = NULL;
  request.path = NULL;
  request.version = NULL;

  request.headers = create_map();
  request.body = create_map();

  // 정상적으로 데이터 읽을 수 있는 상황에서는 계속 읽기
  while ((n = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
    // 헤더 읽어야 하는 차례
    if (contentLength == -1) {

      // buffer에 있는 값 헤더로 옮기기
      memcpy(
        headers + header_len,
        buffer,
        n
      );

      header_len += n;
      headers[header_len] = '\0';

      // header 끝(\r\n\r\n) 찾기
      char* headerEnd = strstr(
        headers,
        "\r\n\r\n"
      );

      // 아직 header 안 끝남
      if (headerEnd == NULL) {
        continue;
      }

      // header 크기 계산
      int headerSize =
        headerEnd - headers + 4;

      // 1. 헤더 파싱
      char* headerStart =
        parseRequestLine(
          &request,
          headers
        );

      parseHeaders(
        &request,
        headerStart
      );

      // 2. 헤더에 content-length라는 필드 있나 확인
      char* contentLengthHeader =
        get(
          request.headers,
          "Content-Length"
        );

      // 3. 헤더에 content-length 있으면 저장해두기
      if (contentLengthHeader != NULL) {
        contentLength =
          atoi(contentLengthHeader);
      }
      else {
        contentLength = 0;
      }

      // body 없음
      if (contentLength == 0) {
        break;
      }

      // body 있음
      else if (contentLength > 0) {

        // 이미 읽혀 있는 body 크기
        int remain =
          header_len - headerSize;

        if (remain > 0) {

          // 남은 값들 body buffer에 넣어두기
          memcpy(
            body + body_len,
            headerEnd + 4,
            remain
          );

          body_len += remain;
        }
      }
    }

    // body 읽어야 하는 차례
    else if (contentLength > 0) {
      // 읽어온 값들 body buffer에 넣기
      memcpy(
        body + body_len,
        buffer,
        n
      );

      body_len += n;
    }

    // body 다 읽었으면 반복문 종료
    if (body_len >= contentLength) {
      break;
    }
  }

  if (n == 0) {
    printf("client disconnected\n");
  }
  else if (n < 0) {
    perror("recv");
  }

  // 바디 파싱
  body[body_len] = '\0';
  parseBody(&request, body);

  print_map(request.headers);
  print_map(request.body);

  if (strcmp(request.method, "GET") == 0 && strcmp(request.path, "/hello") == 0 
    && strcmp(request.version, "HTTP/1.1") == 0) {
    char response[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Length: 11\r\n"
    "\r\n"
    "Get Hello World";

    int sent = send(
      client_fd,
      response,
      strlen(response),
      0
    );

    if (sent < 0) {
      perror("send");
    }
  }

  else if (strcmp(request.method, "POST") == 0 && strcmp(request.path, "/hello") == 0 
    && strcmp(request.version, "HTTP/1.1") == 0) {
    char response[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Length: 11\r\n"
    "\r\n"
    "Post Hello World";

    int sent = send(
      client_fd,
      response,
      strlen(response),
      0
    );

    if (sent < 0) {
      perror("send");
    }
  }
  
  close(client_fd);
  destroyHttpRequest(&request);

  return NULL;
}





int main() {
  // 소켓 생성
  int listen_fd = socket(
    AF_INET,
    SOCK_STREAM,
    0
  );

  // 서버 재시작 시 TIME_WAIT 때문에 bind 실패하는 것 방지
  int opt = 1;

  setsockopt(
    listen_fd,
    SOL_SOCKET,
    SO_REUSEADDR,
    &opt,
    sizeof(opt)
  );

  if (listen_fd < 0) {
    perror("socket");
    exit(1);
  }

  printf("socket=%d\n", listen_fd);

  // 소켓에 서버 주소 설정
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(8080);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  int bindResult = bind(
    listen_fd,
    (struct sockaddr*) &server_addr,
    sizeof(server_addr)
  );

  printf("bind result=%d\n", bindResult);

  if (bindResult < 0) {
    perror("bind");
    exit(1);
}

  // tcp 연결 가능하게 -> 소켓 listen 상태로 변경
  int listenResult = listen(
    listen_fd,
    128 // backlog-queue 크기
  );

  printf("listen result=%d\n", listenResult);

  if (listenResult < 0) {
      perror("listen");
      exit(1);
  }

  while (1) {
    // 메인 스레드는 계속 accept
    int client_fd = accept(
      listen_fd,
      NULL,
      NULL
    );

    // 연결 실패
    if (client_fd < 0) {
      perror("accept");
      continue;
    }

    int* fd_ptr = malloc(sizeof(int));

    if (fd_ptr == NULL) {
      close(client_fd);
      continue;
    }

    *fd_ptr = client_fd;

    // 작업 스레드 생성
    pthread_t tid;

    int result = pthread_create(
      &tid,
      NULL,
      worker,
      fd_ptr  // client_fd 값을 worker 함수의 매개변수로 넘김
    );

    if (result != 0) {
      free(fd_ptr);
      close(client_fd);
      continue;
    }

    // worker thread 종료시에 바로 자원 정리
    if (pthread_detach(tid) != 0) {
      perror("pthread_detach");
    }
  }
}