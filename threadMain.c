#include <stdio.h>
#include <pthread.h>

void* worker(void* arg) {
  int num = *(int*) arg;
  printf("받을 값: %d\n", num);
  return NULL;
}

int main() {
  int num = 100;

  pthread_t tid;

  pthread_create(
    &tid,
    NULL,
    worker,
    &num
  );

  pthread_join(tid, NULL);

  return 0;
}