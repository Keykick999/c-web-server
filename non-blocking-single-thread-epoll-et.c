#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "http_parser.h"
#include "http_request.h"
#include "hashmap.h"
#include "client.h"

#define MAX_HEADERS 4096
#define MAX_BODY 4096
#define MAX_EVENTS 1024

void disconnectClient(int epfd, Client* client) {
  epoll_ctl(
    epfd,
    EPOLL_CTL_DEL,
    client->client_fd,
    NULL
  );

  close(client -> client_fd);

  free(client);
  return;
}

void setNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL);

  // fcntl 실패
  if (flags == -1) {
    perror("fcntl(F_GETFL)");
    return;
  }

  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("fcntl(F_SETFL)");
  }
}

// 작업 스레드가 처리할 작업
void processClient(Client* client, int epfd) {
  char *buffer = client->buffer;
  char *headers = client->headers;
  char *body = client->body;

  int *body_len = &client->body_len;
  int *header_len = &client->header_len;
  int *contentLength = &client->contentLength;

  HttpRequest *request = &client->request;

  // 소켓 recv 버퍼에서 데이터 읽기
  while (1) {
    int n = recv(client->client_fd, buffer, MAX_HEADERS - 1, 0);

    // 오류
    if (n < 0) {
      // 소켓에서 읽을 데이터 없음
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }

      // 그 외 에러들
      perror("recv");

      disconnectClient(epfd, client);

      return;
    }

    // 연결 종료
    else if (n == 0) {
      disconnectClient(epfd, client);
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
          continue;
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

    // body 다 안 읽었으면 continue
    if (*contentLength > 0 && *body_len < *contentLength) {
      continue;
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
  }
  
  // http 1.0 기준으로 구현 했음
  disconnectClient(epfd, client);

  return;
}




int main() {
  // epoll 객체 생성
  int epfd = epoll_create1(0);

  if (epfd == -1) {
    perror("epoll_create1");
    exit(1);
  }

  // 소켓 생성
  int listen_fd = socket(
    AF_INET,
    SOCK_STREAM,
    0
  );

  if (listen_fd < 0) {
    perror("socket");
    exit(1);
  }

  // 서버 재시작 시 TIME_WAIT 때문에 bind 실패하는 것 방지
  int opt = 1;

  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    perror("setsockopt");
    close(listen_fd);
    exit(1);
  }

  // listen_socket을 non-blocking 방식으로 설정
  setNonBlocking(listen_fd);

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

  // client 객체 생성
  Client listen_client;

  listen_client.client_fd = listen_fd;

  // listen_fd 추가
  struct epoll_event event;
  event.events = EPOLLIN | EPOLLET;
  event.data.ptr = &listen_client;

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &event) == -1) {
    perror("epoll_ctl");
    
    close(listen_fd);

    exit(1);
  }

  struct epoll_event events[MAX_EVENTS];

  while (1) {
    // 읽을 수 있는 소켓들 확인
    int readableSocketCount = epoll_wait(epfd, events, MAX_EVENTS, -1);


    // epoll 실패
    if (readableSocketCount < 0) {
      if (errno == EINTR) {
        continue;
      }

      perror("epoll");
      continue;
    }

    // 연결된 모든 소켓들에 대해서 읽을 데이터 있는지 확인 후 파싱(listen socket은 제외)
    for (int i = 0; i < readableSocketCount; i++) {
      Client* ev_client = events[i].data.ptr;

      if (events[i].events & (EPOLLERR | EPOLLHUP)) {
        if(ev_client->client_fd == listen_fd) {
          perror("listen socket error");
          exit(1);
        }

        disconnectClient(epfd, ev_client);
        continue;
      }

      // listen socket에서 읽을 데이터가 있으면 recv
      if (ev_client->client_fd == listen_fd) {
        if (events[i].events & EPOLLIN) {
          while(1) {
            // listen socket 처리
            int client_fd = accept(
              listen_fd,
              NULL,
              NULL
            );

            // 연결 성공
            if (client_fd >= 0) {
              // client_fd non-blocking 방식으로 설정
              setNonBlocking(client_fd);

              // 연결 성공이면 client 구조체 생성
              Client *new_client = malloc(sizeof(Client));

              if (new_client == NULL) {
                close(client_fd);
                continue;
              }
              
              initClient(new_client, client_fd);

              // 이벤트 객체에 추가(TODO: 이렇게 하면 원래 변수 덥어씌워지면서 생기는 문제는 없나)
              struct epoll_event client_event;
              client_event.events = EPOLLIN | EPOLLET;
              client_event.data.ptr = new_client;

              if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_event) == -1) {
                perror("epoll_ctl");

                close(client_fd);

                free(new_client);
                continue;
              }
            }

            // 연결 실패
            else {
              if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("accept");
              }

              break;
            }
          }
        }
      }

      // connected socket 처리
      else {
        if (!(events[i].events & EPOLLIN)) {
          continue;
        }

        if (ev_client != NULL) {
          processClient(ev_client, epfd);
        }
      }
    }
  }
}