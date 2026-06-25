#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "client.h"

typedef struct {
    Client *head;
} LinkedList;

// 초기화
void initList(LinkedList *list);

// 삽입
void addFirst(LinkedList *list, Client *client);
void addLast(LinkedList *list, Client *client);

// 검색
Client* findClient(LinkedList *list, int client_fd);

// 삭제
void removeNode(LinkedList *list, int client_fd);

// 메모리 해제
void destroyList(LinkedList *list);

Client* findClient(LinkedList *list, int client_fd);

#endif