#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "http_parser.h"
#include "http_request.h"
#include "hashmap.h"
#include "linked_list.h"
#include "client.h"

#define MAX_HEADERS 4096
#define MAX_BODY 4096

// 작업 스레드가 처리할 작업
void processClient(LinkedList* clients, Client* client) {
  char *buffer = client->buffer;
  char *headers = client->headers;
  char *body = client->body;

  int *body_len = &client->body_len;
  int *header_len = &client->header_len;
  int *contentLength = &client->contentLength;

  HttpRequest *request = &client->request;

  // 소켓 recv 버퍼에서 데이터 읽기
  int n = recv(client->client_fd, buffer, MAX_HEADERS - 1, 0);

  // 오류
  if (n < 0) {
    // 소켓에서 읽을 데이터 없음
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return;
    }

    // 그 외 에러들
    close(client->client_fd);
    removeNode(clients, client->client_fd);
    perror("recv");
    return;
  }

  // 연결 종료
  else if (n == 0) {
    close(client->client_fd);
    removeNode(clients, client->client_fd);
    return;
  }

  // 데이터 정상적으로 읽음
  else if (n > 0) {
    // 헤더 읽어야 하는 차례
    if (*contentLength == -1) {

      // buffer에 있는 값 헤더로 옮기기
      memcpy(
        headers + *header_len,
        buffer,
        n
      );

      *header_len += n;
      headers[*header_len] = '\0';

      // header 끝(\r\n\r\n) 찾기
      char* headerEnd = strstr(
        headers,
        "\r\n\r\n"
      );

      // 아직 header 안 끝남
      if (headerEnd == NULL) {
        return;
      }

      // 실제 header 크기 계산
      int headerSize =
        headerEnd - headers + 4;

      // 1. 헤더 파싱
      char* headerStart =
        parseRequestLine(
          request,
          headers
        );

      parseHeaders(
        request,
        headerStart
      );

      // 2. 헤더에 content-length라는 필드 있나 확인
      char* contentLengthHeader =
        get(
          request->headers,
          "Content-Length"
        );

      // 3. 헤더에 content-length 있으면 저장해두기
      if (contentLengthHeader != NULL) {
        *contentLength =
          atoi(contentLengthHeader);
      }
      else {
        *contentLength = 0;
      }

      // body 없음
      if (*contentLength == 0) {
        body[0] = '\0';
      }

      // body 있음
      else if (*contentLength > 0) {

        // 이미 읽혀 있는 body 크기(header 버퍼에 복사된 길이 - 실제 헤더 길이)
        int remain = *header_len - headerSize;

        if (remain > 0) {

          // 남은 값들 body buffer에 넣어두기
          memcpy(
            body + *body_len,
            headerEnd + 4,
            remain
          );

          *body_len += remain;
        }
      }
    }

    // body 읽어야 하는 차례
    else if (*contentLength > 0) {
      // 읽어온 값들 body buffer에 넣기
      memcpy(
        body + *body_len,
        buffer,
        n
      );

      *body_len += n;
    }
  }

  // if (n == 0) {
  //   printf("client disconnected\n");
  // }
  // else if (n < 0) {
  //   perror("recv");
  // }

  // body 다 안 읽었으면 return
  if (*contentLength > 0 && *body_len < *contentLength) {
    return;
  }

  // 바디 파싱
  body[*body_len] = '\0';
  parseBody(request, body);

  print_map(request->headers);
  print_map(request->body);

  if (strcmp(request->method, "GET") == 0 && strcmp(request->path, "/hello") == 0 
    && strcmp(request->version, "HTTP/1.1") == 0) {
    char response[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Length: 11\r\n"
    "\r\n"
    "Get Hello World";

    int sent = send(
      client->client_fd,
      response,
      strlen(response),
      0
    );

    if (sent < 0) {
      perror("send");
    }
  }

  else if (strcmp(request->method, "POST") == 0 && strcmp(request->path, "/hello") == 0 
    && strcmp(request->version, "HTTP/1.1") == 0) {
    char response[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Length: 11\r\n"
    "\r\n"
    "Post Hello World";

    int sent = send(
      client->client_fd,
      response,
      strlen(response),
      0
    );

    if (sent < 0) {
      perror("send");
    }
  }

  // http 1.0 기준으로 구현 했음
  close(client->client_fd);
  removeNode(clients, client->client_fd);

  return;
}




int main() {
  // client_fd 리스트
  LinkedList clients;
  initList(&clients);
  

  // 소켓 생성
  int listen_fd = socket(
    AF_INET,
    SOCK_STREAM,
    0
  );

  // listen_socket을 non-blocking 방식으로 설정
  int flags = fcntl(
    listen_fd,
    F_GETFL
  );

  fcntl(
    listen_fd,
    F_SETFL,
    flags | O_NONBLOCK
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

    if (client_fd >= 0) {
      // client_fd non-blocking 방식으로 설정
      int flags = fcntl(
        client_fd,
        F_GETFL
      );

      fcntl(
        client_fd,
        F_SETFL,
        flags | O_NONBLOCK
      );

      // 연결 성공이면 client 구조체 생성
      Client* client = malloc(sizeof(Client));

      if (client == NULL) {
        close(client_fd);
        continue;
      }
      
      initClient(client, client_fd);

      addFirst(&clients, client);
    }

    // 연결 실패
    else {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("accept");
      }
    }


    // client_fd들 중 읽을 데이터 있는지 전체 다 확인
    Client* curr = clients.head;

    // 연결된 모든 소켓들에 대해서 읽을 데이터 있는지 확인 후 파싱
    while (curr != NULL) {
      Client *next = curr->next;

      // recv
      processClient(&clients, curr);

      curr = next;
    }
  }
}