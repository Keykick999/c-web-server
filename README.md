# HTTP Server Learning Journey

## 프로젝트 목표

C 언어를 이용하여 HTTP 서버를 구현하면서 서버의 동시성 처리 방식과 I/O Multiplexing의 발전 과정을 직접 구현하고 이해하는 것을 목표로 한다.

---

## 서버 발전 과정

```text
Blocking
    │
    ▼
Thread-per-Connection
    │
    ▼
Thread Pool
    │
    ▼
Single Thread + Non-blocking
    │
    ▼
select()
    │
    ▼
poll()
    │
    ▼
epoll()   ✅
    │
    ▼
Reactor Pattern
    │
    ▼
Java NIO Selector
    │
    ▼
Netty
```

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

```text
Client
    │
accept()
    │
pthread_create()
```

#### 장점

* 여러 클라이언트를 동시에 처리 가능

#### 단점

* 클라이언트 수만큼 스레드 생성
* Context Switching 비용 증가
* 메모리 사용량 증가

---

### 3. Thread Pool

미리 생성한 Worker Thread가 작업을 처리하도록 변경하였다.

```text
Client
      │
accept()
      │
Job Queue
      │
Worker Thread
```

#### 장점

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

읽을 데이터가 없는 경우

```text
errno == EAGAIN
```

또는

```text
errno == EWOULDBLOCK
```

을 처리하도록 구현하였다.

---

### 5. select()

I/O Multiplexing을 적용하였다.

```text
fd_set
      │
 select()
      │
Ready Socket
```

#### 구현 내용

* `fd_set` 관리
* `FD_SET()`
* `FD_CLR()`
* `FD_ISSET()`
* `FD_ZERO()`
* `max_fd` 관리
* Listen Socket과 Client Socket 동시 감시

읽기 가능한 소켓만 처리하도록 변경하여 하나의 스레드에서 여러 클라이언트를 동시에 처리할 수 있도록 구현하였다.

#### select()의 한계

select는 내부적으로

```text
0 ~ max_fd
```

까지 모든 파일 디스크립터를 검사한다.

예를 들어

```text
max_fd = 50000
```

이고 실제 연결된 소켓이

```text
3
17
40
```

뿐이라도

```text
0 ~ 50000
```

전체를 순회해야 한다.

또한

* `FD_SETSIZE` 제한
* 매 호출마다 `fd_set` 복사
* `max_fd` 직접 관리

등의 단점이 존재한다.

---

### 6. poll()

select의 한계를 개선하기 위해 `poll()` 기반의 I/O Multiplexing을 구현하였다.

```text
pollfd[]
      │
 poll()
      │
Ready Socket
```

#### 구현 내용

* `struct pollfd` 관리
* `events`, `revents` 이해 및 활용
* Listen Socket과 Client Socket 동시 감시
* `pollfd` 배열을 이용한 파일 디스크립터 관리
* Client 연결 및 해제 시 `pollfd` 배열 관리
* 기존 `select()` 서버를 `poll()` 기반으로 변경

#### select()와 poll()의 차이

**select()**

* `fd_set` 사용
* `max_fd` 관리 필요
* `FD_SET()`, `FD_CLR()` 등의 매크로 사용
* `FD_SETSIZE` 제한 존재

**poll()**

* `struct pollfd` 배열 사용
* `max_fd` 관리 불필요
* 감시할 파일 디스크립터 개수만 전달
* `FD_SETSIZE` 제한 없음 (운영체제 자원 한도 내)

하지만 `poll()` 역시 등록된 모든 파일 디스크립터를 순회하면서 이벤트를 확인하기 때문에 시간 복잡도는 **O(n)** 이다.

---

### 7. epoll()

Linux에서 제공하는 `epoll()` 기반의 I/O Multiplexing으로 서버를 변경하였다.

```text
Interest List
        │
epoll_wait()
        │
 Ready Socket
```

#### 구현 내용

* `epoll_create1()`
* `epoll_ctl()`

  * `EPOLL_CTL_ADD`
  * `EPOLL_CTL_DEL`
* `epoll_wait()`
* Listen Socket과 Client Socket 동시 감시
* Client 연결 및 해제 시 epoll Interest List 관리
* 기존 `poll()` 서버를 `epoll()` 기반으로 변경
* `EPOLLERR`, `EPOLLHUP` 예외 처리

#### poll()와 epoll()의 차이

**poll()**

* 모든 파일 디스크립터를 순회
* 이벤트가 발생하지 않은 소켓도 검사
* 시간 복잡도 **O(n)**

**epoll()**

* 커널이 Ready 상태의 파일 디스크립터만 관리
* 이벤트가 발생한 소켓만 사용자 공간으로 전달
* 대규모 동시 접속 환경에서 높은 성능 제공

`epoll()`은 `poll()`처럼 모든 파일 디스크립터를 순회하지 않고, 이벤트가 발생한 파일 디스크립터만 반환하므로 많은 연결을 처리하는 서버에서 훨씬 효율적으로 동작한다.

---

## 현재까지 학습한 내용

* Blocking I/O
* Non-blocking I/O
* Thread-per-Connection
* Thread Pool
* I/O Multiplexing

  * `select()`
  * `poll()`
  * `epoll()`
* HTTP Request Parsing
* Partial Read 처리
* Non-blocking Socket 처리
* Client 연결 및 해제 관리

---

## 앞으로 진행할 내용

* [x] `poll()`

* [x] `epoll()`

  * `epoll_create1()`
  * `epoll_ctl()`
  * `epoll_wait()`
  * `EPOLL_CTL_ADD`
  * `EPOLL_CTL_DEL`
  * `poll()` → `epoll()` 변환

* [ ] Level Trigger (LT)

* [ ] Edge Trigger (ET)

* [ ] `epoll_event.data.ptr` 활용

* [ ] Reactor Pattern

* [ ] Java NIO Selector

* [ ] Netty EventLoop

* [ ] Netty 기반 HTTP Server 분석