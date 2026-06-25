#include <stdlib.h>
#include "linked_list.h"
#include "client.h"

void initList(LinkedList *list)
{
    list->head = NULL;
}

void addLast(LinkedList *list, Client *client)
{
    if (client == NULL)
        return;

    client->next = NULL;

    // 첫 노드
    if (list->head == NULL)
    {
        list->head = client;
        return;
    }

    Client *curr = list->head;

    while (curr->next != NULL)
    {
        curr = curr->next;
    }

    curr->next = client;
}

void addFirst(LinkedList *list, Client *client)
{
    if (client == NULL)
        return;

    client->next = list->head;
    list->head = client;
}

Client* findClient(LinkedList *list, int client_fd)
{
    Client *curr = list->head;

    while (curr != NULL)
    {
        if (curr->client_fd == client_fd)
        {
            return curr;
        }

        curr = curr->next;
    }

    return NULL;
}

void removeNode(LinkedList *list, int client_fd)
{
    Client *prev = NULL;
    Client *curr = list->head;

    while (curr != NULL)
    {
        if (curr->client_fd == client_fd)
        {
            if (prev == NULL)
            {
                list->head = curr->next;
            }
            else
            {
                prev->next = curr->next;
            }

            // Client 내부 자원 해제
            destroyClient(curr);

            // Client 구조체 메모리 해제
            free(curr);

            return;
        }

        prev = curr;
        curr = curr->next;
    }
}

void destroyList(LinkedList *list)
{
    Client *curr = list->head;

    while (curr != NULL)
    {
        Client *next = curr->next;

        destroyClient(curr);
        free(curr);

        curr = next;
    }

    list->head = NULL;
}

Client* findClient(LinkedList *list, int client_fd)
{
    Client *curr = list->head;

    while (curr != NULL)
    {
        if (curr->client_fd == client_fd)
        {
            return curr;
        }

        curr = curr->next;
    }

    return NULL;
}