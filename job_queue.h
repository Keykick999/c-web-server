#include <pthread.h>
#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#define QUEUE_SIZE 1024

// Worker가 처리할 작업
typedef struct {
    int client_fd;
} Job;

// 작업 큐
typedef struct {
    Job jobs[QUEUE_SIZE];

    int front;
    int rear;
    int size;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
    
} JobQueue;

// 초기화
void initQueue(JobQueue *queue);

// 삽입
int enqueue(JobQueue *queue, Job job);

// 삭제
Job dequeue(JobQueue *queue);

// 상태 확인
int isEmpty(JobQueue *queue);
int isFull(JobQueue *queue);

#endif