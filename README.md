# HTTP Server Learning Journey

## 프로젝트 목표

C 언어를 이용하여 HTTP 서버를 구현하면서 서버의 동시성 처리 방식과 I/O Multiplexing의 발전 과정을 직접 구현하고 이해하는 것을 목표로 한다.

---

## 진행 과정

### 1. Blocking Server

가장 기본적인 TCP 서버를 구현하였다.

* `socket()`
* `bind()`
* `listen()`
* `accept()`
* `recv()`
* `send()`

한 번에 하나의 클라이언트만 처리할 수 있으며, 요청을 처리하는 동안 다른 클라이언트는 대기하게 된다.

---

### 2. Thread-per-Connection

클라이언트가 연결될 때마다 새로운 스레드를 생성하도록 변경하였다.

```
Client
    │
accept()
    │
pthread_create()
```

장점

* 여러 클라이언트를 동시에 처리 가능

단점

* 클라이언트 수만큼 스레드 생성
* Context Switching 비용 증가
* 메모리 사용량 증가

---

### 3. Thread Pool

미리 생성한 Worker Thread가 작업을 처리하도록 변경하였다.

```
Client
      │
accept()
      │
Job Queue
      │
Worker Thread
```

장점

* 스레드 생성 비용 제거
* 스레드 개수 제한 가능
* Thread-per-Connection보다 효율적

---

### 4. Single Thread + Non-blocking

모든 소켓을 Non-blocking 모드로 변경하였다.

```c
fcntl(fd, F_SETFL, flags | O_NONBLOCK);
```

`recv()`, `accept()`가 즉시 반환되도록 하여 하나의 스레드에서 여러 소켓을 처리할 수 있는 기반을 마련하였다.

이때 읽을 데이터가 없는 경우

```
errno == EAGAIN
```

또는

```
errno == EWOULDBLOCK
```

을 처리하도록 구현하였다.

---

### 5. select()

I/O Multiplexing을 적용하였다.

```
fd_set
      │
 select()
      │
Ready Socket
```

구현 내용

* `fd_set` 관리
* `FD_SET()`
* `FD_CLR()`
* `FD_ISSET()`
* `FD_ZERO()`
* `max_fd` 관리
* Listen Socket과 Client Socket을 동시에 감시

읽기 가능한 소켓만 처리하도록 변경하여 하나의 스레드에서 여러 클라이언트를 동시에 처리할 수 있도록 구현하였다.

---

## select()의 한계

select는 내부적으로

```
0 ~ max_fd
```

까지 모든 파일 디스크립터를 검사한다.

예를 들어

```
max_fd = 50000
```

이고 실제 연결된 소켓이

```
3
17
40
```

뿐이라도

```
0 ~ 50000
```

전체를 검사해야 한다.

또한

* `FD_SETSIZE` 제한
* 매번 `fd_set` 복사 필요
* `max_fd` 관리 필요

등의 단점이 존재한다.

---

## 현재까지 학습한 내용

* Blocking I/O
* Non-blocking I/O
* Thread-per-Connection
* Thread Pool
* select()
* HTTP Request Parsing
* Partial Read 처리
* Non-blocking Socket 처리
* Client 연결 및 해제 관리

---

## 앞으로 진행할 내용

* [ ] poll()
* [ ] epoll()
* [ ] Reactor Pattern
* [ ] Java NIO Selector
* [ ] Netty EventLoop
* [ ] Netty 기반 HTTP Server 분석
