#include "hashmap.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static unsigned int hash(char* key) {

    unsigned int h = 0;

    while (*key) {
        h = h * 31 + (unsigned char)(*key);
        key++;
    }

    return h % TABLE_SIZE;
}

HashMap* create_map() {

    HashMap* map = malloc(sizeof(HashMap));

    for (int i = 0; i < TABLE_SIZE; i++) {
        map->buckets[i] = NULL;
    }

    return map;
}

void put(
    HashMap* map,
    char* key,
    char* value
) {

    unsigned int index = hash(key);

    Node* current = map->buckets[index];

    while (current != NULL) {

        if (strcmp(current->key, key) == 0) {

            free(current->value);
            current->value = strdup(value);

            return;
        }

        current = current->next;
    }

    Node* node = malloc(sizeof(Node));

    node->key = strdup(key);
    node->value = strdup(value);

    node->next = map->buckets[index];
    map->buckets[index] = node;
}

char* get(
    HashMap* map,
    char* key
) {

    unsigned int index = hash(key);

    Node* current = map->buckets[index];

    while (current != NULL) {

        if (strcmp(current->key, key) == 0) {
            return current->value;
        }

        current = current->next;
    }

    return NULL;
}

void print_map(
    HashMap* map
) {

    printf("========== HashMap ==========\n");

    for (int i = 0; i < TABLE_SIZE; i++) {

        Node* current = map->buckets[i];

        while (current != NULL) {

            printf(
                "%s -> %s\n",
                current->key,
                current->value
            );

            current = current->next;
        }
    }

    printf("=============================\n");
}

void destroy_map(
    HashMap* map
) {

    for (int i = 0; i < TABLE_SIZE; i++) {

        Node* current = map->buckets[i];

        while (current != NULL) {

            Node* next = current->next;

            free(current->key);
            free(current->value);
            free(current);

            current = next;
        }
    }

    free(map);
}