//
// Created by Evan Kelch on 12/22/24.
//

#ifndef UTIL_H
#define UTIL_H

#define MAP_KEY_MAX 100
#define MAP_VALUE_MAX 256
#include "stdbool.h"
#include <SDL.h>

typedef struct Ek_LinkedList {
    char key[MAP_KEY_MAX];
    void *value;
    struct Ek_LinkedList *next;
} Ek_LinkedList;

typedef struct {
    int size;
    int capacity;
    Ek_LinkedList** mapArr;
} Ek_Map;

typedef struct {
    int size;
    int capacity;
    char** arr;
} Ek_List;

typedef struct {
    bool started;
    u_int64_t startTicks;
} LTimer;

void destroyLinkedList(Ek_LinkedList* head);
unsigned long hash(unsigned char* k);
unsigned long getIndex(Ek_Map map, char* key);
Ek_Map* map_new(int cap);
void* map_get(Ek_Map* map, char* key);
char** map_keys(const Ek_Map *map, int* keycount);
void map_put(Ek_Map* map, char* key, void* value);
void map_destroy(Ek_Map* map);

void startTimer(LTimer* t);
void stopTimer(LTimer* t);

Ek_List* list_new(const int capacity);
void list_add(Ek_List* list, char* in);
void list_deleteIndex(Ek_List* list, const int index);
#endif //UTIL_H
