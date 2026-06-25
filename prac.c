#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <poll.h>
#include <sys/epoll.h>

#include <fcntl.h>
#include <errno.h>

#include <sys/timerfd.h>
#include <unistd.h>
#include <stdint.h>


int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(
        listen_fd,
        (struct sockaddr*)&addr,
        sizeof(addr)
    );

    listen(listen_fd, 128);

    int epfd = epoll_create1(0);

    struct epoll_event ev;

    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;

    epoll_ctl(
        epfd,
        EPOLL_CTL_ADD,
        listen_fd,
        &ev
    );

    struct epoll_event events[1024];

    while (1) {
        int n = epoll_wait(
            epfd,
            events,
            1024,
            -1
        );

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == listen_fd) {
                int client_fd = accept(listen_fd, NULL, NULL);

                printf("client_fd: %d\n", client_fd);
            }
        }
    }
}





// 아래는 연습 코드

// fd_set 선언
fd_set readfds;

// fd_set 초기화
FD_ZERO(&readfds);

// listen_fd fd_set에 추가
FD_SET(listen_fd, &readfds);

// client1 fd_set에 추가
FD_SET(client1_fd, &readfds);

// client2 fd_set에 추가
FD_SET(client2_fd, &readfds);

// select로 읽을 수 있는 fd 개수 가져오기 => 근데 개수를 가져와서 어디에 쓰지 fd 값들도 아니고
int ready = select(
    max_fd + 1,
    &readfds,
    NULL,
    NULL,
    NULL
);

// select 이후에 비트 배열에서 읽을 준비가 된 fd만 비트가 켜지도록 수정됨.

// listen socket의 accept queue에 새로운 소켓이 들어옴
if (FD_ISSET(listen_fd, &readfds)) {
    // accept queue에서 소켓 빼고 fd 값 읽기
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(listen_fd, (struct sockaddr_in*) &client_addr, &client_len);

    // client_fd값을 fd_set에 추가
    fd_set readfds;
    FD_SET(client_fd, &readfds);

    // max_fd 값 갱신
    if (client_fd > max_fd) {
        max_fd = client_fd;
    }
}

// poll 연습 코드
struct pollfd fds[10];

fds[0].fd = client_fd;
fds[0].events = POLLIN;

int ret = poll(
    fds,
    1,
    -1
);

for (int i = 0; i < count; i++) {
    if (fds[i].revents & POLLIN) {

    }
}

// 1
struct pollfd fds[1];
fds[0].fd = STDIN_FILENO;
fds[0].events = POLLIN;

while (1) {
    printf("Waiting...\n");

    int ret = poll(fds, 1, -1);

    if (ret == -1) {
        perror("poll");
        continue;
    }

    if (ret > 0) {
        if (fds[0].revents & POLLIN) {
            // 입력 받기
            char str[100];
            fgets(str, sizeof(str), stdin);

            printf("%s", str);
        }
    }
}


// 2
int pipefd[2];

pipe(pipefd);

struct pollfd fds[2];

fds[0].fd = STDIN_FILENO;
fds[0].events = POLLIN;

fds[1].fd = pipefd[0];
fds[1].events = POLLIN;

while (1) {
    int ret = poll(fds, 2, -1); 

    // 이벤트 발생한 fd가 있음
    if (ret > 0) {
        for (int i = 0; i < 2; i++) {
            if (fds[i].revents & POLLIN) {
                if (i == 0) {
                    // 입력 받기
                    char str[100];
                    fgets(str, sizeof(str), stdin);
                    write(pipefd[1], str, strlen(str));
                }
                else if (i == 1) {
                    char str[100];
                    int n = read(pipefd[0], str, sizeof(str) - 1);
                    str[n] = '\0';
                    printf("Read from pipe : %s\n", str);
                }
            }
        }
    }
}


// 1. epoll 연습 문제
int epfd = epoll_create1(0);

if (epfd == -1) {
    perror("epoll create1");
    exit(1);
}

struct epoll_event{
    uint32_t events;
    epoll_data_t data;
}

struct epoll_event event;

event.events = EPOLLIN;
event.data.fd = client_fd;

// 감시목록 수정
epoll_ctl(
    epfd,
    op, // EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL
    fd,
    &event
);

int ready = epoll_wait(
    epfd,
    events,
    maxevents,
    timeout
);


// epoll 객체 생성
int epfd = epoll_create1(0);

struct epoll_event event;

event.events = EPOLLIN;
event.data.fd = listen_fd;


epoll_ctl(
    epfd,
    EPOLL_CTL_ADD,
    listen_fd,
    &event
);


// 2
#define MAX_EVENTS 1024

struct epoll_event events[MAX_EVENTS];

int ready = epoll_wait(
    epfd,
    events,
    MAX_EVENTS,
    -1
);

for (int i = 0; i < ready; i++) {
    if (events[i].events & POLLIN) {
        int fd = events[i].data.fd;
        printf("%d\n", fd);
    }
}


// 문제 3
if (fd == listen_fd) {

    int client_fd = accept(
        listen_fd,
        NULL,
        NULL
    );

    struct epoll_event event;

    event.events = EPOLLIN;
    event.data.fd = client_fd;

    epoll_ctl(
        epfd,
        EPOLL_CTL_ADD,
        client_fd,
        &event
    );
}

// 문제 4
if (n == 0) {
    epoll_ctl(
        epfd,
        EPOLL_CTL_DEL,
        client_fd,
        NULL
    );

    close(client_fd);
}


