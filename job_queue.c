#include "job_queue.h"

Job dequeue(JobQueue *queue)
{
    // 락 획득
    pthread_mutex_lock(
        &queue->mutex
    );

    while (isEmpty(queue)) {
        // 큐가 비었으면 기다리기
        pthread_cond_wait(
            &queue->cond,
            &queue->mutex
        );
    }

    Job job = queue->jobs[queue->front];

    queue->front = (queue->front + 1) % QUEUE_SIZE;
    queue->size--;

    // 락 해제
    pthread_mutex_unlock(
        &queue -> mutex
    );

    return job;
}

int enqueue(JobQueue *queue, Job job)
{
    // 락 획득
    pthread_mutex_lock(
        &queue->mutex
    );

    if (isFull(queue)) {
        pthread_mutex_unlock(
            &queue->mutex
        );
        return -1;
    }

    queue->jobs[queue->rear] = job;

    queue->rear = (queue->rear + 1) % QUEUE_SIZE;
    queue->size++;

    // dequeue가 가능하니 대기큐에서 스레드 하나 빼기
    pthread_cond_signal(
        &queue->cond
    );

    // 락 해제
    pthread_mutex_unlock(
        &queue->mutex
    );

    return 0;
}

void initQueue(JobQueue *queue)
{
    queue->front = 0;
    queue->rear = 0;
    queue->size = 0;

    // 뮤텍스 초기화
    pthread_mutex_init(
        &queue->mutex,
        NULL
    );

    // condition variable 초기화
    pthread_cond_init(
        &queue->cond,
        NULL
    );
}

int isEmpty(JobQueue *queue)
{
    return queue->size == 0;
}

int isFull(JobQueue *queue)
{
    return queue->size == QUEUE_SIZE;
}