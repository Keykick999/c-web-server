#ifndef HASHMAP_H
#define HASHMAP_H

#define TABLE_SIZE 101

typedef struct Node {
    char* key;
    char* value;
    struct Node* next;
} Node;

typedef struct {
    Node* buckets[TABLE_SIZE];
} HashMap;

HashMap* create_map();

void put(
    HashMap* map,
    char* key,
    char* value
);

char* get(
    HashMap* map,
    char* key
);

void print_map(
    HashMap* map
);

void destroy_map(
    HashMap* map
);

#endif