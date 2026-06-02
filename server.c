#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

int main() {

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));

    listen(listen_fd, 10);

    // listen socket non-blocking
    fcntl(listen_fd, F_SETFL, O_NONBLOCK);

    printf("waiting client...\n");

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

    struct epoll_event events[10];

    while (1) {

        int n = epoll_wait(
            epfd,
            events,
            10,
            -1
        );

        for (int i = 0; i < n; i++) {

            int fd = events[i].data.fd;

            // 새로운 연결
            if (fd == listen_fd) {

                int client_fd = accept(
                    listen_fd,
                    NULL,
                    NULL
                );

                if (client_fd < 0) {
                    continue;
                }

                // client도 non-blocking
                fcntl(
                    client_fd,
                    F_SETFL,
                    O_NONBLOCK
                );

                ev.events = EPOLLIN;
                ev.data.fd = client_fd;

                epoll_ctl(
                    epfd,
                    EPOLL_CTL_ADD,
                    client_fd,
                    &ev
                );

                printf(
                    "client connected: %d\n",
                    client_fd
                );
            }

            // 클라이언트 데이터
            else {

                char buffer[1024];

                int len = recv(
                    fd,
                    buffer,
                    sizeof(buffer) - 1,
                    0
                );

                if (len > 0) {

                    buffer[len] = '\0';

                    printf(
                        "fd=%d received: %s\n",
                        fd,
                        buffer
                    );
                }

                else if (len == 0) {

                    printf(
                        "client disconnected: %d\n",
                        fd
                    );

                    epoll_ctl(
                        epfd,
                        EPOLL_CTL_DEL,
                        fd,
                        NULL
                    );

                    close(fd);
                }

                else {

                    if (errno == EAGAIN ||
                        errno == EWOULDBLOCK) {

                        printf(
                            "fd=%d no more data\n",
                            fd
                        );
                    }
                    else {

                        perror("recv");

                        epoll_ctl(
                            epfd,
                            EPOLL_CTL_DEL,
                            fd,
                            NULL
                        );

                        close(fd);
                    }
                }
            }
        }
    }

    return 0;
}