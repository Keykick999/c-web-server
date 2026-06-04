#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <sys/epoll.h>

#include <fcntl.h>
#include <errno.h>


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