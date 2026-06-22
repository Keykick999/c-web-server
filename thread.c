#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct Job {
  pthread_mutex_t* mutex;
  int* count;
} Job;

void * worker(void* args) {
  Job* job = (Job*)args;

  pthread_mutex_t* mutex = job->mutex;
  int* count = job->count;


  for (int i = 0; i < 100000; i++) {
    pthread_mutex_lock(mutex);

    (*count)++;

    pthread_mutex_unlock(mutex);
  }

  return NULL;
}

int main() {
  int* count = malloc(sizeof(int));
  *count = 0;

  Job job;
  pthread_t tids[5];
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  job.mutex = &mutex;
  job.count = count;

  for (int i = 0; i < 5; i++) {
    pthread_create(
      &tids[i],
      NULL,
      worker,
      &job
    );
  };

  for (int i = 0; i < 5; i++) {
    pthread_join(tids[i], NULL);
  }

  printf("count 값: %d", *count);
}